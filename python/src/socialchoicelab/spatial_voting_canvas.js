/**
 * Static 2D spatial voting plot (HTML5 Canvas 2D).
 * Depends on scs_canvas_core.js. No Plotly.
 */
(function() {
  "use strict";
  if (typeof ScsCanvasCore === "undefined") {
    throw new Error("spatial_voting_canvas.js requires scs_canvas_core.js first.");
  }
  var C = ScsCanvasCore;
  var FONT = C.FONT;
  var MONO = C.MONO;

  function defaultThemeColors() {
    return {
      plot_bg: "#fafafa",
      grid: "rgba(140,140,140,0.18)",
      axis: "rgba(100,100,100,0.45)",
      text: "#2d2d2d",
      text_light: "#888",
      border: "#d4d4d4",
      voter_fill: "rgba(100,100,200,0.45)",
      voter_stroke: "rgba(255,255,255,0.65)",
      alt_fill: "rgba(200,100,100,0.85)",
      sq_fill: "rgba(255,200,0,0.95)",
      winset_fill: "rgba(0,100,200,0.12)",
      winset_line: "rgba(0,100,200,0.55)",
      hull_fill: "rgba(120,0,220,0.08)",
      hull_line: "rgba(120,0,220,0.50)",
      ic_line: "rgba(100,100,100,0.45)",
      pref_fill: "rgba(100,100,100,0.06)",
      pref_line: "rgba(100,100,100,0.45)",
      yolk_fill: "rgba(255,165,0,0.12)",
      yolk_line: "rgba(255,140,0,0.75)",
      uncovered_fill: "rgba(0,150,100,0.10)",
      uncovered_line: "rgba(0,120,80,0.55)"
    };
  }

  function mergeColors(base, fromPayload) {
    if (!fromPayload || typeof fromPayload !== "object") return base;
    var o = {};
    for (var k in base) o[k] = base[k];
    for (var k2 in fromPayload) if (fromPayload[k2] != null) o[k2] = fromPayload[k2];
    return o;
  }

  /** Checkbox row helper (same pattern as competition_canvas). */
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
      "user-select:none",
      "margin:2px 12px 2px 0"
    ].join(";");
    var cb = document.createElement("input");
    cb.type = "checkbox";
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

  var UIC = C.COLORS;

  /** Layer entry: flat numeric xy array, or { xy, fill?, line? } from R/Python. */
  function specXY(spec) {
    if (spec == null) return null;
    if (typeof spec === "object" && spec.xy) return spec.xy;
    return spec;
  }

  function hasFillColor(fill) {
    if (fill == null || fill === "" || fill === "rgba(0,0,0,0)" || fill === "transparent") return false;
    return true;
  }

  /** Regular 5-point star centred at (cx, cy); outerR / innerR in pixels. */
  function drawStar5Path(ctx, cx, cy, outerR, innerR) {
    var n = 5;
    ctx.beginPath();
    for (var i = 0; i < n * 2; i++) {
      var r = (i % 2 === 0) ? outerR : innerR;
      var a = -Math.PI / 2 + (i * Math.PI / n);
      var x = cx + Math.cos(a) * r;
      var y = cy + Math.sin(a) * r;
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.closePath();
  }

  function strokeClosedPoly(ctx, flatXY) {
    if (!flatXY || flatXY.length < 4) return;
    ctx.beginPath();
    ctx.moveTo(flatXY[0], flatXY[1]);
    for (var i = 2; i < flatXY.length; i += 2) {
      ctx.lineTo(flatXY[i], flatXY[i + 1]);
    }
    ctx.closePath();
  }

  function SpatialVotingCanvas(el, width, height) {
    var self = this;
    this.el = el;
    this.width = width || 700;
    this.height = height || 600;
    this.data = null;
    this.dpr = window.devicePixelRatio || 1;
    this.padding = null;
    this.plotSide = 0;
    this.viewXMin = 0;
    this.viewXMax = 1;
    this.viewYMin = 0;
    this.viewYMax = 1;
    this._fullView = null;
    this._dragging = false;
    this.visibility = {};
    this.pointScale = 1;
    this.showHeatmap = false;
    this.heatmapCanvas = null;
    this.controlsH = 92;

    var container = document.createElement("div");
    container.style.cssText = [
      "position:relative",
      "display:flex",
      "flex-direction:column",
      "width:" + this.width + "px",
      "height:" + this.height + "px",
      "background:#fff",
      "font-family:" + FONT,
      "box-sizing:border-box",
      "overflow:hidden"
    ].join(";");
    this.container = container;

    var canvas = document.createElement("canvas");
    canvas.style.cssText = "display:block;outline:none;cursor:grab;";
    canvas.tabIndex = 0;
    this.canvas = canvas;
    container.appendChild(canvas);

    var controlsOuter = document.createElement("div");
    controlsOuter.style.cssText = [
      "flex-shrink:0",
      "background:" + UIC.controls,
      "border-top:1px solid " + UIC.controlBorder,
      "display:flex",
      "flex-direction:column",
      "box-sizing:border-box",
      "pointer-events:auto",
      "z-index:4"
    ].join(";");
    this.controlsOuter = controlsOuter;

    var toolsRow = document.createElement("div");
    toolsRow.className = "scs-spatial-tools-row";
    toolsRow.setAttribute("aria-label", "Zoom and display tools");
    toolsRow.style.cssText = [
      "display:flex",
      "align-items:center",
      "gap:8px",
      "flex-wrap:wrap",
      "padding:6px 14px 4px"
    ].join(";");
    this.toolsRow = toolsRow;

    var zoomBtnStyle = [
      "cursor:pointer",
      "background:#fff",
      "color:" + UIC.text,
      "border:1px solid " + UIC.controlBorder,
      "border-radius:4px",
      "padding:2px 7px",
      "font-size:13px",
      "line-height:1",
      "flex-shrink:0",
      "transition:background 0.1s",
      "font-family:" + FONT
    ].join(";");

    function makeZoomBtn(label, title, onClick) {
      var btn = document.createElement("button");
      btn.type = "button";
      btn.textContent = label;
      btn.title = title;
      btn.style.cssText = zoomBtnStyle;
      btn.addEventListener("mouseover", function() { this.style.background = "#eef"; });
      btn.addEventListener("mouseout", function() { this.style.background = "#fff"; });
      btn.addEventListener("click", onClick);
      return btn;
    }

    var zoomSep = document.createElement("span");
    zoomSep.style.cssText = "width:1px;height:16px;background:" + UIC.controlBorder + ";flex-shrink:0;margin:0 2px;";
    var pointSep = document.createElement("span");
    pointSep.style.cssText = zoomSep.style.cssText;

    var zoomOutBtn = makeZoomBtn("\u2212", "Zoom out", function() { self._zoomBy(1 / 1.5); });
    var zoomInBtn = makeZoomBtn("+", "Zoom in", function() { self._zoomBy(1.5); });
    var zoomResetBtn = makeZoomBtn("\u21BA", "Reset zoom", function() {
      if (self.data) {
        self._initView();
        self.draw();
      }
    });

    var pointBtn = document.createElement("button");
    pointBtn.type = "button";
    pointBtn.textContent = "Large Points";
    pointBtn.title = "Toggle large point sizes";
    pointBtn.style.cssText = zoomBtnStyle;
    pointBtn.addEventListener("mouseover", function() { this.style.background = "#eef"; });
    pointBtn.addEventListener("mouseout", function() {
      this.style.background = self.pointScale === 2 ? "#dde" : "#fff";
    });
    pointBtn.addEventListener("click", function() {
      self.pointScale = self.pointScale === 1 ? 2 : 1;
      pointBtn.style.background = self.pointScale === 2 ? "#dde" : "#fff";
      pointBtn.style.fontWeight = self.pointScale === 2 ? "700" : "normal";
      self.draw();
    });
    this.pointBtn = pointBtn;

    toolsRow.appendChild(zoomSep);
    toolsRow.appendChild(zoomOutBtn);
    toolsRow.appendChild(zoomInBtn);
    toolsRow.appendChild(zoomResetBtn);
    toolsRow.appendChild(pointSep);
    toolsRow.appendChild(pointBtn);
    controlsOuter.appendChild(toolsRow);

    var showRow = document.createElement("div");
    showRow.className = "scs-spatial-show-row";
    showRow.setAttribute("aria-label", "Display options");
    showRow.style.cssText = [
      "display:none",
      "flex-wrap:wrap",
      "align-items:center",
      "gap:10px",
      "padding:2px 14px 6px",
      "border-top:1px solid rgba(0,0,0,0.06)",
      "box-sizing:border-box",
      "font-family:" + FONT
    ].join(";");
    this.showRow = showRow;
    controlsOuter.appendChild(showRow);

    var overlayRow = document.createElement("div");
    overlayRow.className = "scs-spatial-overlay-row";
    overlayRow.setAttribute("aria-label", "Layer overlays");
    overlayRow.style.cssText = [
      "display:none",
      "align-items:center",
      "flex-wrap:wrap",
      "gap:10px",
      "padding-top:0",
      "padding-right:10px",
      "padding-bottom:6px",
      "padding-left:0",
      "min-height:28px",
      "font-size:12px",
      "font-family:" + FONT,
      "border-top:1px solid rgba(0,0,0,0.06)",
      "box-sizing:border-box"
    ].join(";");
    this.overlayRow = overlayRow;
    controlsOuter.appendChild(overlayRow);
    container.appendChild(controlsOuter);

    var tip = document.createElement("div");
    tip.style.cssText = [
      "position:fixed",
      "display:none",
      "pointer-events:none",
      "z-index:10000",
      "background:rgba(20,20,20,0.92)",
      "color:#fff",
      "font-size:12px",
      "padding:4px 8px",
      "border-radius:4px",
      "max-width:280px",
      "white-space:pre-wrap",
      "font-family:" + FONT
    ].join(";");
    this.tooltip = tip;
    document.body.appendChild(tip);

    el.appendChild(container);

    this._wireInteractions();
    if (typeof ResizeObserver !== "undefined") {
      new ResizeObserver(function() {
        if (self.data) self.resize();
      }).observe(el);
    }
  }

  SpatialVotingCanvas.prototype._initView = function() {
    var d = this.data;
    var xRange = d.xlim[1] - d.xlim[0];
    var yRange = d.ylim[1] - d.ylim[0];
    var cx = (d.xlim[0] + d.xlim[1]) / 2;
    var cy = (d.ylim[0] + d.ylim[1]) / 2;
    var half = Math.max(xRange, yRange) / 2;
    this.viewXMin = cx - half;
    this.viewXMax = cx + half;
    this.viewYMin = cy - half;
    this.viewYMax = cy + half;
    this._fullView = {
      x0: this.viewXMin, x1: this.viewXMax,
      y0: this.viewYMin, y1: this.viewYMax
    };
  };

  SpatialVotingCanvas.prototype._zoomBy = function(factor) {
    if (!this.data) return;
    var cx = (this.viewXMin + this.viewXMax) / 2;
    var cy = (this.viewYMin + this.viewYMax) / 2;
    var nx = (this.viewXMax - this.viewXMin) / factor;
    var ny = (this.viewYMax - this.viewYMin) / factor;
    this.viewXMin = cx - nx / 2;
    this.viewXMax = cx + nx / 2;
    this.viewYMin = cy - ny / 2;
    this.viewYMax = cy + ny / 2;
    this.draw();
  };

  /** Gaussian KDE heatmap in full data bounds (same kernel as competition_canvas). */
  SpatialVotingCanvas.prototype.computeHeatmap = function() {
    var d = this.data;
    if (!d || !this.padding || this.plotSide <= 0) return;
    var vx = d.voters_x || [];
    var vy = d.voters_y || [];
    if (vx.length === 0) {
      this.heatmapCanvas = null;
      return;
    }
    var kdeRes = Math.min(Math.round(this.plotSide), 160);
    var xRange = d.xlim[1] - d.xlim[0];
    var yRange = d.ylim[1] - d.ylim[0];
    if (xRange <= 0 || yRange <= 0) {
      this.heatmapCanvas = null;
      return;
    }
    var h = 0.14 * Math.max(xRange, yRange);
    var h2inv = 1 / (2 * h * h);
    var n = vx.length;

    var density = new Float32Array(kdeRes * kdeRes);
    var maxD = 0;
    for (var py = 0; py < kdeRes; py++) {
      var yd = d.ylim[0] + (1 - (py + 0.5) / kdeRes) * yRange;
      for (var px = 0; px < kdeRes; px++) {
        var xd = d.xlim[0] + (px + 0.5) / kdeRes * xRange;
        var sum = 0;
        for (var vi = 0; vi < n; vi++) {
          var dx = xd - vx[vi];
          var dy = yd - vy[vi];
          sum += Math.exp(-(dx * dx + dy * dy) * h2inv);
        }
        density[py * kdeRes + px] = sum;
        if (sum > maxD) maxD = sum;
      }
    }

    var imgData = new ImageData(kdeRes, kdeRes);
    var pix = imgData.data;
    for (var j = 0; j < kdeRes * kdeRes; j++) {
      var t = maxD > 0 ? density[j] / maxD : 0;
      var t2 = t * t;
      pix[j * 4] = 255;
      pix[j * 4 + 1] = Math.round(210 - t * 100);
      pix[j * 4 + 2] = Math.round(30 * (1 - t));
      pix[j * 4 + 3] = Math.round(t2 * 55);
    }

    var tmp = document.createElement("canvas");
    tmp.width = kdeRes;
    tmp.height = kdeRes;
    tmp.getContext("2d").putImageData(imgData, 0, 0);
    this.heatmapCanvas = tmp;
  };

  SpatialVotingCanvas.prototype.xToPx = function(x) {
    return C.xToPx2d(x, this.viewXMin, this.viewXMax, this.padding.left, this.plotSide);
  };
  SpatialVotingCanvas.prototype.yToPx = function(y) {
    return C.yToPx2d(y, this.viewYMin, this.viewYMax, this.padding.top, this.plotSide);
  };
  SpatialVotingCanvas.prototype.pxToX = function(px) {
    return C.pxToX2d(px, this.viewXMin, this.viewXMax, this.padding.left, this.plotSide);
  };
  SpatialVotingCanvas.prototype.pxToY = function(py) {
    return C.pxToY2d(py, this.viewYMin, this.viewYMax, this.padding.top, this.plotSide);
  };

  SpatialVotingCanvas.prototype._overPlot = function(clientX, clientY) {
    if (!this.padding) return false;
    var rect = this.canvas.getBoundingClientRect();
    var mx = clientX - rect.left;
    var my = clientY - rect.top;
    return mx >= this.padding.left && mx <= this.padding.left + this.plotSide &&
           my >= this.padding.top && my <= this.padding.top + this.plotSide;
  };

  SpatialVotingCanvas.prototype._wireInteractions = function() {
    var self = this;
    var cv = this.canvas;
    var dragCX = 0, dragCY = 0;
    var vX0, vX1, vY0, vY1;

    cv.addEventListener("wheel", function(e) {
      if (!self.data || !self._overPlot(e.clientX, e.clientY)) return;
      e.preventDefault();
      var factor = e.deltaY < 0 ? 1.15 : 1 / 1.15;
      var rect = cv.getBoundingClientRect();
      var mx = e.clientX - rect.left;
      var my = e.clientY - rect.top;
      var dataX = self.pxToX(mx);
      var dataY = self.pxToY(my);
      var xRange = self.viewXMax - self.viewXMin;
      var yRange = self.viewYMax - self.viewYMin;
      var nx = xRange / factor;
      var ny = yRange / factor;
      var tx = (dataX - self.viewXMin) / xRange;
      var ty = (self.viewYMax - dataY) / yRange;
      self.viewXMin = dataX - tx * nx;
      self.viewXMax = dataX + (1 - tx) * nx;
      self.viewYMin = dataY - (1 - ty) * ny;
      self.viewYMax = dataY + ty * ny;
      self.draw();
    }, { passive: false });

    cv.addEventListener("mousedown", function(e) {
      if (!self.data || !self._overPlot(e.clientX, e.clientY)) return;
      self._dragging = true;
      dragCX = e.clientX;
      dragCY = e.clientY;
      vX0 = self.viewXMin; vX1 = self.viewXMax;
      vY0 = self.viewYMin; vY1 = self.viewYMax;
      cv.style.cursor = "grabbing";
      e.preventDefault();
    });

    document.addEventListener("mousemove", function(e) {
      if (self._dragging && self.padding) {
        var dx = (e.clientX - dragCX) / self.plotSide * (vX1 - vX0);
        var dy = (e.clientY - dragCY) / self.plotSide * (vY1 - vY0);
        self.viewXMin = vX0 - dx;
        self.viewXMax = vX1 - dx;
        self.viewYMin = vY0 + dy;
        self.viewYMax = vY1 + dy;
        self.draw();
        return;
      }
      if (!self.data || !self.padding) return;
      self._updateTooltip(e.clientX, e.clientY);
      cv.style.cursor = self._overPlot(e.clientX, e.clientY) ? "grab" : "default";
    });

    document.addEventListener("mouseup", function() {
      if (self._dragging) {
        self._dragging = false;
        cv.style.cursor = "grab";
      }
    });

    cv.addEventListener("dblclick", function(e) {
      if (!self.data || !self._overPlot(e.clientX, e.clientY) || !self._fullView) return;
      self.viewXMin = self._fullView.x0;
      self.viewXMax = self._fullView.x1;
      self.viewYMin = self._fullView.y0;
      self.viewYMax = self._fullView.y1;
      self.draw();
    });

    cv.addEventListener("mouseleave", function() {
      self.tooltip.style.display = "none";
    });
  };

  SpatialVotingCanvas.prototype._dataXYToScreenFlat = function(flat) {
    var out = [];
    if (!flat) return out;
    for (var i = 0; i + 1 < flat.length; i += 2) {
      out.push(this.xToPx(flat[i]));
      out.push(this.yToPx(flat[i + 1]));
    }
    return out;
  };

  SpatialVotingCanvas.prototype._updateTooltip = function(clientX, clientY) {
    var self = this;
    var d = this.data;
    var tip = this.tooltip;
    if (!d || !this.padding || !this._overPlot(clientX, clientY)) {
      tip.style.display = "none";
      return;
    }
    var rect = this.canvas.getBoundingClientRect();
    var mx = clientX - rect.left;
    var my = clientY - rect.top;
    var hitR = 10;
    var msg = null;

    var vx = d.voters_x || [];
    var vy = d.voters_y || [];
    var names = d.voter_names || [];
    if (this._vis("voters")) {
      for (var vi = 0; vi < vx.length; vi++) {
        var px = this.xToPx(vx[vi]);
        var py = this.yToPx(vy[vi]);
        if (Math.hypot(mx - px, my - py) < hitR) {
          msg = (names[vi] || ("V" + (vi + 1))) + "\n(" + C.formatTick(vx[vi]) + ", " + C.formatTick(vy[vi]) + ")";
          break;
        }
      }
    }

    if (!msg && this._vis("sq") && d.sq && d.sq.length >= 2) {
      var sqx = this.xToPx(d.sq[0]);
      var sqy = this.yToPx(d.sq[1]);
      if (Math.hypot(mx - sqx, my - sqy) < hitR + 4) {
        msg = "Status Quo\n(" + C.formatTick(d.sq[0]) + ", " + C.formatTick(d.sq[1]) + ")";
      }
    }

    if (!msg && this._vis("alternatives") && d.alternatives && d.alternatives.length >= 2) {
      var alts = d.alternatives;
      var an = d.alternative_names || [];
      for (var ai = 0; ai + 1 < alts.length; ai += 2) {
        var ax = this.xToPx(alts[ai]);
        var ay = this.yToPx(alts[ai + 1]);
        if (Math.hypot(mx - ax, my - ay) < hitR + 2) {
          msg = (an[ai / 2] || ("Alt " + (ai / 2 + 1))) + "\n(" + C.formatTick(alts[ai]) + ", " + C.formatTick(alts[ai + 1]) + ")";
          break;
        }
      }
    }

    function tryPoly(flat, label) {
      if (!flat || flat.length < 6) return;
      var scr = self._dataXYToScreenFlat(flat);
      if (C.pointInPolygon(mx, my, scr)) msg = label;
    }

    if (!msg) {
      var L = d.layers || {};
      if (this._vis("convex_hull") && L.convex_hull_xy) tryPoly(specXY(L.convex_hull_xy) || L.convex_hull_xy, "Pareto Set");
      if (!msg && this._vis("winset") && L.winset_paths) {
        for (var wi = 0; wi < L.winset_paths.length; wi++) {
          tryPoly(specXY(L.winset_paths[wi]), "Win set");
          if (msg) break;
        }
      }
      if (!msg && this._vis("preferred_regions") && L.preferred_regions) {
        for (var pi = 0; pi < L.preferred_regions.length; pi++) {
          tryPoly(specXY(L.preferred_regions[pi]), "Preferred region");
          if (msg) break;
        }
      }
      if (!msg && this._vis("ic") && L.ic_curves) {
        for (var ii = 0; ii < L.ic_curves.length; ii++) {
          tryPoly(specXY(L.ic_curves[ii]), "Indifference curve");
          if (msg) break;
        }
      }
      if (!msg && this._vis("uncovered") && L.uncovered_xy) tryPoly(specXY(L.uncovered_xy) || L.uncovered_xy, "Uncovered set");
      if (!msg && this._vis("yolk") && L.yolk && L.yolk.r > 0) {
        var ycx = this.xToPx(L.yolk.cx);
        var ycy = this.yToPx(L.yolk.cy);
        var yr = (L.yolk.r / (this.viewXMax - this.viewXMin)) * this.plotSide;
        if (Math.hypot(mx - ycx, my - ycy) < yr) msg = "Yolk";
      }
    }

    if (!msg && this._vis("centroid") && d.layers && d.layers.centroid) {
      var cx = this.xToPx(d.layers.centroid[0]);
      var cy = this.yToPx(d.layers.centroid[1]);
      if (Math.hypot(mx - cx, my - cy) < hitR) msg = "Centroid";
    }
    if (!msg && this._vis("marginal_median") && d.layers && d.layers.marginal_median) {
      var mx2 = this.xToPx(d.layers.marginal_median[0]);
      var my2 = this.yToPx(d.layers.marginal_median[1]);
      if (Math.hypot(mx - mx2, my - my2) < hitR) msg = "Marginal median";
    }

    if (!msg) {
      tip.style.display = "none";
      return;
    }
    tip.textContent = msg;
    tip.style.display = "block";
    var tw = tip.offsetWidth;
    var th = tip.offsetHeight;
    var px = clientX + 14;
    var py = clientY - th - 8;
    if (px + tw > window.innerWidth - 8) px = clientX - tw - 14;
    if (py < 8) py = clientY + 14;
    tip.style.left = px + "px";
    tip.style.top = py + "px";
  };

  SpatialVotingCanvas.prototype._vis = function(key) {
    var d = this.data;
    if (d && d.layer_toggles === false) {
      if (!Object.prototype.hasOwnProperty.call(this.visibility, key)) return true;
      return this.visibility[key] !== false;
    }
    return this.visibility[key] !== false;
  };

  /** Width reserved for the right-hand legend column (matches competition_canvas style). */
  SpatialVotingCanvas.prototype._legendColumnWidth = function(d) {
    var L = d.layers || {};
    var labels = ["Voters"];
    if (d.alternatives && d.alternatives.length) labels.push("Alternatives");
    if (d.sq && d.sq.length >= 2) labels.push("Status Quo");
    if (L.convex_hull_xy) labels.push("Pareto Set");
    if (L.winset_paths && L.winset_paths.length) labels.push("Win set");
    if (L.preferred_regions && L.preferred_regions.length) labels.push("Preferred regions");
    if (L.ic_curves && L.ic_curves.length) labels.push("Indiff. Curves");
    if (L.yolk && L.yolk.r > 0) labels.push("Yolk");
    if (L.uncovered_xy) labels.push("Uncovered set");
    if (L.centroid) labels.push("Centroid");
    if (L.marginal_median) labels.push("Marginal median");
    var tmp = document.createElement("canvas").getContext("2d");
    tmp.font = "12px " + FONT;
    var maxW = 0;
    for (var li = 0; li < labels.length; li++) {
      maxW = Math.max(maxW, tmp.measureText(labels[li]).width);
    }
    return Math.max(118, Math.ceil(8 + 13 + maxW + 22));
  };

  /** Checkbox row; parent defaults to overlay row (use showRow for Status Quo, etc.). */
  SpatialVotingCanvas.prototype._addToggleRow = function(key, labelText, parentEl) {
    var self = this;
    var parent = parentEl || this.overlayRow;
    var lab = document.createElement("label");
    lab.style.cssText = [
      "display:inline-flex",
      "align-items:center",
      "gap:3px",
      "margin:2px 12px 2px 0",
      "cursor:pointer",
      "user-select:none",
      "font-size:11px",
      "font-family:" + FONT,
      "color:#666",
      "line-height:1.2",
      "flex:0 0 auto",
      "white-space:nowrap"
    ].join(";");
    var cb = document.createElement("input");
    cb.type = "checkbox";
    cb.checked = true;
    this.visibility[key] = true;
    cb.style.cssText = [
      "margin:0",
      "cursor:pointer",
      "flex-shrink:0",
      "accent-color:" + UIC.button,
      "width:11px",
      "height:11px"
    ].join(";");
    cb.addEventListener("change", function() {
      self.visibility[key] = cb.checked;
      self.draw();
    });
    lab.appendChild(cb);
    var sp = document.createElement("span");
    sp.textContent = labelText;
    lab.appendChild(sp);
    parent.appendChild(lab);
  };

  /**
   * Control rows mirror competition_canvas: row1 tools (zoom, large points),
   * row2 "Show:" (heatmap first, same slot as competition), row3 "Overlays:"
   * with toggles in the same relative order as setData() appends IC after
   * static overlay keys, then Win set.
   */
  SpatialVotingCanvas.prototype._rebuildLegendPanel = function() {
    var self = this;
    var d = this.data;
    var L = d.layers || {};
    var showRow = this.showRow;
    var overlayRow = this.overlayRow;
    while (showRow.firstChild) showRow.removeChild(showRow.firstChild);
    while (overlayRow.firstChild) overlayRow.removeChild(overlayRow.firstChild);
    this.visibility = {};

    var hasVoters = d.voters_x && d.voters_x.length > 0;
    var hasSq = d.sq && d.sq.length >= 2;
    var showLayerToggles = d.layer_toggles !== false;

    function buildShowRow() {
      var showLabel = document.createElement("span");
      showLabel.textContent = "Show:";
      showLabel.style.cssText = "font-size:11px;color:#aaa;white-space:nowrap;flex-shrink:0;";
      showRow.appendChild(showLabel);
      if (hasVoters) {
        var heatCb = makeCheckbox("Heatmap", self.showHeatmap, UIC.button, function(v) {
          self.showHeatmap = v;
          self.draw();
        });
        heatCb.input.checked = self.showHeatmap;
        showRow.appendChild(heatCb.el);
      }
      if (hasSq) self._addToggleRow("sq", "Status Quo", showRow);
    }

    if (!showLayerToggles) {
      overlayRow.style.display = "none";
      if (!hasVoters && !hasSq) {
        showRow.style.display = "none";
        return;
      }
      showRow.style.display = "flex";
      buildShowRow();
      return;
    }

    if (hasVoters || hasSq) {
      showRow.style.display = "flex";
      buildShowRow();
    } else {
      showRow.style.display = "none";
    }

    overlayRow.style.display = "flex";
    var oSep = document.createElement("span");
    oSep.style.cssText = "color:#aaa;font-size:11px;padding-right:2px;flex-shrink:0;";
    oSep.textContent = "Overlays:";
    overlayRow.appendChild(oSep);

    if (L.convex_hull_xy) this._addToggleRow("convex_hull", "Pareto Set");
    if (L.centroid) this._addToggleRow("centroid", "Centroid");
    if (L.marginal_median) this._addToggleRow("marginal_median", "Marginal median");
    if (L.ic_curves && L.ic_curves.length) this._addToggleRow("ic", "Indiff. Curves");
    if (L.winset_paths && L.winset_paths.length) this._addToggleRow("winset", "Win set");
    if (L.preferred_regions && L.preferred_regions.length) {
      this._addToggleRow("preferred_regions", "Preferred regions");
    }
    if (L.yolk && L.yolk.r > 0) this._addToggleRow("yolk", "Yolk");
    if (L.uncovered_xy) this._addToggleRow("uncovered", "Uncovered set");
    this._addToggleRow("voters", "Voters");
    if (d.alternatives && d.alternatives.length) this._addToggleRow("alternatives", "Alternatives");
  };

  /** Right column on canvas: symbol + label (same idea as competition_canvas 2D legend). */
  SpatialVotingCanvas.prototype._drawSideLegend = function(ctx) {
    var d = this.data;
    var pad = this.padding;
    var side = this.plotSide;
    var tc = d._colors;
    var L = d.layers || {};
    var legX = pad.left + side + 16;
    var ps = this.pointScale || 1;
    var legCx = legX + 8;
    var rVoter = 5 * ps;
    var halfDiamond = 7 * ps;
    var rSqOuter = 8 * ps;
    var rSqInner = 3.2 * ps;
    var halfCentroid = 7 * ps;
    var halfMarginalTri = 7 * ps;
    var icCircleR = 5 * ps;
    var legendHalfMax = Math.max(rVoter, halfDiamond, rSqOuter, icCircleR, halfCentroid, halfMarginalTri);
    var clipLeft = Math.max(0, legX - 14);
    var clipW = this.width - clipLeft;
    ctx.save();
    ctx.beginPath();
    ctx.rect(clipLeft, pad.top, clipW, side);
    ctx.clip();

    ctx.textAlign = "left";

    var selfLegend = this;
    var vxSpan = selfLegend.viewXMax - selfLegend.viewXMin;
    var yolkRPlot = L.yolk && L.yolk.r > 0 && vxSpan > 1e-12
      ? (L.yolk.r / vxSpan) * selfLegend.plotSide
      : 0;
    var lineH = Math.max(22, Math.ceil(2 * Math.max(legendHalfMax, yolkRPlot || 0) + 10));
    var legTextX = legCx + legendHalfMax + 8;

    var titleTop = pad.top + 6;
    ctx.font = "600 11px " + FONT;
    ctx.fillStyle = tc.text_light;
    ctx.textBaseline = "top";
    ctx.fillText("Legend:", legTextX, titleTop);
    var titleBlockH = 14;
    var gapAfterTitle = 6;
    var ly = titleTop + titleBlockH + gapAfterTitle + legendHalfMax;

    ctx.font = "12px " + FONT;
    ctx.textBaseline = "middle";
    ctx.fillStyle = tc.text;

    function nextRow() {
      ly += lineH;
    }

    if (selfLegend._vis("voters")) {
      ctx.beginPath();
      ctx.arc(legCx, ly, rVoter, 0, Math.PI * 2);
      ctx.fillStyle = tc.voter_fill;
      ctx.fill();
      ctx.strokeStyle = tc.voter_stroke;
      ctx.lineWidth = 0.8;
      ctx.stroke();
      ctx.fillStyle = tc.text;
      ctx.fillText("Voters", legTextX, ly);
      nextRow();
    }
    if (selfLegend._vis("alternatives") && d.alternatives && d.alternatives.length) {
      C.drawDiamond(ctx, legCx, ly, halfDiamond, tc.alt_fill, 7 * ps);
      ctx.fillStyle = tc.text;
      ctx.fillText("Alternatives", legTextX, ly);
      nextRow();
    }
    if (selfLegend._vis("sq") && d.sq && d.sq.length >= 2) {
      ctx.fillStyle = tc.sq_fill;
      ctx.strokeStyle = d.theme === "bw" ? "rgba(40,40,40,0.85)" : "rgba(255,255,255,0.95)";
      ctx.lineWidth = 1.25;
      drawStar5Path(ctx, legCx, ly, rSqOuter, rSqInner);
      ctx.fill();
      ctx.stroke();
      ctx.fillStyle = tc.text;
      ctx.fillText("Status Quo", legTextX, ly);
      nextRow();
    }
    if (selfLegend._vis("convex_hull") && L.convex_hull_xy) {
      var sw = 12 * ps;
      var sh = 8 * ps;
      ctx.fillStyle = tc.hull_fill;
      ctx.fillRect(legCx - sw / 2, ly - sh / 2, sw, sh);
      ctx.strokeStyle = tc.hull_line;
      ctx.lineWidth = 1.5;
      ctx.setLineDash([3, 2]);
      ctx.strokeRect(legCx - sw / 2 + 0.5, ly - sh / 2 + 0.5, sw - 1, sh - 1);
      ctx.setLineDash([]);
      ctx.fillStyle = tc.text;
      ctx.fillText("Pareto Set", legTextX, ly);
      nextRow();
    }
    if (selfLegend._vis("winset") && L.winset_paths && L.winset_paths.length) {
      var swW = 12 * ps;
      var shW = 8 * ps;
      ctx.fillStyle = tc.winset_fill;
      ctx.fillRect(legCx - swW / 2, ly - shW / 2, swW, shW);
      ctx.strokeStyle = tc.winset_line;
      ctx.lineWidth = 1.5;
      ctx.strokeRect(legCx - swW / 2 + 0.5, ly - shW / 2 + 0.5, swW - 1, shW - 1);
      ctx.fillStyle = tc.text;
      ctx.fillText("Win set", legTextX, ly);
      nextRow();
    }
    if (selfLegend._vis("preferred_regions") && L.preferred_regions && L.preferred_regions.length) {
      var swP = 12 * ps;
      var shP = 8 * ps;
      ctx.fillStyle = tc.pref_fill;
      ctx.fillRect(legCx - swP / 2, ly - shP / 2, swP, shP);
      ctx.strokeStyle = tc.pref_line;
      ctx.lineWidth = 1.5;
      ctx.setLineDash([2, 2]);
      ctx.strokeRect(legCx - swP / 2 + 0.5, ly - shP / 2 + 0.5, swP - 1, shP - 1);
      ctx.setLineDash([]);
      ctx.fillStyle = tc.text;
      ctx.fillText("Preferred regions", legTextX, ly);
      nextRow();
    }
    if (selfLegend._vis("ic") && L.ic_curves && L.ic_curves.length) {
      var icCol = C.parseRGBA(tc.ic_line);
      if (icCol.a == null || icCol.a === 0) icCol = { r: 128, g: 128, b: 128, a: 0.9 };
      ctx.strokeStyle = C.rgba(icCol, 0.40);
      ctx.lineWidth = 2;
      ctx.setLineDash([]);
      ctx.beginPath();
      ctx.arc(legCx, ly, icCircleR, 0, Math.PI * 2);
      ctx.stroke();
      ctx.fillStyle = tc.text;
      ctx.fillText("Indiff. Curves", legTextX, ly);
      nextRow();
    }
    if (selfLegend._vis("yolk") && L.yolk && L.yolk.r > 0) {
      var yolkTx = legCx + yolkRPlot + 8;
      ctx.beginPath();
      ctx.arc(legCx, ly, yolkRPlot, 0, Math.PI * 2);
      ctx.fillStyle = tc.yolk_fill;
      ctx.fill();
      ctx.strokeStyle = L.yolk.line != null ? L.yolk.line : tc.yolk_line;
      ctx.lineWidth = 2;
      ctx.stroke();
      ctx.fillStyle = tc.text;
      ctx.fillText("Yolk", Math.max(legTextX, yolkTx), ly);
      nextRow();
    }
    if (selfLegend._vis("uncovered") && L.uncovered_xy) {
      var swU = 12 * ps;
      var shU = 8 * ps;
      ctx.fillStyle = tc.uncovered_fill;
      ctx.fillRect(legCx - swU / 2, ly - shU / 2, swU, shU);
      ctx.strokeStyle = tc.uncovered_line;
      ctx.lineWidth = 1.5;
      ctx.setLineDash([4, 2]);
      ctx.strokeRect(legCx - swU / 2 + 0.5, ly - shU / 2 + 0.5, swU - 1, shU - 1);
      ctx.setLineDash([]);
      ctx.fillStyle = tc.text;
      ctx.fillText("Uncovered set", legTextX, ly);
      nextRow();
    }
    if (selfLegend._vis("centroid") && L.centroid) {
      var oc = C.OVERLAY_COLORS.centroid;
      ctx.strokeStyle = L.centroid_stroke || oc.pt;
      ctx.lineWidth = 2;
      var lr = halfCentroid;
      ctx.beginPath();
      ctx.moveTo(legCx - lr, ly);
      ctx.lineTo(legCx + lr, ly);
      ctx.moveTo(legCx, ly - lr);
      ctx.lineTo(legCx, ly + lr);
      ctx.stroke();
      ctx.fillStyle = tc.text;
      ctx.fillText("Centroid", legTextX, ly);
      nextRow();
    }
    if (selfLegend._vis("marginal_median") && L.marginal_median) {
      var om = C.OVERLAY_COLORS.marginal_median;
      ctx.fillStyle = L.marginal_median_fill || om.pt;
      ctx.strokeStyle = L.marginal_median_stroke || om.stroke;
      ctx.lineWidth = 1;
      var tr = halfMarginalTri;
      ctx.beginPath();
      ctx.moveTo(legCx, ly - tr);
      ctx.lineTo(legCx - tr * 0.866, ly + tr * 0.5);
      ctx.lineTo(legCx + tr * 0.866, ly + tr * 0.5);
      ctx.closePath();
      ctx.fill();
      ctx.stroke();
      ctx.fillStyle = tc.text;
      ctx.fillText("Marginal median", legTextX, ly);
    }

    ctx.restore();
  };

  SpatialVotingCanvas.prototype.setData = function(data) {
    this.data = data;
    if (!data.theme_colors) data.theme_colors = {};
    data._colors = mergeColors(defaultThemeColors(), data.theme_colors);
    this.showHeatmap = false;
    this.pointScale = 1;
    if (this.pointBtn) {
      this.pointBtn.style.background = "#fff";
      this.pointBtn.style.fontWeight = "normal";
    }
    this._initView();
    this._rebuildLegendPanel();
    this.resize();
  };

  SpatialVotingCanvas.prototype.resize = function() {
    var d = this.data;
    if (!d) return;
    var cw = this.el.offsetWidth || this.width;
    var ch = this.el.offsetHeight || this.height;
    if (cw <= 0 || ch <= 0) return;
    this.width = cw;
    this.height = ch;
    this.container.style.width = cw + "px";
    this.container.style.height = ch + "px";

    var hasVoters = d.voters_x && d.voters_x.length > 0;
    var hasSq = d.sq && d.sq.length >= 2;
    var showLayerToggles = d.layer_toggles !== false;
    var toolsH = 44;
    var showRowH = hasVoters || hasSq ? 34 : 0;
    var overlayRowH = showLayerToggles ? 34 : 0;
    this.controlsH = toolsH + showRowH + overlayRowH;
    var plotCanvasH = Math.max(160, ch - this.controlsH);
    this.plotCanvasH = plotCanvasH;

    var padLeft0 = 56;
    var padTop0 = 44;
    var padBottom0 = 56;
    var legendColW = this._legendColumnWidth(d);
    var padRight = legendColW + 8;
    var availW = cw - padLeft0 - padRight;
    var availH = plotCanvasH - padTop0 - padBottom0;
    var side = Math.max(0, Math.min(availW, availH));
    var plotLeft = padLeft0 + Math.floor((availW - side) / 2);
    var plotTop = padTop0 + Math.floor((availH - side) / 2);
    this.plotSide = side;
    this.padding = { left: plotLeft, top: plotTop, right: padRight, bottom: padBottom0 };

    var cv = this.canvas;
    var dpr = this.dpr;
    cv.width = cw * dpr;
    cv.height = plotCanvasH * dpr;
    cv.style.width = cw + "px";
    cv.style.height = plotCanvasH + "px";
    cv.style.display = "block";
    cv.style.flexShrink = "0";
    var ctx = cv.getContext("2d");
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    this._ctx = ctx;
    var padL = plotLeft + "px";
    var padR = "12px";
    if (this.toolsRow) {
      this.toolsRow.style.paddingLeft = padL;
      this.toolsRow.style.paddingRight = padR;
    }
    if (this.showRow) {
      this.showRow.style.paddingLeft = padL;
      this.showRow.style.paddingRight = padR;
    }
    if (this.overlayRow) {
      this.overlayRow.style.paddingLeft = padL;
      this.overlayRow.style.paddingRight = "10px";
    }
    this.computeHeatmap();
    this.draw();
  };

  SpatialVotingCanvas.prototype.draw = function() {
    var d = this.data;
    var ctx = this._ctx;
    if (!d || !ctx || !this.padding) return;

    var self = this;

    var cw = this.width;
    var ch = this.plotCanvasH != null ? this.plotCanvasH : this.height;
    var pad = this.padding;
    var side = this.plotSide;
    var tc = d._colors;

    ctx.fillStyle = tc.plot_bg;
    ctx.fillRect(0, 0, cw, ch);

    ctx.strokeStyle = tc.border;
    ctx.lineWidth = 1;
    ctx.strokeRect(pad.left + 0.5, pad.top + 0.5, side - 1, side - 1);

    ctx.save();
    ctx.beginPath();
    ctx.rect(pad.left, pad.top, side, side);
    ctx.clip();

    if (self.showHeatmap && self.heatmapCanvas && d.voters_x && d.voters_x.length) {
      var fullXRange = d.xlim[1] - d.xlim[0];
      var fullYRange = d.ylim[1] - d.ylim[0];
      if (fullXRange > 0 && fullYRange > 0) {
        var kW = self.heatmapCanvas.width;
        var kH = self.heatmapCanvas.height;
        var hsx = (self.viewXMin - d.xlim[0]) / fullXRange * kW;
        var hsy = (1 - (self.viewYMax - d.ylim[0]) / fullYRange) * kH;
        var hsw = (self.viewXMax - self.viewXMin) / fullXRange * kW;
        var hsh = (self.viewYMax - self.viewYMin) / fullYRange * kH;
        ctx.save();
        ctx.imageSmoothingEnabled = true;
        ctx.imageSmoothingQuality = "high";
        ctx.drawImage(self.heatmapCanvas, hsx, hsy, hsw, hsh, pad.left, pad.top, side, side);
        ctx.restore();
      }
    }

    var ps = self.pointScale || 1;

    var gx0 = this.viewXMin;
    var gx1 = this.viewXMax;
    var gy0 = this.viewYMin;
    var gy1 = this.viewYMax;
    ctx.setLineDash([3, 4]);
    ctx.strokeStyle = tc.grid;
    ctx.lineWidth = 1;
    var nGrid = 4;
    for (var gi = 1; gi < nGrid; gi++) {
      var gtf = gi / nGrid;
      var gxp = pad.left + gtf * side;
      var gyp = pad.top + gtf * side;
      ctx.beginPath();
      ctx.moveTo(gxp, pad.top);
      ctx.lineTo(gxp, pad.top + side);
      ctx.stroke();
      ctx.beginPath();
      ctx.moveTo(pad.left, gyp);
      ctx.lineTo(pad.left + side, gyp);
      ctx.stroke();
    }
    ctx.setLineDash([]);

    ctx.strokeStyle = tc.axis;
    ctx.lineWidth = 1.5;
    if (gx0 < 0 && gx1 > 0) {
      var ox = this.xToPx(0);
      ctx.beginPath();
      ctx.moveTo(ox, pad.top);
      ctx.lineTo(ox, pad.top + side);
      ctx.stroke();
    }
    if (gy0 < 0 && gy1 > 0) {
      var oy = this.yToPx(0);
      ctx.beginPath();
      ctx.moveTo(pad.left, oy);
      ctx.lineTo(pad.left + side, oy);
      ctx.stroke();
    }

    var L = d.layers || {};

    function drawFilled(flat, fill, line, dash, strokeLw) {
      if (!flat || flat.length < 4) return;
      var scr = [];
      for (var k = 0; k < flat.length; k += 2) {
        scr.push(self.xToPx(flat[k]));
        scr.push(self.yToPx(flat[k + 1]));
      }
      ctx.beginPath();
      ctx.moveTo(scr[0], scr[1]);
      for (var m = 2; m < scr.length; m += 2) ctx.lineTo(scr[m], scr[m + 1]);
      ctx.closePath();
      ctx.fillStyle = fill;
      ctx.fill();
      ctx.strokeStyle = line;
      ctx.lineWidth = strokeLw != null ? strokeLw : 1.5;
      ctx.setLineDash(dash || []);
      ctx.stroke();
      ctx.setLineDash([]);
    }

    function drawLineLoop(flat, color, lw, dash) {
      if (!flat || flat.length < 4) return;
      ctx.beginPath();
      ctx.moveTo(self.xToPx(flat[0]), self.yToPx(flat[1]));
      for (var k = 2; k < flat.length; k += 2) {
        ctx.lineTo(self.xToPx(flat[k]), self.yToPx(flat[k + 1]));
      }
      ctx.closePath();
      ctx.strokeStyle = color;
      ctx.lineWidth = lw != null ? lw : 1.5;
      ctx.setLineDash(dash || [4, 3]);
      ctx.stroke();
      ctx.setLineDash([]);
    }

    if (this._vis("convex_hull") && L.convex_hull_xy) {
      var hullSpec = L.convex_hull_xy;
      var hullFlat = specXY(hullSpec) || hullSpec;
      var hullFill = (hullSpec && hullSpec.fill != null) ? hullSpec.fill : tc.hull_fill;
      var hullLine = (hullSpec && hullSpec.line != null) ? hullSpec.line : tc.hull_line;
      drawFilled(hullFlat, hullFill, hullLine, [6, 4]);
    }
    if (this._vis("winset") && L.winset_paths) {
      for (var w = 0; w < L.winset_paths.length; w++) {
        var ws = L.winset_paths[w];
        var wxy = specXY(ws);
        var wf = (ws && ws.fill != null) ? ws.fill : tc.winset_fill;
        var wl = (ws && ws.line != null) ? ws.line : tc.winset_line;
        drawFilled(wxy, wf, wl, []);
      }
    }
    if (this._vis("preferred_regions") && L.preferred_regions) {
      for (var p = 0; p < L.preferred_regions.length; p++) {
        var pr = L.preferred_regions[p];
        var pxy = specXY(pr);
        var pf = (pr && pr.fill != null) ? pr.fill : tc.pref_fill;
        var pln = (pr && pr.line != null) ? pr.line : tc.pref_line;
        drawFilled(pxy, pf, pln, [4, 3]);
      }
    }
    if (this._vis("yolk") && L.yolk && L.yolk.r > 0) {
      var ycx = this.xToPx(L.yolk.cx);
      var ycy = this.yToPx(L.yolk.cy);
      var yr = (L.yolk.r / (this.viewXMax - this.viewXMin)) * this.plotSide;
      ctx.beginPath();
      ctx.arc(ycx, ycy, yr, 0, Math.PI * 2);
      ctx.fillStyle = L.yolk.fill != null ? L.yolk.fill : tc.yolk_fill;
      ctx.fill();
      ctx.strokeStyle = L.yolk.line != null ? L.yolk.line : tc.yolk_line;
      ctx.lineWidth = 2;
      ctx.stroke();
    }
    if (this._vis("uncovered") && L.uncovered_xy) {
      var uSpec = L.uncovered_xy;
      var ux = specXY(uSpec) || uSpec;
      var uf = (uSpec && uSpec.fill != null) ? uSpec.fill : tc.uncovered_fill;
      var ul = (uSpec && uSpec.line != null) ? uSpec.line : tc.uncovered_line;
      drawFilled(ux, uf, ul, [8, 4]);
    }
    if (this._vis("ic") && L.ic_curves) {
      for (var ic = 0; ic < L.ic_curves.length; ic++) {
        var it = L.ic_curves[ic];
        var ixy = specXY(it);
        var iline = (it && it.line != null) ? it.line : tc.ic_line;
        var ifill = it && it.fill;
        if (hasFillColor(ifill)) {
          drawFilled(ixy, ifill, iline, [4, 3], 2);
        } else {
          drawLineLoop(ixy, iline, 2, [4, 3]);
        }
      }
    }

    var vx = d.voters_x || [];
    var vy = d.voters_y || [];
    if (this._vis("voters")) {
      for (var vi = 0; vi < vx.length; vi++) {
        var vpx = this.xToPx(vx[vi]);
        var vpy = this.yToPx(vy[vi]);
        var vr = 5 * ps;
        ctx.beginPath();
        ctx.arc(vpx, vpy, vr, 0, Math.PI * 2);
        ctx.fillStyle = tc.voter_fill;
        ctx.fill();
        ctx.strokeStyle = tc.voter_stroke;
        ctx.lineWidth = 0.8;
        ctx.stroke();
      }
    }

    if (this._vis("alternatives") && d.alternatives) {
      var al = d.alternatives;
      var an = d.alternative_names || [];
      for (var ai = 0; ai + 1 < al.length; ai += 2) {
        C.drawDiamond(ctx, this.xToPx(al[ai]), this.yToPx(al[ai + 1]), 7 * ps, tc.alt_fill, 7 * ps);
      }
      if (d.show_labels && an.length) {
        ctx.fillStyle = tc.text;
        ctx.font = "11px " + FONT;
        ctx.textAlign = "center";
        ctx.textBaseline = "bottom";
        for (var li = 0; li + 1 < al.length; li += 2) {
          var lab = an[li / 2] || ("Alt " + (li / 2 + 1));
          ctx.fillText(lab, this.xToPx(al[li]), this.yToPx(al[li + 1]) - 10);
        }
      }
    }

    if (this._vis("sq") && d.sq && d.sq.length >= 2) {
      var sqx = this.xToPx(d.sq[0]);
      var sqy = this.yToPx(d.sq[1]);
      var outer = 8 * ps;
      var inner = 3.2 * ps;
      ctx.fillStyle = tc.sq_fill;
      ctx.strokeStyle = d.theme === "bw" ? "rgba(40,40,40,0.85)" : "rgba(255,255,255,0.95)";
      ctx.lineWidth = 1.25;
      drawStar5Path(ctx, sqx, sqy, outer, inner);
      ctx.fill();
      ctx.stroke();
    }

    if (this._vis("centroid") && L.centroid && L.centroid.length >= 2) {
      var cpx = this.xToPx(L.centroid[0]);
      var cpy = this.yToPx(L.centroid[1]);
      var oc = C.OVERLAY_COLORS.centroid;
      ctx.strokeStyle = L.centroid_stroke || oc.pt;
      ctx.lineWidth = 2;
      var cr = 7 * ps;
      ctx.beginPath();
      ctx.moveTo(cpx - cr, cpy);
      ctx.lineTo(cpx + cr, cpy);
      ctx.moveTo(cpx, cpy - cr);
      ctx.lineTo(cpx, cpy + cr);
      ctx.stroke();
    }
    if (this._vis("marginal_median") && L.marginal_median && L.marginal_median.length >= 2) {
      var mmx = this.xToPx(L.marginal_median[0]);
      var mmy = this.yToPx(L.marginal_median[1]);
      var om = C.OVERLAY_COLORS.marginal_median;
      ctx.fillStyle = L.marginal_median_fill || om.pt;
      ctx.strokeStyle = L.marginal_median_stroke || om.stroke;
      ctx.lineWidth = 1;
      var tr = 7 * ps;
      ctx.beginPath();
      ctx.moveTo(mmx, mmy - tr);
      ctx.lineTo(mmx - tr * 0.866, mmy + tr * 0.5);
      ctx.lineTo(mmx + tr * 0.866, mmy + tr * 0.5);
      ctx.closePath();
      ctx.fill();
      ctx.stroke();
    }

    ctx.restore();

    ctx.fillStyle = tc.text;
    ctx.font = "14px " + FONT;
    ctx.textAlign = "center";
    ctx.textBaseline = "alphabetic";
    ctx.fillText(d.title || "", pad.left + side / 2, 22);

    if (side > 0) {
      var nTicks = 5;
      var vxRange = this.viewXMax - this.viewXMin;
      var vyRange = this.viewYMax - this.viewYMin;
      ctx.strokeStyle = tc.axis;
      ctx.lineWidth = 1;
      ctx.font = "11px " + FONT;
      ctx.fillStyle = tc.text_light;
      ctx.textAlign = "center";
      ctx.textBaseline = "top";
      for (var xi = 0; xi <= nTicks; xi++) {
        var xtf = xi / nTicks;
        var xv = this.viewXMin + xtf * vxRange;
        var xpx = pad.left + xtf * side;
        ctx.beginPath();
        ctx.moveTo(xpx, pad.top + side);
        ctx.lineTo(xpx, pad.top + side + 5);
        ctx.stroke();
        ctx.fillText(C.formatTick(xv), xpx, pad.top + side + 8);
      }
      ctx.textAlign = "right";
      ctx.textBaseline = "middle";
      for (var yi = 0; yi <= nTicks; yi++) {
        var ytf = yi / nTicks;
        var yv = this.viewYMin + ytf * vyRange;
        var ypx = pad.top + (1 - ytf) * side;
        ctx.beginPath();
        ctx.moveTo(pad.left, ypx);
        ctx.lineTo(pad.left - 5, ypx);
        ctx.stroke();
        ctx.fillText(C.formatTick(yv), pad.left - 8, ypx);
      }
    }

    var dnames = d.dim_names || ["Dimension 1", "Dimension 2"];
    ctx.fillStyle = tc.text;
    ctx.font = "12px " + FONT;
    ctx.textAlign = "center";
    ctx.textBaseline = "alphabetic";
    ctx.fillText(dnames[0], pad.left + side / 2, pad.top + side + 44);
    ctx.save();
    ctx.translate(pad.left - 48, pad.top + side / 2);
    ctx.rotate(-Math.PI / 2);
    ctx.fillText(dnames[1], 0, 0);
    ctx.restore();

    this._drawSideLegend(ctx);
  };

  if (typeof HTMLWidgets !== "undefined") {
    HTMLWidgets.widget({
      name: "spatial_voting_canvas",
      type: "output",
      factory: function(el, width, height) {
        var w = new SpatialVotingCanvas(el, width, height);
        return {
          renderValue: function(x) { if (x != null) w.setData(x); },
          resize: function(ww, hh) { w.resize(); }
        };
      }
    });
  }
})();
