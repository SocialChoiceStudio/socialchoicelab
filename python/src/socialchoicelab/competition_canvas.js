/**
 * Canvas-based competition trajectory animation widget.
 * Data stored once; frames drawn on demand. No per-frame payload duplication.
 *
 * Features:
 *  - Layered canvases: static background (grid, axes, voters, KDE heatmap)
 *    plus an animated foreground redrawn every frame.
 *  - Voter density heatmap (KDE, computed once at load; cached as offscreen canvas).
 *  - Live vote-share bar showing current per-candidate share inside the plot.
 *  - Heading / movement arrows derived from consecutive frame positions.
 *  - Ghost position: semi-transparent diamond N frames behind the current frame.
 *  - Keyboard scrubbing: ← / → to step, Space to play/pause.
 *  - Loop vs. stop-at-end toggle.
 *  - All display features individually togglable in a second controls row.
 *  - Scroll-wheel zoom (map style: fixed pixel sizes, zoom around cursor).
 *  - Drag-to-pan within the plot area. Double-click to reset.
 *  - Adaptive right-side padding: legend width computed from longest name.
 *  - ResizeObserver: re-renders when the host element changes size.
 */
(function() {
  var FONT   = "system-ui, -apple-system, BlinkMacSystemFont, sans-serif";
  var MONO   = "ui-monospace, 'Cascadia Code', Menlo, monospace";
  var COLORS = {
    background:    "#fafafa",
    plotBorder:    "#d4d4d4",
    grid:          "rgba(140,140,140,0.18)",
    axis:          "rgba(100,100,100,0.45)",
    text:          "#2d2d2d",
    textLight:     "#888",
    controls:      "#f2f2f2",
    controlBorder: "#ddd",
    button:        "#3a6bbf",
    buttonHover:   "#2e5a9e",
    buttonText:    "#fff"
  };

  // Colours for static geometry overlays.
  // Chosen to avoid clashing with candidate palettes (okabe_ito, dark2, competition_1d).
  // Centroid = crimson red; Marginal median = indigo-violet (avoids the blue in okabe_ito/competition_1d).
  var OVERLAY_COLORS = {
    centroid:        { pt: "rgba(185,10,10,0.95)",   stroke: "rgba(255,255,255,0.85)" },
    marginal_median: { pt: "rgba(100,0,180,0.90)",   stroke: "rgba(255,255,255,0.85)" },
    geometric_mean:  { pt: "rgba(100,55,0,0.95)",    stroke: "rgba(255,255,255,0.85)" },
    pareto_set:      { fill: "rgba(120,0,220,0.08)", border: "rgba(120,0,220,0.50)" }
  };
  // Human-readable labels for overlay keys.
  var OVERLAY_LABELS = {
    centroid:        "Centroid",
    marginal_median: "Marginal Median",
    geometric_mean:  "Geometric Mean",
    pareto_set:      "Pareto Set"
  };

  // ── Colour helpers ─────────────────────────────────────────────────────────

  function parseRGBA(str) {
    var m = str.match(
      /rgba?\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*(?:,\s*([\d.]+)\s*)?\)/
    );
    if (m) return { r: +m[1], g: +m[2], b: +m[3], a: m[4] != null ? +m[4] : 1 };
    if (str.charAt(0) === "#") {
      var hex = str.slice(1);
      if (hex.length === 6) {
        return {
          r: parseInt(hex.slice(0, 2), 16),
          g: parseInt(hex.slice(2, 4), 16),
          b: parseInt(hex.slice(4, 6), 16),
          a: 1
        };
      }
    }
    return { r: 0, g: 0, b: 0, a: 1 };
  }

  function rgba(c, alpha) {
    return "rgba(" + c.r + "," + c.g + "," + c.b + "," +
           (alpha != null ? alpha : c.a) + ")";
  }

  function formatTick(val) {
    return val.toFixed(2).replace(/\.?0+$/, "");
  }

  // ── UI helpers ─────────────────────────────────────────────────────────────

  function makeCheckbox(text, checked, accentColor, onChange) {
    var label = document.createElement("label");
    label.style.cssText = [
      "display:inline-flex",
      "align-items:center",
      "gap:3px",
      "font-size:11px",
      "color:#666",
      "cursor:pointer",
      "white-space:nowrap",
      "user-select:none"
    ].join(";");
    var cb = document.createElement("input");
    cb.type    = "checkbox";
    cb.checked = checked;
    cb.style.cssText = [
      "cursor:pointer",
      "accent-color:" + accentColor,
      "width:11px",
      "height:11px",
      "flex-shrink:0"
    ].join(";");
    cb.addEventListener("change", function() { onChange(cb.checked); });
    label.appendChild(cb);
    label.appendChild(document.createTextNode(text));
    return { el: label, input: cb };
  }

  // ── Widget constructor ─────────────────────────────────────────────────────

  function CompetitionCanvas(el, width, height) {
    this.el            = el;
    this.width         = width;
    this.height        = height;
    this.data          = null;
    this.currentFrame  = 0;
    this.playing       = false;
    this.rafId         = null;
    this.lastTime      = 0;
    this.frameDuration = 100;
    this.dpr           = window.devicePixelRatio || 1;
    this.controlsH     = 96;   // increases to 124 when overlay row is visible
    this.padding       = null;
    this.plotSide      = 0;
    this.heatmapCanvas = null;

    // View state (zoom / pan — start at full data range, set in _initView)
    this.viewXMin = 0;
    this.viewXMax = 1;
    this.viewYMin = 0;
    this.viewYMax = 1;
    this._zoomTimer = null;

    // Toggle state
    this.showHeatmap  = true;
    this.showVoteBar  = true;
    this.showArrows   = false;
    this.showGhost    = false;
    this.showIC       = false;
    this.showWinset   = false;
    this.showVoronoi  = false;
    this.loopPlayback = false;
    this.ghostFrames  = 5;
    this.trailMode    = "none";   // overwritten by setData
    this.pointScale   = 1;        // 1 = normal, 2 = doubled
    this.overlayToggles = {};     // key → boolean; populated by setData

    var self = this;

    // ── Outer container ──────────────────────────────────────────────────────
    var container = document.createElement("div");
    container.style.cssText = [
      "width:"         + (width  || 800) + "px",
      "height:"        + (height || 700) + "px",
      "position:relative",
      "background:#fff",
      "display:flex",
      "flex-direction:column",
      "font-family:"  + FONT,
      "box-sizing:border-box"
    ].join(";");
    this.container = container;

    // ── Canvas wrapper ────────────────────────────────────────────────────────
    var canvasWrap = document.createElement("div");
    canvasWrap.style.cssText = "position:relative;flex:1;min-height:0;overflow:hidden;";
    this.canvasWrap = canvasWrap;

    var bgCanvas = document.createElement("canvas");
    bgCanvas.style.cssText = "position:absolute;top:0;left:0;z-index:0;";
    this.bgCanvas = bgCanvas;
    canvasWrap.appendChild(bgCanvas);

    var fgCanvas = document.createElement("canvas");
    fgCanvas.style.cssText = "position:absolute;top:0;left:0;z-index:1;outline:none;";
    fgCanvas.tabIndex = 0;
    this.fgCanvas = fgCanvas;
    canvasWrap.appendChild(fgCanvas);

    container.appendChild(canvasWrap);

    // ── Controls outer ────────────────────────────────────────────────────────
    var controlsOuter = document.createElement("div");
    controlsOuter.style.cssText = [
      "flex-shrink:0",
      "background:"    + COLORS.controls,
      "border-top:1px solid " + COLORS.controlBorder,
      "display:flex",
      "flex-direction:column",
      "height:"        + this.controlsH + "px",
      "box-sizing:border-box"
    ].join(";");
    this.controlsOuter = controlsOuter;

    // ── Row 0 — scrubber ──────────────────────────────────────────────────────
    var row0 = document.createElement("div");
    row0.style.cssText = [
      "display:flex",
      "align-items:center",
      "gap:8px",
      "padding:6px 14px 2px"
    ].join(";");
    this.row0 = row0;

    // ── Row 1 — playback ──────────────────────────────────────────────────────
    var row1 = document.createElement("div");
    row1.style.cssText = [
      "display:flex",
      "align-items:center",
      "gap:8px",
      "padding:2px 14px 4px",
      "flex:1"
    ].join(";");
    this.row1 = row1;

    var playBtn = document.createElement("button");
    playBtn.type        = "button";
    playBtn.textContent = "\u25B6 Play";
    playBtn.style.cssText = [
      "cursor:pointer",
      "background:"    + COLORS.button,
      "color:"         + COLORS.buttonText,
      "border:none",
      "border-radius:5px",
      "padding:4px 12px",
      "font-size:12px",
      "font-family:"   + FONT,
      "font-weight:600",
      "letter-spacing:0.3px",
      "white-space:nowrap",
      "flex-shrink:0",
      "transition:background 0.15s"
    ].join(";");
    playBtn.addEventListener("mouseover", function() {
      this.style.background = COLORS.buttonHover;
    });
    playBtn.addEventListener("mouseout", function() {
      this.style.background = COLORS.button;
    });
    this.playBtn = playBtn;

    var slider = document.createElement("input");
    slider.type  = "range";
    slider.min   = 0;
    slider.max   = 1;
    slider.value = 0;
    slider.style.cssText = [
      "flex:1",
      "min-width:60px",
      "accent-color:" + COLORS.button,
      "cursor:pointer"
    ].join(";");
    this.slider = slider;

    var frameLabel = document.createElement("span");
    frameLabel.style.cssText = [
      "width:130px",
      "text-align:right",
      "font-size:11px",
      "color:"         + COLORS.textLight,
      "font-family:"   + MONO,
      "white-space:nowrap",
      "flex-shrink:0"
    ].join(";");
    this.frameLabel = frameLabel;

    var speedWrap = document.createElement("label");
    speedWrap.style.cssText = [
      "display:flex",
      "align-items:center",
      "gap:4px",
      "font-size:11px",
      "color:"         + COLORS.text,
      "white-space:nowrap",
      "flex-shrink:0"
    ].join(";");
    speedWrap.appendChild(document.createTextNode("Speed"));

    var speedSelect = document.createElement("select");
    speedSelect.style.cssText = [
      "font-size:11px",
      "border:1px solid " + COLORS.controlBorder,
      "border-radius:3px",
      "padding:2px 3px",
      "background:#fff",
      "cursor:pointer"
    ].join(";");
    [["0.5\u00D7", 0.5], ["1\u00D7", 1], ["2\u00D7", 2], ["4\u00D7", 4]].forEach(function(p) {
      var o = document.createElement("option");
      o.value = p[1]; o.textContent = p[0];
      speedSelect.appendChild(o);
    });
    speedSelect.value = "1";
    this.speedSelect = speedSelect;
    speedWrap.appendChild(speedSelect);

    var loopCb = makeCheckbox("Loop", false, COLORS.button, function(v) {
      self.loopPlayback = v;
    });

    // Zoom buttons  [−]  [+]  [↺]
    var zoomBtnStyle = [
      "cursor:pointer",
      "background:#fff",
      "color:"         + COLORS.text,
      "border:1px solid " + COLORS.controlBorder,
      "border-radius:4px",
      "padding:2px 7px",
      "font-size:13px",
      "line-height:1",
      "flex-shrink:0",
      "transition:background 0.1s"
    ].join(";");

    function makeZoomBtn(label, title, onClick) {
      var btn = document.createElement("button");
      btn.type        = "button";
      btn.textContent = label;
      btn.title       = title;
      btn.style.cssText = zoomBtnStyle;
      btn.addEventListener("mouseover", function() { this.style.background = "#eef"; });
      btn.addEventListener("mouseout",  function() { this.style.background = "#fff"; });
      btn.addEventListener("click", onClick);
      return btn;
    }

    var zoomSep = document.createElement("span");
    zoomSep.style.cssText = "width:1px;height:16px;background:" + COLORS.controlBorder + ";flex-shrink:0;margin:0 2px;";

    var zoomInBtn  = makeZoomBtn("+", "Zoom in",   function() { self._zoomBy(1.5);  });
    var zoomOutBtn = makeZoomBtn("\u2212", "Zoom out",  function() { self._zoomBy(1/1.5); });
    var zoomResetBtn = makeZoomBtn("\u21BA", "Reset zoom", function() {
      if (self.data) { self._initView(); self.drawBackground(); self.draw(); }
    });

    row0.appendChild(slider);
    row0.appendChild(frameLabel);

    var pointSep = document.createElement("span");
    pointSep.style.cssText = "width:1px;height:16px;background:" + COLORS.controlBorder + ";flex-shrink:0;margin:0 2px;";

    var pointBtn = document.createElement("button");
    pointBtn.type        = "button";
    pointBtn.textContent = "Large Points";
    pointBtn.title       = "Toggle large point sizes";
    pointBtn.style.cssText = zoomBtnStyle;
    pointBtn.addEventListener("mouseover", function() { this.style.background = "#eef"; });
    pointBtn.addEventListener("mouseout",  function() { this.style.background = self.pointScale === 2 ? "#dde" : "#fff"; });
    pointBtn.addEventListener("click", function() {
      self.pointScale = self.pointScale === 1 ? 2 : 1;
      pointBtn.style.background  = self.pointScale === 2 ? "#dde" : "#fff";
      pointBtn.style.fontWeight  = self.pointScale === 2 ? "700" : "normal";
      self.drawBackground();
      self.draw();
    });
    this.pointBtn = pointBtn;

    row1.appendChild(playBtn);
    row1.appendChild(speedWrap);
    row1.appendChild(loopCb.el);
    row1.appendChild(zoomSep);
    row1.appendChild(zoomOutBtn);
    row1.appendChild(zoomInBtn);
    row1.appendChild(zoomResetBtn);
    row1.appendChild(pointSep);
    row1.appendChild(pointBtn);

    // ── Row 2 — display toggles ───────────────────────────────────────────────
    var row2 = document.createElement("div");
    row2.style.cssText = [
      "display:flex",
      "align-items:center",
      "gap:10px",
      "padding:2px 14px 6px",
      "border-top:1px solid rgba(0,0,0,0.06)"
    ].join(";");
    this.row2 = row2;

    var showLabel = document.createElement("span");
    showLabel.textContent = "Show:";
    showLabel.style.cssText = "font-size:11px;color:#aaa;white-space:nowrap;flex-shrink:0;";
    row2.appendChild(showLabel);

    var heatCb = makeCheckbox("Heatmap", true, COLORS.button, function(v) {
      self.showHeatmap = v;
      self.drawBackground();
      self.draw();
    });
    row2.appendChild(heatCb.el);

    var vbarCb = makeCheckbox("Vote bar", true, COLORS.button, function(v) {
      self.showVoteBar = v;
      self.draw();
    });
    row2.appendChild(vbarCb.el);

    var arrowCb = makeCheckbox("Arrows", false, COLORS.button, function(v) {
      self.showArrows = v;
      self.draw();
    });
    row2.appendChild(arrowCb.el);

    var ghostWrap = document.createElement("label");
    ghostWrap.style.cssText = [
      "display:inline-flex",
      "align-items:center",
      "gap:4px",
      "font-size:11px",
      "color:#666",
      "white-space:nowrap"
    ].join(";");
    ghostWrap.appendChild(document.createTextNode("Ghost:"));

    var ghostSelect = document.createElement("select");
    ghostSelect.style.cssText = [
      "font-size:11px",
      "border:1px solid " + COLORS.controlBorder,
      "border-radius:3px",
      "padding:1px 4px",
      "background:#fff",
      "cursor:pointer"
    ].join(";");
    [["none", "None"], ["3", "3 rounds"], ["5", "5 rounds"], ["10", "10 rounds"]].forEach(function(p) {
      var o = document.createElement("option");
      o.value = p[0]; o.textContent = p[1];
      ghostSelect.appendChild(o);
    });
    ghostSelect.value = "none";
    ghostSelect.addEventListener("change", function() {
      if (ghostSelect.value === "none") {
        self.showGhost = false;
      } else {
        self.showGhost   = true;
        self.ghostFrames = parseInt(ghostSelect.value);
      }
      self.draw();
    });
    ghostWrap.appendChild(ghostSelect);
    row2.appendChild(ghostWrap);

    var trailWrap = document.createElement("label");
    trailWrap.style.cssText = [
      "display:inline-flex",
      "align-items:center",
      "gap:4px",
      "font-size:11px",
      "color:#666",
      "white-space:nowrap"
    ].join(";");
    trailWrap.appendChild(document.createTextNode("Trail:"));

    var trailSelect = document.createElement("select");
    trailSelect.style.cssText = [
      "font-size:11px",
      "border:1px solid " + COLORS.controlBorder,
      "border-radius:3px",
      "padding:1px 4px",
      "background:#fff",
      "cursor:pointer"
    ].join(";");
    [["none", "None"], ["fade", "Fade"], ["full", "Full"]].forEach(function(p) {
      var o = document.createElement("option");
      o.value = p[0]; o.textContent = p[1];
      trailSelect.appendChild(o);
    });
    trailSelect.value = "none";
    trailSelect.addEventListener("change", function() {
      self.trailMode = trailSelect.value;
      self.draw();
    });
    this.trailSelect = trailSelect;
    trailWrap.appendChild(trailSelect);
    row2.appendChild(trailWrap);

    // Overlay controls row — populated by setData() when overlays_static is present.
    // padding-left is set by resize() to match the plot left edge (same as row0/1/2).
    var overlayRow = document.createElement("div");
    overlayRow.style.cssText = [
      "display:none",
      "align-items:center",
      "flex-wrap:wrap",
      "gap:10px",
      "padding-top:0",
      "padding-right:10px",
      "padding-bottom:0",
      "padding-left:0",
      "height:28px",
      "font-size:12px",
      "font-family:" + FONT
    ].join(";");
    this.overlayRow = overlayRow;

    controlsOuter.appendChild(row0);
    controlsOuter.appendChild(row1);
    controlsOuter.appendChild(row2);
    controlsOuter.appendChild(overlayRow);
    container.appendChild(controlsOuter);
    el.appendChild(container);

    // ── Playback event listeners ───────────────────────────────────────────────

    playBtn.addEventListener("click", function() {
      self.playing = !self.playing;
      playBtn.textContent = self.playing ? "\u23F8 Pause" : "\u25B6 Play";
      if (self.playing) { self.lastTime = 0; self.requestFrame(); }
    });

    slider.addEventListener("input", function() {
      self.playing = false;
      playBtn.textContent = "\u25B6 Play";
      self.setFrame(Math.round(parseFloat(slider.value)));
    });

    speedSelect.addEventListener("change", function() {
      self.frameDuration = 100 / parseFloat(speedSelect.value);
    });

    // Keyboard scrubbing.
    fgCanvas.addEventListener("keydown", function(e) {
      if (!self.data) return;
      if (e.key === "ArrowRight") {
        self.playing = false;
        playBtn.textContent = "\u25B6 Play";
        self.setFrame(Math.min(self.currentFrame + 1, self.data.rounds.length - 1));
        e.preventDefault();
      } else if (e.key === "ArrowLeft") {
        self.playing = false;
        playBtn.textContent = "\u25B6 Play";
        self.setFrame(Math.max(self.currentFrame - 1, 0));
        e.preventDefault();
      } else if (e.key === " ") {
        playBtn.click();
        e.preventDefault();
      }
    });
    fgCanvas.addEventListener("click", function() { fgCanvas.focus(); });

    // ── Zoom / pan event listeners ────────────────────────────────────────────

    var dragging      = false;
    var dragStartCX   = 0, dragStartCY = 0;
    var dragStartVXMin = 0, dragStartVXMax = 0;
    var dragStartVYMin = 0, dragStartVYMax = 0;

    function overPlot(clientX, clientY) {
      if (!self.padding) return false;
      var rect = fgCanvas.getBoundingClientRect();
      var mx   = clientX - rect.left;
      var my   = clientY - rect.top;
      return mx >= self.padding.left && mx <= self.padding.left + self.plotSide &&
             my >= self.padding.top  && my <= self.padding.top  + self.plotSide;
    }

    fgCanvas.addEventListener("wheel", function(e) {
      if (!self.data || !overPlot(e.clientX, e.clientY)) return;
      e.preventDefault();
      var factor = e.deltaY < 0 ? 1.15 : 1 / 1.15;
      var rect   = fgCanvas.getBoundingClientRect();
      var mx     = e.clientX - rect.left;
      var my     = e.clientY - rect.top;
      var dataX  = self.pxToX(mx);
      var dataY  = self.pxToY(my);
      var xRange = self.viewXMax - self.viewXMin;
      var yRange = self.viewYMax - self.viewYMin;
      var nx     = xRange / factor;
      var ny     = yRange / factor;
      var tx     = (dataX - self.viewXMin) / xRange;
      var ty     = (self.viewYMax - dataY)  / yRange;
      self.viewXMin = dataX - tx * nx;
      self.viewXMax = dataX + (1 - tx) * nx;
      self.viewYMin = dataY - (1 - ty) * ny;
      self.viewYMax = dataY + ty * ny;
      self.drawBackground();
      self.draw();
    }, { passive: false });

    fgCanvas.addEventListener("mousedown", function(e) {
      if (!self.data || !overPlot(e.clientX, e.clientY)) return;
      dragging       = true;
      dragStartCX    = e.clientX;
      dragStartCY    = e.clientY;
      dragStartVXMin = self.viewXMin;
      dragStartVXMax = self.viewXMax;
      dragStartVYMin = self.viewYMin;
      dragStartVYMax = self.viewYMax;
      fgCanvas.style.cursor = "grabbing";
      e.preventDefault();
    });

    document.addEventListener("mousemove", function(e) {
      if (!dragging || !self.padding) return;
      var dx = (e.clientX - dragStartCX) / self.plotSide * (dragStartVXMax - dragStartVXMin);
      var dy = (e.clientY - dragStartCY) / self.plotSide * (dragStartVYMax - dragStartVYMin);
      self.viewXMin = dragStartVXMin - dx;
      self.viewXMax = dragStartVXMax - dx;
      self.viewYMin = dragStartVYMin + dy;
      self.viewYMax = dragStartVYMax + dy;
      self.drawBackground();
      self.draw();
    });

    document.addEventListener("mouseup", function() {
      if (dragging) {
        dragging = false;
        fgCanvas.style.cursor = overPlot._lastOver ? "grab" : "default";
      }
    });

    fgCanvas.addEventListener("mousemove", function(e) {
      if (!dragging) {
        fgCanvas.style.cursor = overPlot(e.clientX, e.clientY) ? "grab" : "default";
      }
    });

    fgCanvas.addEventListener("dblclick", function(e) {
      if (!self.data || !overPlot(e.clientX, e.clientY)) return;
      self._initView();
      self.drawBackground();
      self.draw();
    });

  }

  // ── View state ─────────────────────────────────────────────────────────────

  CompetitionCanvas.prototype._zoomBy = function(factor) {
    if (!this.data) return;
    var cx = (this.viewXMin + this.viewXMax) / 2;
    var cy = (this.viewYMin + this.viewYMax) / 2;
    var nx = (this.viewXMax - this.viewXMin) / factor;
    var ny = (this.viewYMax - this.viewYMin) / factor;
    this.viewXMin = cx - nx / 2;
    this.viewXMax = cx + nx / 2;
    this.viewYMin = cy - ny / 2;
    this.viewYMax = cy + ny / 2;
    this.drawBackground();
    this.draw();
  };

  CompetitionCanvas.prototype._initView = function() {
    var data = this.data;
    if (this.mode1d) {
      // 1D: only X range is used; Y is a placeholder and never rendered.
      // Expand view by 3% on each side so markers at the data extremes don't
      // sit flush against the clip boundary.
      var xExpand = (data.xlim[1] - data.xlim[0]) * 0.03;
      this.viewXMin = data.xlim[0] - xExpand;
      this.viewXMax = data.xlim[1] + xExpand;
      this.viewYMin = -1;
      this.viewYMax =  1;
    } else {
      // 2D: enforce equal aspect ratio so Euclidean IC circles render correctly.
      // DO NOT remove this — IC polygon data is geometrically correct and relies
      // on equal pixel-per-unit scaling to appear undistorted.
      var xRange = data.xlim[1] - data.xlim[0];
      var yRange = data.ylim[1] - data.ylim[0];
      var cx     = (data.xlim[0] + data.xlim[1]) / 2;
      var cy     = (data.ylim[0] + data.ylim[1]) / 2;
      var half   = Math.max(xRange, yRange) / 2;
      this.viewXMin = cx - half;
      this.viewXMax = cx + half;
      this.viewYMin = cy - half;
      this.viewYMax = cy + half;
    }
    // Store the full (unzoomed) view for _isZoomed comparisons.
    this._fullViewXMin = this.viewXMin;
    this._fullViewXMax = this.viewXMax;
    this._fullViewYMin = this.viewYMin;
    this._fullViewYMax = this.viewYMax;
  };

  CompetitionCanvas.prototype._isZoomed = function() {
    if (!this.data || this._fullViewXMin == null) return false;
    if (Math.abs(this.viewXMax - this.viewXMin - (this._fullViewXMax - this._fullViewXMin)) > 1e-9 ||
        Math.abs(this.viewXMin - this._fullViewXMin) > 1e-9) return true;
    if (this.mode1d) return false;
    return Math.abs(this.viewYMax - this.viewYMin - (this._fullViewYMax - this._fullViewYMin)) > 1e-9 ||
           Math.abs(this.viewYMin - this._fullViewYMin) > 1e-9;
  };

  // ── Data & frame control ───────────────────────────────────────────────────

  CompetitionCanvas.prototype.setData = function(data) {
    this.data         = data;
    this.mode1d       = (data.n_dims === 1);
    this.slider.max   = Math.max(0, data.rounds.length - 1);
    this.slider.value = 0;
    this.currentFrame = 0;
    this.trailMode    = data.trail || "fade";
    this.trailSelect.value = this.trailMode;

    this.showIC = false;
    this.showWinset = false;
    this.showVoronoi = false;

    // Build overlay checkboxes from overlays_static keys, plus IC and WinSet toggles.
    var self = this;
    var overlays = data.overlays_static;
    var overlayRow = this.overlayRow;
    overlayRow.innerHTML = "";
    this.overlayToggles = {};
    var POINT_KEYS = { centroid: 1, marginal_median: 1, geometric_mean: 1 };
    var POLY_KEYS  = { pareto_set: 1 };
    var hasOverlayItems = false;
    if (overlays && typeof overlays === "object") {
      var keys = Object.keys(overlays);
      if (keys.length > 0) {
        hasOverlayItems = true;
        var sep = document.createElement("span");
        sep.style.cssText = "color:#aaa;font-size:11px;padding-right:2px";
        sep.textContent = "Overlays:";
        overlayRow.appendChild(sep);
        keys.forEach(function(key) {
          self.overlayToggles[key] = false;
          var cb = makeCheckbox(
            OVERLAY_LABELS[key] || key, false, COLORS.button,
            function(v) { self.overlayToggles[key] = v; self.drawBackground(); }
          );
          overlayRow.appendChild(cb.el);
        });
      }
    }
    if (data.indifference_curves != null) {
      hasOverlayItems = true;
      var icCb = makeCheckbox("Indiff. Curves", false, COLORS.button, function(v) {
        self.showIC = v;
        self.drawBackground();
        self.draw();
      });
      overlayRow.appendChild(icCb.el);
    }
    if (data.winsets != null || data.winset_intervals_1d != null) {
      hasOverlayItems = true;
      var winsetCb = makeCheckbox("Win Set", false, COLORS.button, function(v) {
        self.showWinset = v;
        self.drawBackground();
        self.draw();
      });
      overlayRow.appendChild(winsetCb.el);
    }
    if (data.voronoi_cells != null) {
      hasOverlayItems = true;
      var voronoiCb = makeCheckbox("Cutlines", false, COLORS.button, function(v) {
        self.showVoronoi = v;
        self.drawBackground();
        self.draw();
      });
      overlayRow.appendChild(voronoiCb.el);
    }
    if (hasOverlayItems) {
      overlayRow.style.display = "flex";
      this.controlsH = 124;
    } else {
      overlayRow.style.display = "none";
      this.controlsH = 96;
    }

    this._initView();
    this.resize(this.width, this.height);
  };

  CompetitionCanvas.prototype.setFrame = function(idx) {
    if (!this.data || this.data.rounds.length === 0) return;
    idx = Math.max(0, Math.min(idx, this.data.rounds.length - 1));
    this.currentFrame = idx;
    this.slider.value = idx;
    var last = this.data.rounds.length - 1;
    this.frameLabel.textContent = this.data.rounds[idx] + " / " + this.data.rounds[last];
    this.draw();
  };

  CompetitionCanvas.prototype.requestFrame = function() {
    var self = this;
    if (!self.playing || !self.data) return;
    self.rafId = requestAnimationFrame(function(t) {
      self.rafId = null;
      if (!self.playing) return;
      if (self.lastTime === 0) self.lastTime = t;
      if (t - self.lastTime >= self.frameDuration) {
        self.lastTime = t;
        var next = self.currentFrame + 1;
        if (next >= self.data.rounds.length) {
          if (self.loopPlayback) {
            next = 0;
          } else {
            self.playing = false;
            self.playBtn.textContent = "\u25B6 Play";
            self.setFrame(self.data.rounds.length - 1);
            return;
          }
        }
        self.setFrame(next);
      }
      self.requestFrame();
    });
  };

  CompetitionCanvas.prototype.resize = function(width, height) {
    this.width  = width  || this.el.offsetWidth  || 700;
    this.height = height || this.el.offsetHeight || 600;
    var cw = this.width;
    if (!this.data || cw <= 0) return;

    // Adaptive right padding — measure longest competitor name (shared by 1D and 2D).
    var tmpCtx    = document.createElement("canvas").getContext("2d");
    tmpCtx.font   = "12px " + FONT;
    var nComp     = this.data.positions[0] ? this.data.positions[0].length : 0;
    var maxTextW  = tmpCtx.measureText("Voters").width;
    for (var i = 0; i < nComp; i++) {
      var nm = (this.data.competitor_names && this.data.competitor_names[i]) ||
               ("Candidate " + (i + 1));
      maxTextW = Math.max(maxTextW, tmpCtx.measureText(nm).width);
    }
    var rightPad = Math.max(130, Math.ceil(8 + 13 + maxTextW + 22));
    var dpr = this.dpr;

    if (this.mode1d) {
      // 1D: canvas height computed dynamically to fit legend + stats panel.
      // Legend column: nComp candidates + voters row + overlay rows + IC + WinSet.
      // Stats column: Centroid (header+2), Median (header+2), WinSet (header+nComp).
      var d1d      = this.data;
      var nComp1d  = (d1d && d1d.positions && d1d.positions[0]) ? d1d.positions[0].length : 1;
      var nLeg1d   = nComp1d + 1;
      if (d1d && d1d.overlays_static)      nLeg1d += Object.keys(d1d.overlays_static).length;
      if (d1d && d1d.indifference_curves)  nLeg1d++;
      if (d1d && d1d.winset_intervals_1d)  nLeg1d++;
      var statsStart1d  = 8 + nLeg1d * 22 + 10;   // sLegY01d=8, sLh1d=22, gap=10
      var nStat1d       = 0;
      var sOvlInit      = d1d && d1d.overlays_static;
      if (sOvlInit && sOvlInit["centroid"])        nStat1d += 3;  // header + 2 rows
      if (sOvlInit && sOvlInit["marginal_median"]) nStat1d += 3;
      if (d1d && d1d.winset_intervals_1d)          nStat1d += 1 + nComp1d;
      // Seat holders: always shown (header + 1 row per max seat holders, min 1)
      if (d1d && d1d.seat_holder_indices) {
        var nMaxSh1d = 0;
        for (var _shi = 0; _shi < d1d.seat_holder_indices.length; _shi++) {
          nMaxSh1d = Math.max(nMaxSh1d, (d1d.seat_holder_indices[_shi] || []).length);
        }
        nStat1d += 1 + Math.max(nMaxSh1d, 1);
      }
      var ch1d = Math.max(240, statsStart1d + nStat1d * 18 + 20);  // stH1d=18, pad=20
      var pad1d = { top: 0, right: rightPad, bottom: 0, left: 64 };
      [this.bgCanvas, this.fgCanvas].forEach(function(c) {
        c.width        = cw * dpr;
        c.height       = ch1d * dpr;
        c.style.width  = cw + "px";
        c.style.height = ch1d + "px";
      });
      this.padding  = pad1d;
      this.plotW    = cw - pad1d.left - rightPad;
      this.plotH1d  = ch1d;
      this.lineY    = 120;  // y-pixel of the number line within the canvas
      var padLeft1d = pad1d.left + "px";
      this.row0.style.paddingLeft  = padLeft1d;
      this.row0.style.paddingRight = rightPad + "px";
      this.row1.style.paddingLeft  = padLeft1d;
      this.row1.style.paddingRight = rightPad + "px";
      this.row2.style.paddingLeft        = padLeft1d;
      this.overlayRow.style.paddingLeft  = padLeft1d;
      this.drawBackground();
      this.setFrame(this.currentFrame);
      return;
    }

    // 2D path (unchanged).
    var ch = Math.max(0, this.height - this.controlsH);
    if (ch <= 0) return;

    // icon (8) + gap (13) + text + right margin (14)
    var pad    = { top: 68, right: rightPad, bottom: 60, left: 64 };
    var availW = cw - pad.left - pad.right;
    var availH = ch - pad.top  - pad.bottom;
    var side   = Math.max(0, Math.min(availW, availH));

    pad.left += Math.floor((availW - side) / 2);
    pad.top  += Math.floor((availH - side) / 2);

    [this.bgCanvas, this.fgCanvas].forEach(function(c) {
      c.width        = cw * dpr;
      c.height       = ch * dpr;
      c.style.width  = cw + "px";
      c.style.height = ch + "px";
    });

    this.padding  = pad;
    this.plotSide = side;

    var padRight = Math.max(8, cw - pad.left - side) + "px";
    var padLeft  = pad.left + "px";
    this.row0.style.paddingLeft  = padLeft;
    this.row0.style.paddingRight = padRight;
    this.row1.style.paddingLeft  = padLeft;
    this.row1.style.paddingRight = padRight;
    this.row2.style.paddingLeft        = padLeft;
    this.overlayRow.style.paddingLeft  = padLeft;

    this.computeHeatmap();
    this.drawBackground();
    this.setFrame(this.currentFrame);
  };

  // ── Coordinate transforms (use current view range for zoom/pan) ───────────

  CompetitionCanvas.prototype.xToPx = function(x) {
    var t = (x - this.viewXMin) / (this.viewXMax - this.viewXMin);
    return this.padding.left + t * this.plotSide;
  };

  CompetitionCanvas.prototype.yToPx = function(y) {
    var t = (y - this.viewYMin) / (this.viewYMax - this.viewYMin);
    return this.padding.top + (1 - t) * this.plotSide;
  };

  // 1D coordinate transform: data x → CSS pixel along the number line.
  CompetitionCanvas.prototype.xToPx1d = function(x) {
    var t = (x - this.viewXMin) / (this.viewXMax - this.viewXMin);
    return this.padding.left + t * this.plotW;
  };

  // Inverse: CSS pixel → data coordinate (used by zoom/pan).
  CompetitionCanvas.prototype.pxToX = function(cssPx) {
    return this.viewXMin + (cssPx - this.padding.left) / this.plotSide *
           (this.viewXMax - this.viewXMin);
  };

  CompetitionCanvas.prototype.pxToY = function(cssPy) {
    return this.viewYMin + (1 - (cssPy - this.padding.top) / this.plotSide) *
           (this.viewYMax - this.viewYMin);
  };

  // ── KDE heatmap ───────────────────────────────────────────────────────────

  CompetitionCanvas.prototype.computeHeatmap = function() {
    var data = this.data;
    if (!data || !this.padding || this.plotSide <= 0) return;
    var vx = data.voters_x || [];
    var vy = data.voters_y || [];
    if (vx.length === 0) { this.heatmapCanvas = null; return; }

    var kdeRes = Math.min(Math.round(this.plotSide), 160);
    var xRange = data.xlim[1] - data.xlim[0];
    var yRange = data.ylim[1] - data.ylim[0];
    var h      = 0.14 * Math.max(xRange, yRange);
    var h2inv  = 1 / (2 * h * h);
    var n      = vx.length;

    var density = new Float32Array(kdeRes * kdeRes);
    var maxD    = 0;
    for (var py = 0; py < kdeRes; py++) {
      var yd = data.ylim[0] + (1 - (py + 0.5) / kdeRes) * yRange;
      for (var px = 0; px < kdeRes; px++) {
        var xd = data.xlim[0] + (px + 0.5) / kdeRes * xRange;
        var d  = 0;
        for (var vi = 0; vi < n; vi++) {
          var dx = xd - vx[vi], dy = yd - vy[vi];
          d += Math.exp(-(dx * dx + dy * dy) * h2inv);
        }
        density[py * kdeRes + px] = d;
        if (d > maxD) maxD = d;
      }
    }

    var imgData = new ImageData(kdeRes, kdeRes);
    var pix     = imgData.data;
    for (var j = 0; j < kdeRes * kdeRes; j++) {
      var t  = maxD > 0 ? density[j] / maxD : 0;
      var t2 = t * t;
      pix[j * 4]     = 255;
      pix[j * 4 + 1] = Math.round(210 - t * 100);
      pix[j * 4 + 2] = Math.round(30 * (1 - t));
      pix[j * 4 + 3] = Math.round(t2 * 55);
    }

    var tmp = document.createElement("canvas");
    tmp.width  = kdeRes;
    tmp.height = kdeRes;
    tmp.getContext("2d").putImageData(imgData, 0, 0);
    this.heatmapCanvas = tmp;
  };

  // ── Static background layer ───────────────────────────────────────────────

  // Draw static geometry overlays onto the background canvas context.
  // ctx is already scaled by dpr and save()/restore() is handled by the caller.
  CompetitionCanvas.prototype._drawOverlaysStatic = function(ctx, pad, side) {
    var overlays = this.data && this.data.overlays_static;
    if (!overlays) return;
    var self    = this;
    var oKeys   = Object.keys(overlays);

    ctx.save();
    ctx.beginPath();
    ctx.rect(pad.left, pad.top, side, side);
    ctx.clip();

    for (var oi = 0; oi < oKeys.length; oi++) {
      var key = oKeys[oi];
      if (!self.overlayToggles[key]) continue;
      var obj = overlays[key];
      var oc  = OVERLAY_COLORS[key] || {};

      if (key === "pareto_set") {
        var poly = obj.polygon;
        if (!poly || poly.length < 2) continue;
        ctx.beginPath();
        ctx.moveTo(self.xToPx(poly[0][0]), self.yToPx(poly[0][1]));
        for (var pi = 1; pi < poly.length; pi++) {
          ctx.lineTo(self.xToPx(poly[pi][0]), self.yToPx(poly[pi][1]));
        }
        ctx.closePath();
        ctx.fillStyle   = oc.fill   || "rgba(63,81,181,0.10)";
        ctx.fill();
        ctx.strokeStyle = oc.border || "rgba(63,81,181,0.55)";
        ctx.lineWidth   = 1.5;
        ctx.setLineDash([6, 4]);
        ctx.stroke();
        ctx.setLineDash([]);
      } else {
        var px  = self.xToPx(obj.x);
        var py  = self.yToPx(obj.y);
        var ptR = 6 * self.pointScale;
        var ptC = oc.pt || "rgba(100,100,100,0.9)";
        var ptS = oc.stroke || "rgba(255,255,255,0.85)";
        if (key === "centroid") {
          // Pure cross.
          ctx.strokeStyle = ptC;
          ctx.lineWidth   = 2;
          ctx.beginPath();
          ctx.moveTo(px - ptR * 1.3, py); ctx.lineTo(px + ptR * 1.3, py);
          ctx.moveTo(px, py - ptR * 1.3); ctx.lineTo(px, py + ptR * 1.3);
          ctx.stroke();
        } else if (key === "marginal_median") {
          // Upward-pointing filled triangle.
          ctx.fillStyle   = ptC;
          ctx.strokeStyle = ptS;
          ctx.lineWidth   = 1.5;
          ctx.beginPath();
          ctx.moveTo(px, py - ptR);
          ctx.lineTo(px - ptR * 0.866, py + ptR * 0.5);
          ctx.lineTo(px + ptR * 0.866, py + ptR * 0.5);
          ctx.closePath();
          ctx.fill();
          ctx.stroke();
        } else {
          // Geometric mean (and unknown keys): bold X marker.
          ctx.strokeStyle = ptC;
          ctx.lineWidth   = 2.5;
          ctx.beginPath();
          ctx.moveTo(px - ptR * 0.8, py - ptR * 0.8); ctx.lineTo(px + ptR * 0.8, py + ptR * 0.8);
          ctx.moveTo(px + ptR * 0.8, py - ptR * 0.8); ctx.lineTo(px - ptR * 0.8, py + ptR * 0.8);
          ctx.stroke();
        }
      }
    }

    ctx.restore();
  };

  // ── 1D background (number line, rug, overlays, legend) ──────────────────────

  CompetitionCanvas.prototype.drawBackground1d = function() {
    var data  = this.data;
    var dpr   = this.dpr;
    var ctx   = this.bgCanvas.getContext("2d");
    var pad   = this.padding;
    var plotW = this.plotW;
    var lineY = this.lineY;
    var cw    = this.bgCanvas.width / dpr;
    var ch    = this.bgCanvas.height / dpr;

    ctx.clearRect(0, 0, this.bgCanvas.width, this.bgCanvas.height);
    ctx.save();
    ctx.scale(dpr, dpr);

    // Background strip
    ctx.fillStyle = COLORS.background;
    ctx.fillRect(pad.left, 0, plotW, ch);

    // Strip border
    ctx.strokeStyle = COLORS.plotBorder;
    ctx.lineWidth   = 1;
    ctx.strokeRect(pad.left + 0.5, 0.5, plotW - 1, ch - 1);

    // Title
    if (data.title) {
      ctx.fillStyle    = COLORS.text;
      ctx.font         = "bold 14px " + FONT;
      ctx.textAlign    = "center";
      ctx.textBaseline = "alphabetic";
      ctx.fillText(data.title, pad.left + plotW / 2, lineY - 56);
    }

    // Number line
    ctx.strokeStyle = COLORS.axis;
    ctx.lineWidth   = 1.5;
    ctx.beginPath();
    ctx.moveTo(pad.left, lineY);
    ctx.lineTo(pad.left + plotW, lineY);
    ctx.stroke();

    // X-axis ticks and labels
    var nTicks  = 6;
    var vxRange = this.viewXMax - this.viewXMin;
    ctx.strokeStyle  = COLORS.axis;
    ctx.lineWidth    = 1;
    ctx.font         = "11px " + FONT;
    ctx.textAlign    = "center";
    ctx.textBaseline = "top";
    ctx.fillStyle    = COLORS.textLight;
    for (var xi = 0; xi <= nTicks; xi++) {
      var xt  = xi / nTicks;
      var xv  = this.viewXMin + xt * vxRange;
      var xpx = this.xToPx1d(xv);
      ctx.beginPath();
      ctx.moveTo(xpx, lineY);
      ctx.lineTo(xpx, lineY + 6);
      ctx.stroke();
      ctx.fillText(formatTick(xv), xpx, lineY + 9);
    }

    // Axis label
    var axisName = (data.dim_names && data.dim_names[0]) || "Dimension 1";
    ctx.fillStyle    = COLORS.text;
    ctx.font         = "12px " + FONT;
    ctx.textAlign    = "center";
    ctx.textBaseline = "alphabetic";
    ctx.fillText(axisName, pad.left + plotW / 2, lineY + 40);

    // Voters — filled circles on the number line, same style as 2D
    var vx     = data.voters_x || [];
    var voterC = parseRGBA(data.voter_color || "rgba(100,100,100,0.55)");
    var voterR = 3.5 * this.pointScale;
    for (var vi = 0; vi < vx.length; vi++) {
      var vpx = this.xToPx1d(vx[vi]);
      if (vpx < pad.left - 8 || vpx > pad.left + plotW + 8) continue;
      ctx.beginPath();
      ctx.arc(vpx, lineY, voterR, 0, Math.PI * 2);
      ctx.fillStyle   = rgba(voterC, 0.50);
      ctx.fill();
      ctx.strokeStyle = "rgba(255,255,255,0.65)";
      ctx.lineWidth   = 0.8;
      ctx.stroke();
    }

    // Static overlays — same symbols as 2D, no text label
    var overlays = data.overlays_static;
    if (overlays && typeof overlays === "object") {
      var oKeys = Object.keys(overlays);
      for (var oi = 0; oi < oKeys.length; oi++) {
        var oKey = oKeys[oi];
        if (!this.overlayToggles[oKey]) continue;
        var ov = overlays[oKey];
        if (ov == null || ov.x == null) continue;
        var ovpx = this.xToPx1d(ov.x);
        if (ovpx < pad.left - 2 || ovpx > pad.left + plotW + 2) continue;
        var oc   = OVERLAY_COLORS[oKey] || {};
        var ptC  = oc.pt || "rgba(100,100,100,0.9)";
        var ptS  = oc.stroke || "rgba(255,255,255,0.85)";
        var ptR  = oKey === "marginal_median" ? 9 * this.pointScale : 6 * this.pointScale;
        if (oKey === "centroid") {
          // Pure cross — matches 2D
          ctx.strokeStyle = ptC;
          ctx.lineWidth   = 2;
          ctx.beginPath();
          ctx.moveTo(ovpx - ptR * 1.3, lineY); ctx.lineTo(ovpx + ptR * 1.3, lineY);
          ctx.moveTo(ovpx, lineY - ptR * 1.3); ctx.lineTo(ovpx, lineY + ptR * 1.3);
          ctx.stroke();
        } else if (oKey === "marginal_median") {
          // Upward-pointing filled triangle — matches 2D
          ctx.fillStyle   = ptC;
          ctx.strokeStyle = ptS;
          ctx.lineWidth   = 1.5;
          ctx.beginPath();
          ctx.moveTo(ovpx, lineY - ptR);
          ctx.lineTo(ovpx - ptR * 0.866, lineY + ptR * 0.5);
          ctx.lineTo(ovpx + ptR * 0.866, lineY + ptR * 0.5);
          ctx.closePath();
          ctx.fill();
          ctx.stroke();
        } else {
          // Fallback: bold X — matches 2D geometric_mean style
          ctx.strokeStyle = ptC;
          ctx.lineWidth   = 2.5;
          ctx.beginPath();
          ctx.moveTo(ovpx - ptR * 0.8, lineY - ptR * 0.8); ctx.lineTo(ovpx + ptR * 0.8, lineY + ptR * 0.8);
          ctx.moveTo(ovpx + ptR * 0.8, lineY - ptR * 0.8); ctx.lineTo(ovpx - ptR * 0.8, lineY + ptR * 0.8);
          ctx.stroke();
        }
      }
    }

    // Legend — right of the plot strip
    var nComp    = data.positions[0] ? data.positions[0].length : 0;
    var legX     = pad.left + plotW + 18;
    var legY0    = 8;
    var lineH    = 22;
    var legIconR = 5 * this.pointScale;
    var legTextX = legX + 8 + legIconR + 8;

    // Seats: N (bold, top of legend)
    var nSeats = (data.seat_holder_indices && data.seat_holder_indices[0])
      ? data.seat_holder_indices[0].length : 0;
    ctx.font         = "bold 12px " + FONT;
    ctx.fillStyle    = COLORS.text;
    ctx.textAlign    = "left";
    ctx.textBaseline = "middle";
    ctx.fillText("Seats: " + nSeats, legTextX, legY0);

    // Candidate symbols — inverted triangle (▼) to suggest 1D position marker
    for (var lc = 0; lc < nComp; lc++) {
      var ly   = legY0 + (lc + 1) * lineH;
      var lcol = parseRGBA(data.competitor_colors[lc] || "rgba(0,0,0,0.9)");
      var ts   = legIconR * 0.85;
      ctx.beginPath();
      ctx.moveTo(legX + 8,          ly + ts);
      ctx.lineTo(legX + 8 - ts,     ly - ts * 0.5);
      ctx.lineTo(legX + 8 + ts,     ly - ts * 0.5);
      ctx.closePath();
      ctx.fillStyle   = rgba(lcol, 0.95);
      ctx.fill();
      ctx.strokeStyle = "rgba(255,255,255,0.92)";
      ctx.lineWidth   = 1.5;
      ctx.stroke();
      ctx.font      = "12px " + FONT;
      ctx.fillStyle = COLORS.text;
      ctx.textAlign    = "left";
      ctx.textBaseline = "middle";
      ctx.fillText(
        (data.competitor_names && data.competitor_names[lc]) || ("Candidate " + (lc + 1)),
        legTextX, ly
      );
    }

    // Voter circle symbol in legend — matches 2D style
    var vlegY    = legY0 + (nComp + 1) * lineH;
    var voterLegR = 3.5 * this.pointScale;
    ctx.beginPath();
    ctx.arc(legX + 8, vlegY, voterLegR, 0, Math.PI * 2);
    ctx.fillStyle   = rgba(voterC, 0.50);
    ctx.fill();
    ctx.strokeStyle = "rgba(255,255,255,0.65)";
    ctx.lineWidth   = 0.8;
    ctx.stroke();
    ctx.font         = "12px " + FONT;
    ctx.fillStyle    = COLORS.text;
    ctx.textAlign    = "left";
    ctx.textBaseline = "middle";
    ctx.fillText("Voters", legTextX, vlegY);

    // Overlay legend entries — same symbols as 2D legend
    var legRow = legY0 + (nComp + 2) * lineH;
    if (overlays) {
      var olKeys = Object.keys(overlays);
      for (var oli = 0; oli < olKeys.length; oli++) {
        var olKey = olKeys[oli];
        if (!this.overlayToggles[olKey]) continue;
        var oly    = legRow;
        legRow    += lineH;
        var olc    = OVERLAY_COLORS[olKey] || {};
        var legCx  = legX + 8;
        var legR   = 5;
        ctx.font         = "12px " + FONT;
        ctx.textAlign    = "left";
        ctx.textBaseline = "middle";
        if (olKey === "centroid") {
          var ptCC = olc.pt || "rgba(100,100,100,0.9)";
          ctx.strokeStyle = ptCC; ctx.lineWidth = 1.5;
          ctx.beginPath();
          ctx.moveTo(legCx - legR, oly); ctx.lineTo(legCx + legR, oly);
          ctx.moveTo(legCx, oly - legR); ctx.lineTo(legCx, oly + legR);
          ctx.stroke();
        } else if (olKey === "marginal_median") {
          var ptCM = olc.pt || "rgba(100,100,100,0.9)";
          ctx.fillStyle   = ptCM;
          ctx.strokeStyle = "rgba(255,255,255,0.8)"; ctx.lineWidth = 1;
          ctx.beginPath();
          ctx.moveTo(legCx, oly - legR);
          ctx.lineTo(legCx - legR * 0.866, oly + legR * 0.5);
          ctx.lineTo(legCx + legR * 0.866, oly + legR * 0.5);
          ctx.closePath(); ctx.fill(); ctx.stroke();
        } else {
          var ptCX = olc.pt || "rgba(100,100,100,0.9)";
          ctx.strokeStyle = ptCX; ctx.lineWidth = 1.5;
          ctx.beginPath();
          ctx.moveTo(legCx - legR * 0.7, oly - legR * 0.7); ctx.lineTo(legCx + legR * 0.7, oly + legR * 0.7);
          ctx.moveTo(legCx + legR * 0.7, oly - legR * 0.7); ctx.lineTo(legCx - legR * 0.7, oly + legR * 0.7);
          ctx.stroke();
        }
        ctx.fillStyle = COLORS.text;
        ctx.fillText(OVERLAY_LABELS[olKey] || olKey, legTextX, oly);
      }
    }

    // IC legend entry
    if (this.showIC && data.indifference_curves) {
      var icLegY = legRow;
      legRow += lineH;
      var icLegColor = data.competitor_colors && data.competitor_colors[0]
        ? parseRGBA(data.competitor_colors[0])
        : { r: 128, g: 128, b: 128, a: 0.9 };
      ctx.strokeStyle = rgba(icLegColor, 0.40);
      ctx.lineWidth   = 1.5;
      ctx.beginPath();
      ctx.moveTo(legX + 8 - legIconR, icLegY);
      ctx.lineTo(legX + 8 + legIconR, icLegY);
      ctx.stroke();
      ctx.fillStyle    = COLORS.text;
      ctx.font         = "12px " + FONT;
      ctx.textAlign    = "left";
      ctx.textBaseline = "middle";
      ctx.fillText("Indiff. Curves", legTextX, icLegY);
    }

    ctx.restore();
  };

  CompetitionCanvas.prototype.drawBackground = function() {
    var data = this.data;
    if (!data || !this.padding) return;
    if (this.mode1d) { this.drawBackground1d(); return; }
    var canvas = this.bgCanvas;
    var ctx    = canvas.getContext("2d");
    var dpr    = this.dpr;
    var pad    = this.padding;
    var side   = this.plotSide;

    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.save();
    ctx.scale(dpr, dpr);

    // Plot background.
    ctx.fillStyle = COLORS.background;
    ctx.fillRect(pad.left, pad.top, side, side);

    // KDE heatmap: source-clip to the currently visible data range.
    if (this.showHeatmap && this.heatmapCanvas) {
      var fullXRange = data.xlim[1] - data.xlim[0];
      var fullYRange = data.ylim[1] - data.ylim[0];
      var kW   = this.heatmapCanvas.width;
      var kH   = this.heatmapCanvas.height;
      var sx   = (this.viewXMin - data.xlim[0]) / fullXRange * kW;
      var sy   = (1 - (this.viewYMax - data.ylim[0]) / fullYRange) * kH;
      var sw   = (this.viewXMax - this.viewXMin) / fullXRange * kW;
      var sh   = (this.viewYMax - this.viewYMin) / fullYRange * kH;
      ctx.save();
      ctx.imageSmoothingEnabled = true;
      ctx.imageSmoothingQuality = "high";
      ctx.drawImage(this.heatmapCanvas, sx, sy, sw, sh, pad.left, pad.top, side, side);
      ctx.restore();
    }

    // Plot border.
    ctx.strokeStyle = COLORS.plotBorder;
    ctx.lineWidth   = 1;
    ctx.strokeRect(pad.left + 0.5, pad.top + 0.5, side, side);

    // Grid lines (dashed, based on current view range).
    ctx.setLineDash([3, 4]);
    ctx.strokeStyle = COLORS.grid;
    ctx.lineWidth   = 1;
    var nGrid = 4;
    for (var gi = 1; gi < nGrid; gi++) {
      var gt = gi / nGrid;
      var gx = pad.left + gt * side;
      var gy = pad.top  + gt * side;
      ctx.beginPath(); ctx.moveTo(gx, pad.top);  ctx.lineTo(gx, pad.top + side); ctx.stroke();
      ctx.beginPath(); ctx.moveTo(pad.left, gy); ctx.lineTo(pad.left + side, gy); ctx.stroke();
    }
    ctx.setLineDash([]);

    // Tick marks + numeric labels (use current view range so labels update on zoom/pan).
    var nTicks   = 5;
    var vxRange  = this.viewXMax - this.viewXMin;
    var vyRange  = this.viewYMax - this.viewYMin;
    ctx.strokeStyle = COLORS.axis;
    ctx.lineWidth   = 1;
    ctx.font        = "11px " + FONT;

    ctx.textAlign    = "center";
    ctx.textBaseline = "top";
    ctx.fillStyle    = COLORS.textLight;
    for (var xi = 0; xi <= nTicks; xi++) {
      var xt  = xi / nTicks;
      var xv  = this.viewXMin + xt * vxRange;
      var xpx = pad.left + xt * side;
      ctx.beginPath(); ctx.moveTo(xpx, pad.top + side); ctx.lineTo(xpx, pad.top + side + 5); ctx.stroke();
      ctx.fillText(formatTick(xv), xpx, pad.top + side + 8);
    }

    ctx.textAlign    = "right";
    ctx.textBaseline = "middle";
    for (var yi = 0; yi <= nTicks; yi++) {
      var yt  = yi / nTicks;
      var yv  = this.viewYMin + yt * vyRange;
      var ypx = pad.top + (1 - yt) * side;
      ctx.beginPath(); ctx.moveTo(pad.left, ypx); ctx.lineTo(pad.left - 5, ypx); ctx.stroke();
      ctx.fillText(formatTick(yv), pad.left - 8, ypx);
    }

    // Axis labels.
    ctx.fillStyle    = COLORS.text;
    ctx.font         = "12px " + FONT;
    ctx.textAlign    = "center";
    ctx.textBaseline = "alphabetic";
    ctx.fillText(data.dim_names[0] || "Dimension 1", pad.left + side / 2, pad.top + side + 44);
    ctx.save();
    ctx.translate(pad.left - 48, pad.top + side / 2);
    ctx.rotate(-Math.PI / 2);
    ctx.fillText(data.dim_names[1] || "Dimension 2", 0, 0);
    ctx.restore();

    // Title.
    if (data.title) {
      ctx.fillStyle    = COLORS.text;
      ctx.font         = "bold 14px " + FONT;
      ctx.textAlign    = "center";
      ctx.textBaseline = "alphabetic";
      ctx.fillText(data.title, pad.left + side / 2, pad.top - 34);
    }

    // Voters — only draw those within the current view.
    var vx     = data.voters_x || [];
    var vy     = data.voters_y || [];
    var voterC = parseRGBA(data.voter_color || "rgba(100,100,100,0.55)");
    for (var vi2 = 0; vi2 < vx.length; vi2++) {
      var vpx = this.xToPx(vx[vi2]);
      var vpy = this.yToPx(vy[vi2]);
      if (vpx < pad.left - 8 || vpx > pad.left + side + 8 ||
          vpy < pad.top  - 8 || vpy > pad.top  + side + 8) continue;
      ctx.beginPath();
      ctx.arc(vpx, vpy, 3.5 * this.pointScale, 0, Math.PI * 2);
      ctx.fillStyle   = rgba(voterC, 0.50);
      ctx.fill();
      ctx.strokeStyle = "rgba(255,255,255,0.65)";
      ctx.lineWidth   = 0.8;
      ctx.stroke();
    }

    // Static geometry overlays (drawn above voters, below candidates).
    this._drawOverlaysStatic(ctx, pad, side);

    // Legend (static).
    var nComp  = data.positions[0] ? data.positions[0].length : 0;
    var legX   = pad.left + side + 18;
    var legY0  = pad.top  + 8;
    var lineH  = 22;

    ctx.font         = "12px " + FONT;
    ctx.textAlign    = "left";
    ctx.textBaseline = "middle";

    var legIconR = 5 * this.pointScale;
    var legTextX = legX + 8 + legIconR + 8;

    // Seats count — always shown at the top of the legend, bold.
    var nSeats = (data.seat_holder_indices && data.seat_holder_indices[0])
      ? data.seat_holder_indices[0].length : 0;
    ctx.font         = "bold 12px " + FONT;
    ctx.fillStyle    = COLORS.text;
    ctx.textAlign    = "left";
    ctx.textBaseline = "middle";
    ctx.fillText("Seats: " + nSeats, legTextX, legY0);

    for (var lc = 0; lc < nComp; lc++) {
      var ly   = legY0 + (lc + 1) * lineH;
      var lcol = parseRGBA(data.competitor_colors[lc] || "rgba(0,0,0,0.9)");
      this._drawDiamond(ctx, legX + 8, ly, legIconR, rgba(lcol, 0.95), 0);
      ctx.font      = "12px " + FONT;
      ctx.fillStyle = COLORS.text;
      ctx.fillText(
        (data.competitor_names && data.competitor_names[lc]) || ("Candidate " + (lc + 1)),
        legTextX, ly
      );
    }

    var vlegY = legY0 + (nComp + 1) * lineH;
    var voterLegR = 3.5 * this.pointScale;
    ctx.beginPath();
    ctx.arc(legX + 8, vlegY, voterLegR, 0, Math.PI * 2);
    ctx.fillStyle   = rgba(voterC, 0.50);
    ctx.fill();
    ctx.strokeStyle = "rgba(255,255,255,0.65)";
    ctx.lineWidth   = 0.8;
    ctx.stroke();
    ctx.font         = "12px " + FONT;
    ctx.fillStyle    = COLORS.text;
    ctx.textAlign    = "left";
    ctx.textBaseline = "middle";
    ctx.fillText("Voters", legTextX, vlegY);

    // Overlay legend entries — only for keys that are toggled on.
    var overlays    = data.overlays_static;
    var legRow      = legY0 + (nComp + 2) * lineH;
    if (overlays) {
      var oKeys = Object.keys(overlays);
      for (var oi = 0; oi < oKeys.length; oi++) {
        var oKey = oKeys[oi];
        if (!this.overlayToggles[oKey]) continue;
        var oly = legRow;
        legRow += lineH;
        var oc  = OVERLAY_COLORS[oKey] || {};
        ctx.font         = "12px " + FONT;
        ctx.textAlign    = "left";
        ctx.textBaseline = "middle";
        var legCx = legX + 8;
        var legR  = 5;
        if (oKey === "pareto_set") {
          ctx.fillStyle   = oc.fill   || "rgba(120,0,220,0.08)";
          ctx.fillRect(legX + 1, oly - 6, 14, 12);
          ctx.strokeStyle = oc.border || "rgba(120,0,220,0.55)";
          ctx.lineWidth   = 1.5;
          ctx.setLineDash([4, 3]);
          ctx.strokeRect(legX + 1, oly - 6, 14, 12);
          ctx.setLineDash([]);
        } else if (oKey === "centroid") {
          var ptC = oc.pt || "rgba(100,100,100,0.9)";
          ctx.strokeStyle = ptC; ctx.lineWidth = 1.5;
          ctx.beginPath();
          ctx.moveTo(legCx - legR, oly); ctx.lineTo(legCx + legR, oly);
          ctx.moveTo(legCx, oly - legR); ctx.lineTo(legCx, oly + legR);
          ctx.stroke();
        } else if (oKey === "marginal_median") {
          var ptC2 = oc.pt || "rgba(100,100,100,0.9)";
          ctx.fillStyle   = ptC2;
          ctx.strokeStyle = "rgba(255,255,255,0.8)"; ctx.lineWidth = 1;
          ctx.beginPath();
          ctx.moveTo(legCx, oly - legR);
          ctx.lineTo(legCx - legR * 0.866, oly + legR * 0.5);
          ctx.lineTo(legCx + legR * 0.866, oly + legR * 0.5);
          ctx.closePath(); ctx.fill(); ctx.stroke();
        } else {
          var ptC3 = oc.pt || "rgba(100,100,100,0.9)";
          ctx.strokeStyle = ptC3; ctx.lineWidth = 1.5;
          ctx.beginPath();
          ctx.moveTo(legCx - legR * 0.7, oly - legR * 0.7); ctx.lineTo(legCx + legR * 0.7, oly + legR * 0.7);
          ctx.moveTo(legCx + legR * 0.7, oly - legR * 0.7); ctx.lineTo(legCx - legR * 0.7, oly + legR * 0.7);
          ctx.stroke();
        }
        ctx.fillStyle = COLORS.text;
        ctx.fillText(OVERLAY_LABELS[oKey] || oKey, legTextX, oly);
      }
    }

    // IC legend entry — shown only when ICs are toggled on.
    if (this.showIC && data.indifference_curves) {
      var icLegY  = legRow;
      legRow += lineH;
      var icLegCx = legX + 8;
      var icLegR  = 5;
      var icLegColor = data.competitor_colors && data.competitor_colors[0]
        ? parseRGBA(data.competitor_colors[0])
        : {r: 128, g: 128, b: 128, a: 0.9};
      ctx.strokeStyle = rgba(icLegColor, 0.40);
      ctx.lineWidth   = 1.5;
      ctx.setLineDash([]);
      ctx.beginPath();
      ctx.arc(icLegCx, icLegY, icLegR, 0, Math.PI * 2);
      ctx.stroke();
      ctx.fillStyle = COLORS.text;
      ctx.font      = "12px " + FONT;
      ctx.textAlign    = "left";
      ctx.textBaseline = "middle";
      ctx.fillText("Indiff. Curves", legTextX, icLegY);
    }

    // WinSet legend entry — shown only when WinSets are toggled on.
    if (this.showWinset && data.winsets) {
      var wsLegY  = legRow;
      legRow += lineH;
      var wsLegColor = data.competitor_colors && data.competitor_colors[0]
        ? parseRGBA(data.competitor_colors[0])
        : {r: 128, g: 128, b: 128, a: 0.9};
      ctx.fillStyle   = rgba(wsLegColor, 0.10);
      ctx.fillRect(legX + 1, wsLegY - 6, 14, 12);
      ctx.strokeStyle = rgba(wsLegColor, 0.70);
      ctx.lineWidth   = 2.5;
      ctx.setLineDash([4, 3]);
      ctx.strokeRect(legX + 1, wsLegY - 6, 14, 12);
      ctx.setLineDash([]);
      ctx.fillStyle = COLORS.text;
      ctx.font      = "12px " + FONT;
      ctx.textAlign    = "left";
      ctx.textBaseline = "middle";
      ctx.fillText("Win Set", legTextX, wsLegY);
    }

    // Cutlines legend entry — shown only when Cutlines (Voronoi) is toggled on.
    if (this.showVoronoi && data.voronoi_cells) {
      var voLegY  = legRow;
      legRow += lineH;
      var voLegColor = data.competitor_colors && data.competitor_colors[0]
        ? parseRGBA(data.competitor_colors[0])
        : {r: 128, g: 128, b: 128, a: 0.9};
      ctx.fillStyle   = rgba(voLegColor, 0.10);
      ctx.fillRect(legX + 1, voLegY - 6, 14, 12);
      ctx.strokeStyle = rgba(voLegColor, 0.70);
      ctx.lineWidth   = 2.5;
      ctx.setLineDash([4, 3]);
      ctx.strokeRect(legX + 1, voLegY - 6, 14, 12);
      ctx.setLineDash([]);
      ctx.fillStyle = COLORS.text;
      ctx.font      = "12px " + FONT;
      ctx.textAlign    = "left";
      ctx.textBaseline = "middle";
      ctx.fillText("Cutlines", legTextX, voLegY);
    }

    ctx.restore();
  };

  // ── 1D animated foreground layer ──────────────────────────────────────────

  CompetitionCanvas.prototype.draw1d = function() {
    var data       = this.data;
    var dpr        = this.dpr;
    var ctx        = this.fgCanvas.getContext("2d");
    var frameIdx   = this.currentFrame;
    var nComp      = data.positions[0] ? data.positions[0].length : 0;
    var pad        = this.padding;
    var plotW      = this.plotW;
    var lineY      = this.lineY;
    var candSize   = 8 * this.pointScale;

    ctx.clearRect(0, 0, this.fgCanvas.width, this.fgCanvas.height);
    ctx.save();
    ctx.scale(dpr, dpr);

    // Clip to plot strip
    ctx.save();
    ctx.beginPath();
    ctx.rect(pad.left, 0, plotW, this.plotH1d);
    ctx.clip();

    // IC vertical lines — one pair per voter at the indifference endpoints.
    // Each endpoint is where a voter is equidistant between their ideal point
    // and the seat holder; drawn as a thin vertical stroke crossing the number line.
    if (this.showIC && data.indifference_curves && data.indifference_curves[frameIdx]) {
      var icFrame  = data.indifference_curves[frameIdx];
      var icCidxs  = data.ic_competitor_indices[frameIdx];
      var icTop    = lineY - 22;
      var icBot    = lineY + 10;
      for (var si = 0; si < icFrame.length; si++) {
        var seatCidx     = icCidxs[si];
        var scol         = parseRGBA(data.competitor_colors[seatCidx] || "rgba(128,128,128,0.9)");
        var sVoterCurves = icFrame[si];
        ctx.strokeStyle = rgba(scol, 0.30);
        ctx.lineWidth   = 1 * this.pointScale;
        ctx.lineCap     = "round";
        for (var vi = 0; vi < sVoterCurves.length; vi++) {
          var pts = sVoterCurves[vi];
          if (!pts || pts.length === 0) continue;
          var lx = this.xToPx1d(pts[0]);
          ctx.beginPath();
          ctx.moveTo(lx, icTop);
          ctx.lineTo(lx, icBot);
          ctx.stroke();
          if (pts.length >= 2) {
            var rx = this.xToPx1d(pts[1]);
            ctx.beginPath();
            ctx.moveTo(rx, icTop);
            ctx.lineTo(rx, icBot);
            ctx.stroke();
          }
        }
      }
    }

    // WinSet band — semi-transparent filled rect, same vertical span as the IC lines.
    if (this.showWinset && data.winset_intervals_1d && data.winset_intervals_1d[frameIdx]) {
      var wsFrame1d = data.winset_intervals_1d[frameIdx];
      var wsBandTop = lineY - 22;   // matches icTop
      var wsBandH   = 32;           // icBot (lineY+10) - icTop (lineY-22) = 32
      for (var wsi = 0; wsi < wsFrame1d.length; wsi++) {
        var wsEntry = wsFrame1d[wsi];
        if (!wsEntry || isNaN(wsEntry.lo) || isNaN(wsEntry.hi)) continue;
        var wcol  = parseRGBA(data.competitor_colors[wsEntry.competitor_idx] || "rgba(128,128,128,0.9)");
        var wsX0  = this.xToPx1d(wsEntry.lo);
        var wsX1  = this.xToPx1d(wsEntry.hi);
        var wsW   = wsX1 - wsX0;
        if (wsW <= 0) continue;
        // Faint fill
        ctx.fillStyle = rgba(wcol, 0.12);
        ctx.fillRect(wsX0, wsBandTop, wsW, wsBandH);
        // Endpoint strokes
        ctx.strokeStyle = rgba(wcol, 0.40);
        ctx.lineWidth   = 1.5 * this.pointScale;
        ctx.lineCap     = "round";
        ctx.beginPath();
        ctx.moveTo(wsX0, wsBandTop - 2);
        ctx.lineTo(wsX0, wsBandTop + wsBandH + 2);
        ctx.stroke();
        ctx.beginPath();
        ctx.moveTo(wsX1, wsBandTop - 2);
        ctx.lineTo(wsX1, wsBandTop + wsBandH + 2);
        ctx.stroke();
      }
    }

    // Trails — polyline of past x positions along lineY
    if (this.trailMode !== "none" && frameIdx > 0) {
      for (var tc = 0; tc < nComp; tc++) {
        var tcol     = parseRGBA(data.competitor_colors[tc] || "rgba(0,0,0,0.9)");
        var startIdx = this.trailMode === "fade"
          ? Math.max(0, frameIdx - data.trail_length)
          : 0;
        for (var tf = startIdx; tf < frameIdx; tf++) {
          var alpha = this.trailMode === "fade"
            ? (tf - startIdx + 1) / (frameIdx - startIdx + 1) * 0.5
            : 0.35;
          var tx0 = this.xToPx1d(data.positions[tf][tc][0]);
          var tx1 = this.xToPx1d(data.positions[tf + 1][tc][0]);
          ctx.strokeStyle = rgba(tcol, alpha);
          ctx.lineWidth   = 1.5 * this.pointScale;
          ctx.lineCap     = "round";
          ctx.beginPath();
          ctx.moveTo(tx0, lineY);
          ctx.lineTo(tx1, lineY);
          ctx.stroke();
        }
      }
    }

    // Candidate markers — filled inverted triangles sitting on the line
    for (var cc = 0; cc < nComp; cc++) {
      var ccolor = parseRGBA(data.competitor_colors[cc] || "rgba(0,0,0,0.9)");
      var cx     = this.xToPx1d(data.positions[frameIdx][cc][0]);
      var cs     = candSize;
      ctx.beginPath();
      ctx.moveTo(cx,      lineY + cs);
      ctx.lineTo(cx - cs, lineY - cs * 0.5);
      ctx.lineTo(cx + cs, lineY - cs * 0.5);
      ctx.closePath();
      if (cs > 3) {
        ctx.shadowColor   = "rgba(0,0,0,0.30)";
        ctx.shadowBlur    = 7;
        ctx.shadowOffsetY = 1;
      }
      ctx.fillStyle   = rgba(ccolor, 0.95);
      ctx.fill();
      ctx.shadowColor   = "rgba(0,0,0,0)";
      ctx.shadowBlur    = 0;
      ctx.shadowOffsetY = 0;
      ctx.strokeStyle   = "rgba(255,255,255,0.92)";
      ctx.lineWidth     = 1.5;
      ctx.stroke();
    }

    // Movement arrows — horizontal arrows above the candidate markers
    if (this.showArrows && frameIdx > 0) {
      var xScale = plotW / (this.viewXMax - this.viewXMin);
      for (var ac = 0; ac < nComp; ac++) {
        var acol  = parseRGBA(data.competitor_colors[ac] || "rgba(0,0,0,0.9)");
        var prevX = data.positions[frameIdx - 1][ac][0];
        var currX = data.positions[frameIdx][ac][0];
        var dpx   = (currX - prevX) * xScale;
        if (Math.abs(dpx) < 0.5) continue;
        var dir      = dpx > 0 ? 1 : -1;
        var baseX    = this.xToPx1d(currX);
        var arrowY   = lineY - candSize - 6 * this.pointScale;
        var shaftLen = 10 * this.pointScale;
        var headSize = 6  * this.pointScale;
        var tipX     = baseX + dir * shaftLen;
        var angle    = Math.atan2(0, dir);
        var headAng  = Math.PI / 6;
        ctx.strokeStyle = rgba(acol, 0.80);
        ctx.lineWidth   = 1.5 * this.pointScale;
        ctx.lineCap     = "round";
        ctx.beginPath();
        ctx.moveTo(baseX, arrowY);
        ctx.lineTo(tipX,  arrowY);
        ctx.stroke();
        ctx.fillStyle = rgba(acol, 0.80);
        ctx.beginPath();
        ctx.moveTo(tipX, arrowY);
        ctx.lineTo(tipX - headSize * Math.cos(angle - headAng), arrowY - headSize * Math.sin(angle - headAng));
        ctx.lineTo(tipX - headSize * Math.cos(angle + headAng), arrowY - headSize * Math.sin(angle + headAng));
        ctx.closePath();
        ctx.fill();
      }
    }

    // Vote-share bar at bottom of strip
    if (this.showVoteBar && data.vote_shares && data.vote_shares[frameIdx]) {
      var shares = data.vote_shares[frameIdx];
      var barH   = 8;
      var barY   = lineY + 54;
      var barX   = pad.left;
      var barW   = plotW;
      var xOff   = 0;
      ctx.fillStyle = "rgba(255,255,255,0.55)";
      ctx.fillRect(barX, barY, barW, barH);
      for (var bc = 0; bc < nComp; bc++) {
        var bcol = parseRGBA(data.competitor_colors[bc] || "rgba(0,0,0,0.9)");
        var segW = Math.round(shares[bc] * barW);
        ctx.fillStyle = rgba(bcol, 0.80);
        ctx.fillRect(barX + xOff, barY, segW, barH);
        xOff += segW;
      }
      ctx.strokeStyle = "rgba(255,255,255,0.6)";
      ctx.lineWidth   = 0.5;
      ctx.strokeRect(barX, barY, barW, barH);
    }

    ctx.restore();  // exit clip

    // Round counter (above the number line, centered)
    var last = data.rounds.length - 1;
    ctx.font         = "12px " + FONT;
    ctx.fillStyle    = COLORS.textLight;
    ctx.textAlign    = "center";
    ctx.textBaseline = "alphabetic";
    ctx.fillText(
      data.rounds[frameIdx] + " / " + data.rounds[last],
      pad.left + plotW / 2, lineY - 40
    );

    // Zoom indicator (x-only in 1D)
    if (this._isZoomed()) {
      var zoomLevel = (data.xlim[1] - data.xlim[0]) / (this.viewXMax - this.viewXMin);
      ctx.font         = "10px " + MONO;
      ctx.textAlign    = "right";
      ctx.textBaseline = "top";
      ctx.fillStyle    = COLORS.textLight;
      ctx.fillText(zoomLevel.toFixed(1) + "\u00D7  dbl-click to reset", pad.left + plotW - 4, 4);
    }

    // ── Per-frame summary statistics (right-side panel, below legend) ─────────
    var sOvl1d       = data.overlays_static;
    var show1dCent   = !!(this.overlayToggles["centroid"]        && sOvl1d && sOvl1d["centroid"]);
    var show1dMed    = !!(this.overlayToggles["marginal_median"] && sOvl1d && sOvl1d["marginal_median"]);
    var show1dWs     = !!(this.showWinset && data.winset_intervals_1d);
    if (data.seat_holder_indices || show1dCent || show1dMed || show1dWs) {
      var nComp1d    = data.positions[0] ? data.positions[0].length : 0;
      var seatIdxs1d = (data.seat_holder_indices && data.seat_holder_indices[frameIdx]) || [];
      var sLegX1d    = pad.left + plotW + 18;
      // Position stats block below legend rows (candidates + voters + overlays + IC/WinSet toggles)
      var sLegY01d   = 8;
      var sLh1d      = 22;
      var sRows1d    = nComp1d + 1;  // competitors + voters row
      if (sOvl1d) sRows1d += Object.keys(sOvl1d).length;
      if (data.indifference_curves) sRows1d++;
      if (data.winset_intervals_1d) sRows1d++;
      var statsY1d   = sLegY01d + sRows1d * sLh1d + 10;
      var cw1d       = this.fgCanvas.width / dpr;
      var sLegW1d    = Math.max(60, cw1d - sLegX1d - 4);
      var valRX1d    = sLegX1d + sLegW1d - 4;
      var stH1d      = 18;
      var curY1d     = statsY1d;

      // Separator
      ctx.strokeStyle = "rgba(0,0,0,0.13)";
      ctx.lineWidth   = 1;
      ctx.setLineDash([]);
      ctx.beginPath();
      ctx.moveTo(sLegX1d, curY1d - 5);
      ctx.lineTo(sLegX1d + sLegW1d - 18, curY1d - 5);
      ctx.stroke();

      function drawStat1dHeader(txt, y) {
        ctx.font         = "bold 11px " + FONT;
        ctx.textAlign    = "left";
        ctx.textBaseline = "middle";
        ctx.fillStyle    = COLORS.textLight;
        ctx.fillText(txt, sLegX1d, y);
      }
      function drawStat1dRow(lbl, val, y) {
        ctx.font         = "11px " + FONT;
        ctx.textBaseline = "middle";
        ctx.fillStyle    = COLORS.textLight;
        ctx.textAlign    = "left";
        ctx.fillText(lbl, sLegX1d + 6, y);
        ctx.fillStyle = COLORS.text;
        ctx.textAlign = "right";
        ctx.fillText(val, valRX1d, y);
      }

      // Seat holders — always shown first in the stats panel
      drawStat1dHeader("Seat holders", curY1d); curY1d += stH1d;
      if (seatIdxs1d.length > 0) {
        for (var sh1d = 0; sh1d < seatIdxs1d.length; sh1d++) {
          var shIdx1d = seatIdxs1d[sh1d];
          var shCol1d = parseRGBA(data.competitor_colors[shIdx1d] || "rgba(0,0,0,0.9)");
          var shNm1d  = (data.competitor_names && data.competitor_names[shIdx1d]) || ("Candidate " + (shIdx1d + 1));
          var shIcX1d = sLegX1d + 7;
          var shTs1d  = 5;
          ctx.beginPath();
          ctx.moveTo(shIcX1d,           curY1d + shTs1d);
          ctx.lineTo(shIcX1d - shTs1d,  curY1d - shTs1d * 0.5);
          ctx.lineTo(shIcX1d + shTs1d,  curY1d - shTs1d * 0.5);
          ctx.closePath();
          ctx.fillStyle = rgba(shCol1d, 0.95);
          ctx.fill();
          ctx.font         = "11px " + FONT;
          ctx.fillStyle    = COLORS.text;
          ctx.textAlign    = "left";
          ctx.textBaseline = "middle";
          ctx.fillText(shNm1d, shIcX1d + shTs1d + 5, curY1d);
          curY1d += stH1d;
        }
      } else {
        ctx.font         = "11px " + FONT;
        ctx.fillStyle    = COLORS.textLight;
        ctx.textAlign    = "left";
        ctx.textBaseline = "middle";
        ctx.fillText("\u2014", sLegX1d + 6, curY1d);
        curY1d += stH1d;
      }

      var framePosns1d = data.positions[frameIdx];
      var seatLabel1d  = seatIdxs1d.length === 1 ? "Seat winner avg" : "Seat winners avg";

      if (show1dCent) {
        var cx1d = sOvl1d["centroid"].x;
        var sumC1d = 0;
        for (var ci1 = 0; ci1 < nComp1d; ci1++) sumC1d += Math.abs(framePosns1d[ci1][0] - cx1d);
        var sumCS1d = 0;
        for (var csj1 = 0; csj1 < seatIdxs1d.length; csj1++)
          sumCS1d += Math.abs(framePosns1d[seatIdxs1d[csj1]][0] - cx1d);
        drawStat1dHeader("Centroid dist.", curY1d); curY1d += stH1d;
        drawStat1dRow("All cands avg", (sumC1d / nComp1d).toFixed(3), curY1d); curY1d += stH1d;
        drawStat1dRow(seatLabel1d, seatIdxs1d.length > 0 ? (sumCS1d / seatIdxs1d.length).toFixed(3) : "\u2014", curY1d); curY1d += stH1d;
      }

      if (show1dMed) {
        var mm1d = sOvl1d["marginal_median"].x;
        var sumM1d = 0;
        for (var mi1 = 0; mi1 < nComp1d; mi1++) sumM1d += Math.abs(framePosns1d[mi1][0] - mm1d);
        var sumMS1d = 0;
        for (var msj1 = 0; msj1 < seatIdxs1d.length; msj1++)
          sumMS1d += Math.abs(framePosns1d[seatIdxs1d[msj1]][0] - mm1d);
        drawStat1dHeader("Median dist.", curY1d); curY1d += stH1d;
        drawStat1dRow("All cands avg", (sumM1d / nComp1d).toFixed(3), curY1d); curY1d += stH1d;
        drawStat1dRow(seatLabel1d, seatIdxs1d.length > 0 ? (sumMS1d / seatIdxs1d.length).toFixed(3) : "\u2014", curY1d); curY1d += stH1d;
      }

      if (show1dWs) {
        var wsf1d = data.winset_intervals_1d[frameIdx];
        drawStat1dHeader("Win Set", curY1d); curY1d += stH1d;
        if (wsf1d && wsf1d.length > 0) {
          for (var wsk = 0; wsk < wsf1d.length; wsk++) {
            var wse1d = wsf1d[wsk];
            var wsLen = (!wse1d || isNaN(wse1d.lo) || isNaN(wse1d.hi))
              ? null : Math.abs(wse1d.hi - wse1d.lo);
            var wsLbl = wsf1d.length > 1
              ? ("Cand " + (wse1d.competitor_idx + 1) + " length")
              : "Length";
            drawStat1dRow(wsLbl, wsLen !== null ? wsLen.toFixed(4) : "empty", curY1d);
            curY1d += stH1d;
          }
        } else {
          drawStat1dRow("Length", "empty", curY1d); curY1d += stH1d;
        }
      }
    }

    ctx.restore();
  };

  // ── Animated foreground layer ──────────────────────────────────────────────

  CompetitionCanvas.prototype.draw = function() {
    var data = this.data;
    if (!data || !this.padding) return;
    if (this.mode1d) { this.draw1d(); return; }

    var canvas   = this.fgCanvas;
    var ctx      = canvas.getContext("2d");
    var dpr      = this.dpr;
    var pad      = this.padding;
    var side     = this.plotSide;
    var frameIdx = this.currentFrame;
    var nComp    = data.positions[0] ? data.positions[0].length : 0;
    var trail    = this.trailMode;
    var trailLen = data.trail_length != null ? data.trail_length : 10;

    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.save();
    ctx.scale(dpr, dpr);

    // Clip animated drawing to the plot area so candidates/trails don't
    // bleed into the padding when panned or zoomed out.
    ctx.save();
    ctx.beginPath();
    ctx.rect(pad.left, pad.top, side, side);
    ctx.clip();

    // Round counter.
    var lastIdx = data.rounds.length - 1;
    ctx.fillStyle    = COLORS.textLight;
    ctx.font         = "11px " + MONO;
    ctx.textAlign    = "center";
    ctx.textBaseline = "alphabetic";
    // (drawn outside clip region below)

    // Ghost.
    if (this.showGhost) {
      var ghostIdx = Math.max(0, frameIdx - this.ghostFrames);
      if (ghostIdx !== frameIdx) {
        for (var gc = 0; gc < nComp; gc++) {
          var gpos = data.positions[ghostIdx][gc];
          var gcol = parseRGBA(data.competitor_colors[gc] || "rgba(0,0,0,0.9)");
          this._drawDiamond(ctx, this.xToPx(gpos[0]), this.yToPx(gpos[1]), 4 * this.pointScale, rgba(gcol, 0.20), 0);
        }
      }
    }

    // Trails.
    for (var c = 0; c < nComp; c++) {
      var color = parseRGBA(data.competitor_colors[c] || "rgba(0,0,0,0.9)");

      if (trail === "full" && frameIdx > 0) {
        ctx.strokeStyle = rgba(color, 0.75);
        ctx.lineWidth   = 2;
        ctx.lineCap     = "round";
        ctx.lineJoin    = "round";
        ctx.beginPath();
        for (var f = 0; f <= frameIdx; f++) {
          var pt = data.positions[f][c];
          if (f === 0) ctx.moveTo(this.xToPx(pt[0]), this.yToPx(pt[1]));
          else         ctx.lineTo(this.xToPx(pt[0]), this.yToPx(pt[1]));
        }
        ctx.stroke();

      } else if (trail === "fade" && frameIdx > 0) {
        var start = Math.max(0, frameIdx - trailLen);
        ctx.lineCap   = "round";
        ctx.lineJoin  = "round";
        ctx.lineWidth = 2;
        for (var s = start; s < frameIdx; s++) {
          var age   = frameIdx - s;
          var alpha = 0.80 * Math.pow(0.62, age - 1);
          if (alpha < 0.01) continue;
          ctx.strokeStyle = rgba(color, alpha);
          ctx.beginPath();
          var s0 = data.positions[s][c];
          var s1 = data.positions[s + 1][c];
          ctx.moveTo(this.xToPx(s0[0]), this.yToPx(s0[1]));
          ctx.lineTo(this.xToPx(s1[0]), this.yToPx(s1[1]));
          ctx.stroke();
        }
      }
    }

    // Indifference curves — drawn per voter through each seat holder's position.
    if (this.showIC && data.indifference_curves) {
      var icFrames = data.indifference_curves[frameIdx];
      var icIdxs   = data.ic_competitor_indices ? data.ic_competitor_indices[frameIdx] : null;
      if (icFrames) {
        ctx.lineWidth = 1.5;
        ctx.lineCap   = "round";
        ctx.lineJoin  = "round";
        for (var si = 0; si < icFrames.length; si++) {
          var compIdx = icIdxs ? icIdxs[si] : si;
          var icColor = parseRGBA(data.competitor_colors[compIdx] || "rgba(128,128,128,0.9)");
          ctx.strokeStyle = rgba(icColor, 0.20);
          var voterCurves = icFrames[si];
          for (var vi = 0; vi < voterCurves.length; vi++) {
            var verts = voterCurves[vi];
            if (!verts || verts.length < 4) continue;
            ctx.beginPath();
            ctx.moveTo(this.xToPx(verts[0]), this.yToPx(verts[1]));
            for (var ki = 2; ki < verts.length; ki += 2) {
              ctx.lineTo(this.xToPx(verts[ki]), this.yToPx(verts[ki + 1]));
            }
            ctx.closePath();
            ctx.stroke();
          }
        }
      }
    }

    // Voronoi cells — drawn per competitor for the current frame (Euclidean candidate regions).
    // Same style as WinSet: fill with competitor colour at low alpha, dashed stroke.
    if (this.showVoronoi && data.voronoi_cells) {
      var voFrame = data.voronoi_cells[frameIdx];
      if (voFrame) {
        ctx.lineWidth = 2.5;
        ctx.setLineDash([6, 4]);
        ctx.lineCap  = "round";
        ctx.lineJoin = "round";
        for (var vci = 0; vci < voFrame.length; vci++) {
          var vc = voFrame[vci];
          if (!vc || !vc.paths) continue;
          var vcColor = parseRGBA(data.competitor_colors[vc.competitor_idx] || "rgba(128,128,128,0.9)");
          ctx.fillStyle   = rgba(vcColor, 0.10);
          ctx.strokeStyle = rgba(vcColor, 0.70);
          ctx.beginPath();
          for (var vpi = 0; vpi < vc.paths.length; vpi++) {
            var vverts = vc.paths[vpi];
            if (!vverts || vverts.length < 4) continue;
            ctx.moveTo(this.xToPx(vverts[0]), this.yToPx(vverts[1]));
            for (var vki = 2; vki < vverts.length; vki += 2) {
              ctx.lineTo(this.xToPx(vverts[vki]), this.yToPx(vverts[vki + 1]));
            }
            ctx.closePath();
          }
          ctx.fill();
          ctx.stroke();
        }
        ctx.setLineDash([]);
      }
    }

    // WinSet boundaries — drawn per seat holder for the current frame.
    // Filled with the seat holder's colour at low alpha; dashed border at higher alpha.
    // Holes are handled via the evenodd fill rule so multi-component winsets render correctly.
    if (this.showWinset && data.winsets) {
      var wsFrame = data.winsets[frameIdx];
      if (wsFrame) {
        ctx.lineWidth = 2.5;
        ctx.setLineDash([6, 4]);
        ctx.lineCap  = "round";
        ctx.lineJoin = "round";
        for (var wsi = 0; wsi < wsFrame.length; wsi++) {
          var ws = wsFrame[wsi];
          if (!ws) continue;
          var wsColor = parseRGBA(data.competitor_colors[ws.competitor_idx] || "rgba(128,128,128,0.9)");
          ctx.fillStyle   = rgba(wsColor, 0.10);
          ctx.strokeStyle = rgba(wsColor, 0.70);
          ctx.beginPath();
          for (var wpi = 0; wpi < ws.paths.length; wpi++) {
            var wverts = ws.paths[wpi];
            if (!wverts || wverts.length < 4) continue;
            ctx.moveTo(this.xToPx(wverts[0]), this.yToPx(wverts[1]));
            for (var wki = 2; wki < wverts.length; wki += 2) {
              ctx.lineTo(this.xToPx(wverts[wki]), this.yToPx(wverts[wki + 1]));
            }
            ctx.closePath();
          }
          ctx.fill("evenodd");
          ctx.stroke();
        }
        ctx.setLineDash([]);
      }
    }

    // Candidate markers (size 5 normal / 10 doubled — map-style zoom).
    var candSize = 5 * this.pointScale;
    for (var cc = 0; cc < nComp; cc++) {
      var ccolor = parseRGBA(data.competitor_colors[cc] || "rgba(0,0,0,0.9)");
      var pos    = data.positions[frameIdx][cc];
      this._drawDiamond(ctx, this.xToPx(pos[0]), this.yToPx(pos[1]), candSize, rgba(ccolor, 0.95), 7);
    }

    // Heading / movement arrows.
    if (this.showArrows && frameIdx > 0) {
      var xScale = side / (this.viewXMax - this.viewXMin);
      var yScale = side / (this.viewYMax - this.viewYMin);
      for (var ac = 0; ac < nComp; ac++) {
        var acol  = parseRGBA(data.competitor_colors[ac] || "rgba(0,0,0,0.9)");
        var prev  = data.positions[frameIdx - 1][ac];
        var curr  = data.positions[frameIdx][ac];
        var dpx   = (curr[0] - prev[0]) * xScale;
        var dpy   = -(curr[1] - prev[1]) * yScale;
        var len   = Math.sqrt(dpx * dpx + dpy * dpy);
        if (len < 0.5) continue;
        var nx      = dpx / len;
        var ny      = dpy / len;
        var baseX    = this.xToPx(curr[0]);
        var baseY    = this.yToPx(curr[1]);
        var shaftLen = 10 * this.pointScale;
        var headSize = 6  * this.pointScale;
        var tipX     = baseX + nx * shaftLen;
        var tipY     = baseY + ny * shaftLen;
        var angle    = Math.atan2(ny, nx);
        var headAng  = Math.PI / 6;

        ctx.strokeStyle = rgba(acol, 0.80);
        ctx.lineWidth   = 1.5 * this.pointScale;
        ctx.lineCap     = "round";
        ctx.beginPath();
        ctx.moveTo(baseX, baseY);
        ctx.lineTo(tipX, tipY);
        ctx.stroke();

        ctx.fillStyle = rgba(acol, 0.80);
        ctx.beginPath();
        ctx.moveTo(tipX, tipY);
        ctx.lineTo(tipX - headSize * Math.cos(angle - headAng), tipY - headSize * Math.sin(angle - headAng));
        ctx.lineTo(tipX - headSize * Math.cos(angle + headAng), tipY - headSize * Math.sin(angle + headAng));
        ctx.closePath();
        ctx.fill();
      }
    }

    // Vote-share bar.
    if (this.showVoteBar && data.vote_shares && data.vote_shares[frameIdx]) {
      var shares = data.vote_shares[frameIdx];
      var barH   = 10;
      var barY   = pad.top + side - barH - 1;
      var barX   = pad.left;
      var barW   = side;
      var xOff   = 0;

      ctx.fillStyle = "rgba(255,255,255,0.55)";
      ctx.fillRect(barX, barY, barW, barH);

      for (var bc = 0; bc < nComp; bc++) {
        var bcol = parseRGBA(data.competitor_colors[bc] || "rgba(0,0,0,0.9)");
        var segW = Math.round(shares[bc] * barW);
        ctx.fillStyle = rgba(bcol, 0.80);
        ctx.fillRect(barX + xOff, barY, segW, barH);
        xOff += segW;
      }

      ctx.strokeStyle = "rgba(255,255,255,0.6)";
      ctx.lineWidth   = 0.5;
      ctx.strokeRect(barX, barY, barW, barH);
    }

    ctx.restore();  // end clip region

    // Round counter and zoom indicator (outside clip, above plot).
    ctx.fillStyle    = COLORS.textLight;
    ctx.font         = "11px " + MONO;
    ctx.textAlign    = "center";
    ctx.textBaseline = "alphabetic";
    ctx.fillText(
      data.rounds[frameIdx] + "  /  " + data.rounds[lastIdx],
      pad.left + side / 2,
      pad.top - 14
    );

    if (this._isZoomed()) {
      var zoomLevelX = (data.xlim[1] - data.xlim[0]) / (this.viewXMax - this.viewXMin);
      var zoomLevelY = (data.ylim[1] - data.ylim[0]) / (this.viewYMax - this.viewYMin);
      var zoomLevel  = Math.max(zoomLevelX, zoomLevelY);
      ctx.font         = "10px " + MONO;
      ctx.textAlign    = "right";
      ctx.textBaseline = "top";
      ctx.fillText(zoomLevel.toFixed(1) + "\u00D7  dbl-click to reset", pad.left + side - 4, pad.top + 4);
    }

    // ── Per-frame summary statistics (right-side panel, below legend) ─────────
    // Determine which stat sections are active.
    var sOvl             = data.overlays_static;
    var showCentStat     = !!(this.overlayToggles["centroid"]        && sOvl && sOvl["centroid"]);
    var showMedStat      = !!(this.overlayToggles["marginal_median"] && sOvl && sOvl["marginal_median"]);
    var showWinsetStat   = !!(this.showWinset && data.winsets);
    var hasSeatHolders2d = !!(data.seat_holder_indices && data.seat_holder_indices.length > 0);
    var statsHaveContent = hasSeatHolders2d || showCentStat || showMedStat || showWinsetStat;

    if (statsHaveContent) {
      // Compute statsY0 by mirroring the legend row count from drawBackground().
      var sLegY0 = pad.top + 8;
      var sLineH = 22;
      var sRows  = nComp + 2;   // n competitors + voters row + seats row
      if (sOvl) {
        var sOvlKeys = Object.keys(sOvl);
        for (var sk = 0; sk < sOvlKeys.length; sk++) {
          if (this.overlayToggles[sOvlKeys[sk]]) sRows++;
        }
      }
      if (this.showIC     && data.indifference_curves) sRows++;
      if (this.showWinset && data.winsets)             sRows++;
      if (this.showVoronoi && data.voronoi_cells)      sRows++;
      var statsY0  = sLegY0 + sRows * sLineH + 10;
      var canvasH  = this.fgCanvas.height / dpr;
      if (statsY0 < canvasH - 20) {
        var sLegX     = pad.left + side + 18;
        var sLegW     = Math.max(60, this.width - pad.left - side - 14);
        var valRightX = pad.left + side + sLegW - 4;
        var stLineH   = 18;
        var curY      = statsY0;

        // Separator line
        ctx.strokeStyle = "rgba(0,0,0,0.13)";
        ctx.lineWidth   = 1;
        ctx.setLineDash([]);
        ctx.beginPath();
        ctx.moveTo(sLegX, curY - 5);
        ctx.lineTo(sLegX + sLegW - 18, curY - 5);
        ctx.stroke();

        // Euclidean distance. Note: uses Euclidean regardless of the
        // competition's configured metric — this is a display heuristic.
        function stDist(ax, ay, bx, by) {
          var dx = ax - bx, dy = ay - by;
          return Math.sqrt(dx * dx + dy * dy);
        }

        function drawStatHeader(txt, y) {
          ctx.font         = "bold 11px " + FONT;
          ctx.textAlign    = "left";
          ctx.textBaseline = "middle";
          ctx.fillStyle    = COLORS.textLight;
          ctx.fillText(txt, sLegX, y);
        }

        function drawStatRow(lbl, val, y) {
          ctx.font         = "11px " + FONT;
          ctx.textBaseline = "middle";
          ctx.fillStyle    = COLORS.textLight;
          ctx.textAlign    = "left";
          ctx.fillText(lbl, sLegX + 6, y);
          ctx.fillStyle = COLORS.text;
          ctx.textAlign = "right";
          ctx.fillText(val, valRightX, y);
        }

        var framePosns = data.positions[frameIdx];
        var seatIdxs   = (data.seat_holder_indices && data.seat_holder_indices[frameIdx]) || [];
        var seatLabel  = seatIdxs.length === 1 ? "Seat winner avg" : "Seat winners avg";

        // Seat holders — always shown first in the stats panel
        drawStatHeader("Seat holders", curY); curY += stLineH;
        if (seatIdxs.length > 0) {
          for (var sh2d = 0; sh2d < seatIdxs.length; sh2d++) {
            var shIdx2d = seatIdxs[sh2d];
            var shCol2d = parseRGBA(data.competitor_colors[shIdx2d] || "rgba(0,0,0,0.9)");
            var shNm2d  = (data.competitor_names && data.competitor_names[shIdx2d]) || ("Candidate " + (shIdx2d + 1));
            // Diamond icon (matches background legend symbol)
            var shIcX2d = sLegX + 7;
            var shR2d   = 5;
            ctx.beginPath();
            ctx.moveTo(shIcX2d,         curY - shR2d);
            ctx.lineTo(shIcX2d + shR2d, curY);
            ctx.lineTo(shIcX2d,         curY + shR2d);
            ctx.lineTo(shIcX2d - shR2d, curY);
            ctx.closePath();
            ctx.fillStyle = rgba(shCol2d, 0.95);
            ctx.fill();
            ctx.font         = "11px " + FONT;
            ctx.fillStyle    = COLORS.text;
            ctx.textAlign    = "left";
            ctx.textBaseline = "middle";
            ctx.fillText(shNm2d, shIcX2d + shR2d + 5, curY);
            curY += stLineH;
          }
        } else {
          ctx.font         = "11px " + FONT;
          ctx.fillStyle    = COLORS.textLight;
          ctx.textAlign    = "left";
          ctx.textBaseline = "middle";
          ctx.fillText("\u2014", sLegX + 6, curY);
          curY += stLineH;
        }

        // Centroid distance stats
        if (showCentStat) {
          var cx = sOvl["centroid"].x, cy = sOvl["centroid"].y;
          var sumC = 0;
          for (var csi = 0; csi < nComp; csi++) {
            sumC += stDist(framePosns[csi][0], framePosns[csi][1], cx, cy);
          }
          var sumCS = 0;
          for (var csj = 0; csj < seatIdxs.length; csj++) {
            var csp = framePosns[seatIdxs[csj]];
            sumCS += stDist(csp[0], csp[1], cx, cy);
          }
          drawStatHeader("Centroid dist.", curY); curY += stLineH;
          drawStatRow("All cands avg", (sumC / nComp).toFixed(3), curY); curY += stLineH;
          drawStatRow(seatLabel, seatIdxs.length > 0 ? (sumCS / seatIdxs.length).toFixed(3) : "\u2014", curY); curY += stLineH;
        }

        // Marginal median distance stats
        if (showMedStat) {
          var mmx = sOvl["marginal_median"].x, mmy = sOvl["marginal_median"].y;
          var sumM = 0;
          for (var msi = 0; msi < nComp; msi++) {
            sumM += stDist(framePosns[msi][0], framePosns[msi][1], mmx, mmy);
          }
          var sumMS = 0;
          for (var msj = 0; msj < seatIdxs.length; msj++) {
            var msp = framePosns[seatIdxs[msj]];
            sumMS += stDist(msp[0], msp[1], mmx, mmy);
          }
          drawStatHeader("Median dist.", curY); curY += stLineH;
          drawStatRow("All cands avg", (sumM / nComp).toFixed(3), curY); curY += stLineH;
          drawStatRow(seatLabel, seatIdxs.length > 0 ? (sumMS / seatIdxs.length).toFixed(3) : "\u2014", curY); curY += stLineH;
        }

        // WinSet area (shoelace formula; holes subtract)
        if (showWinsetStat) {
          var wsFrameStat = data.winsets[frameIdx];
          var totalArea   = 0;
          var anyNonNull  = false;
          if (wsFrameStat) {
            for (var wsi3 = 0; wsi3 < wsFrameStat.length; wsi3++) {
              var wse = wsFrameStat[wsi3];
              if (!wse) continue;
              anyNonNull = true;
              for (var wp3 = 0; wp3 < wse.paths.length; wp3++) {
                var wv = wse.paths[wp3];
                var nv = wv.length / 2;
                var pa = 0;
                for (var pv = 0; pv < nv; pv++) {
                  var nxt = (pv + 1) % nv;
                  pa += wv[pv * 2] * wv[nxt * 2 + 1];
                  pa -= wv[nxt * 2] * wv[pv * 2 + 1];
                }
                pa = Math.abs(pa) * 0.5;
                totalArea += wse.is_hole[wp3] ? -pa : pa;
              }
            }
          }
          drawStatHeader("Win Set", curY); curY += stLineH;
          drawStatRow("Area", anyNonNull ? totalArea.toFixed(4) : "empty", curY); curY += stLineH;
        }

      }
    }

    ctx.restore();
  };

  // ── Drawing primitives ─────────────────────────────────────────────────────

  CompetitionCanvas.prototype._drawDiamond = function(ctx, px, py, size, fill, shadow) {
    ctx.beginPath();
    ctx.moveTo(px,        py - size);
    ctx.lineTo(px + size, py);
    ctx.lineTo(px,        py + size);
    ctx.lineTo(px - size, py);
    ctx.closePath();
    if (shadow) {
      ctx.shadowColor   = "rgba(0,0,0,0.30)";
      ctx.shadowBlur    = shadow;
      ctx.shadowOffsetY = 1;
    }
    ctx.fillStyle = fill;
    ctx.fill();
    ctx.shadowColor   = "rgba(0,0,0,0)";
    ctx.shadowBlur    = 0;
    ctx.shadowOffsetY = 0;
    ctx.strokeStyle   = "rgba(255,255,255,0.92)";
    ctx.lineWidth     = 1.5;
    ctx.stroke();
  };

  // ── HTMLWidgets registration ───────────────────────────────────────────────

  if (typeof HTMLWidgets !== "undefined") {
    HTMLWidgets.widget({
      name: "competition_canvas",
      type: "output",
      factory: function(el, width, height) {
        var widget = new CompetitionCanvas(el, width, height);
        return {
          renderValue: function(x) { if (x != null) widget.setData(x); },
          resize:      function(w, h) { /* fixed-size widget: no-op */ }
        };
      }
    });
  }
})();
