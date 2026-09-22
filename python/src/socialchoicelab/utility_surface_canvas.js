/**
 * 3D utility surface widget — scientific style matching the 2D spatial canvas.
 *
 * Camera orbit approach (standard Three.js pattern):
 *   The scene is static. The camera orbits around it using spherical
 *   coordinates (theta = azimuth, phi = polar angle from Y-up).
 *   Preset views just move the camera to known positions.
 *   camera.lookAt(0,0,0) is called after every position change.
 *
 * Payload fields set by plot_utility_surface() in R / Python:
 *   xvals        float[]   N x-axis grid values
 *   yvals        float[]   N y-axis grid values
 *   z            float[]   N*N utility values, row-major
 *   ideal_x      float     ideal point x
 *   ideal_y      float     ideal point y
 *   ideal_u      float     utility at peak
 *   min_u        float     minimum utility on grid
 *   max_u        float     maximum utility on grid
 *   level_values float[]   utility values for contour rings
 *   axis_labels  string[]  [xLabel, yLabel, "Utility"]
 *   title        string    optional plot title
 */
/* global HTMLWidgets, THREE */
(function () {
  "use strict";

  /* ── Design constants ────────────────────────────────────────────────────── */
  var FONT       = "system-ui, -apple-system, BlinkMacSystemFont, sans-serif";
  var TEXT_COLOR = "#2d2d2d";
  var TEXT_LIGHT = "#444";
  var AXIS_HEX   = 0x333333;
  var BG_COLOR   = new THREE.Color(0xfafafa);

  var SURF_LO    = new THREE.Color(0xdce8f7);
  var SURF_HI    = new THREE.Color(0x8ab4e0);

  var DARK2 = [0x1b9e77, 0xd95f02, 0x7570b3, 0xe7298a,
               0x66a61e, 0xe6ab02, 0xa6761d];

  /* Tube radii */
  var R_AXIS = 0.010;
  var R_TICK = 0.006;
  var R_RING = 0.007;
  var R_DROP = 0.005;

  var N_TICKS = 5;

  /* ── Camera orbit defaults (spherical coordinates) ───────────────────────── */
  /* theta = azimuth in XZ plane;  phi = polar angle from +Y axis.
     phi = 0 → directly above;  phi = π/2 → horizontal;  phi = π → below. */
  var CAM_RADIUS = 5.734;   /* sqrt(3.8² + 2² + 3.8²) */
  var INIT_THETA = Math.PI / 4;                          /* 45° azimuth */
  var INIT_PHI   = Math.acos(2.0 / CAM_RADIUS);          /* ~69.6° from top */
  var PHI_EPS    = 0.001;   /* keep phi away from exact poles for lookAt */

  /* ── Tube helper: thin cylinder from p1 to p2 ────────────────────────────── */
  var _yAxis = new THREE.Vector3(0, 1, 0);

  function makeTube(p1, p2, radius, hexColor) {
    var dir = new THREE.Vector3().subVectors(p2, p1);
    var len = dir.length();
    if (len < 1e-6) return new THREE.Object3D();
    var geo = new THREE.CylinderGeometry(radius, radius, len, 6, 1, false);
    var mat = new THREE.MeshLambertMaterial({ color: hexColor });
    var mesh = new THREE.Mesh(geo, mat);
    mesh.position.copy(p1).add(p2).multiplyScalar(0.5);
    mesh.quaternion.setFromUnitVectors(_yAxis, dir.normalize());
    return mesh;
  }

  function makeDashedTube(p1, p2, radius, hexColor, dashLen, gapLen) {
    var grp = new THREE.Group();
    var dir = new THREE.Vector3().subVectors(p2, p1);
    var total = dir.length();
    if (total < 1e-6) return grp;
    var unit = dir.clone().normalize();
    var t = 0;
    var phase = true;
    while (t < total) {
      var segLen = Math.min(phase ? dashLen : gapLen, total - t);
      if (phase && segLen > 1e-6) {
        var a = p1.clone().addScaledVector(unit, t);
        var b = p1.clone().addScaledVector(unit, t + segLen);
        grp.add(makeTube(a, b, radius, hexColor));
      }
      t += segLen;
      phase = !phase;
    }
    return grp;
  }

  /* ── Surface geometry ────────────────────────────────────────────────────── */
  function lerpColor(t, lo, hi) {
    return new THREE.Color(
      lo.r + t * (hi.r - lo.r),
      lo.g + t * (hi.g - lo.g),
      lo.b + t * (hi.b - lo.b)
    );
  }

  function buildSurface(d, norm) {
    var N   = d.xvals.length;
    var z   = d.z;
    var pos = new Float32Array(N * N * 3);
    var col = new Float32Array(N * N * 3);

    for (var i = 0; i < N; i++) {
      for (var j = 0; j < N; j++) {
        var idx = i * N + j;
        var u = z[idx];
        var t = norm.t(u);
        pos[idx * 3]     = norm.x(d.xvals[i]);
        pos[idx * 3 + 1] = norm.u(u);
        pos[idx * 3 + 2] = norm.y(d.yvals[j]);
        var c = lerpColor(t, SURF_LO, SURF_HI);
        col[idx * 3]     = c.r;
        col[idx * 3 + 1] = c.g;
        col[idx * 3 + 2] = c.b;
      }
    }

    var indices = [];
    for (var ii = 0; ii < N - 1; ii++) {
      for (var jj = 0; jj < N - 1; jj++) {
        var a = ii * N + jj, b = (ii + 1) * N + jj;
        var cc = ii * N + (jj + 1), dd = (ii + 1) * N + (jj + 1);
        indices.push(a, b, cc, b, dd, cc);
      }
    }

    var geo = new THREE.BufferGeometry();
    geo.setAttribute("position", new THREE.BufferAttribute(pos, 3));
    geo.setAttribute("color",    new THREE.BufferAttribute(col, 3));
    geo.setIndex(indices);
    geo.computeVertexNormals();

    var mat = new THREE.MeshLambertMaterial({
      vertexColors: true,
      side: THREE.DoubleSide,
      transparent: true,
      opacity: 0.40
    });
    return new THREE.Mesh(geo, mat);
  }

  /* ── Contour rings: merged tube meshes per level ─────────────────────────── */
  function buildContours(d, norm) {
    var N   = d.xvals.length;
    var z   = d.z;
    var grp = new THREE.Group();
    var OFF = 0.003;

    d.level_values.forEach(function (level, li) {
      if (level <= d.min_u || level >= d.max_u) return;

      var pairs = [];

      function tryEdge(x0, y0, u0, x1, y1, u1) {
        if ((u0 < level) === (u1 < level)) return null;
        var t = (level - u0) / (u1 - u0);
        return new THREE.Vector3(
          norm.x(x0 + t * (x1 - x0)),
          norm.u(level) + OFF,
          norm.y(y0 + t * (y1 - y0))
        );
      }

      for (var i = 0; i < N - 1; i++) {
        for (var j = 0; j < N - 1; j++) {
          var z00 = z[i * N + j],       z10 = z[(i + 1) * N + j];
          var z01 = z[i * N + (j + 1)], z11 = z[(i + 1) * N + (j + 1)];
          var edge = [];
          var p;
          p = tryEdge(d.xvals[i],   d.yvals[j],   z00, d.xvals[i+1], d.yvals[j],   z10); if (p) edge.push(p);
          p = tryEdge(d.xvals[i+1], d.yvals[j],   z10, d.xvals[i+1], d.yvals[j+1], z11); if (p) edge.push(p);
          p = tryEdge(d.xvals[i],   d.yvals[j+1], z01, d.xvals[i+1], d.yvals[j+1], z11); if (p) edge.push(p);
          p = tryEdge(d.xvals[i],   d.yvals[j],   z00, d.xvals[i],   d.yvals[j+1], z01); if (p) edge.push(p);
          if (edge.length === 2) { pairs.push(edge[0], edge[1]); }
        }
      }
      if (pairs.length === 0) return;

      var positions = [];
      var triIdx    = [];
      var vBase     = 0;
      var up        = new THREE.Vector3(0, 1, 0);
      var tmpU      = new THREE.Vector3();
      var tmpV      = new THREE.Vector3();
      var dir       = new THREE.Vector3();

      for (var k = 0; k < pairs.length; k += 2) {
        var p1 = pairs[k], p2 = pairs[k + 1];
        dir.subVectors(p2, p1);
        var len = dir.length();
        if (len < 1e-6) continue;
        dir.divideScalar(len);

        if (Math.abs(dir.y) < 0.9) {
          tmpU.crossVectors(dir, up).normalize();
        } else {
          tmpU.set(1, 0, 0);
        }
        tmpV.crossVectors(dir, tmpU).normalize();

        var r = R_RING;
        var corners = [
          tmpU.clone().multiplyScalar(r),
          tmpV.clone().multiplyScalar(r),
          tmpU.clone().multiplyScalar(-r),
          tmpV.clone().multiplyScalar(-r)
        ];

        for (var c = 0; c < 4; c++) {
          var v1 = p1.clone().add(corners[c]);
          var v2 = p2.clone().add(corners[c]);
          positions.push(v1.x, v1.y, v1.z, v2.x, v2.y, v2.z);
        }

        for (var f = 0; f < 4; f++) {
          var a  = vBase + f * 2;
          var b  = vBase + ((f + 1) % 4) * 2;
          var c1 = vBase + f * 2 + 1;
          var d1 = vBase + ((f + 1) % 4) * 2 + 1;
          triIdx.push(a, b, c1, b, d1, c1);
        }
        vBase += 8;
      }

      var geo = new THREE.BufferGeometry();
      geo.setAttribute("position", new THREE.Float32BufferAttribute(positions, 3));
      geo.setIndex(triIdx);
      geo.computeVertexNormals();
      var mat = new THREE.MeshLambertMaterial({ color: DARK2[li % DARK2.length] });
      grp.add(new THREE.Mesh(geo, mat));
    });

    return grp;
  }

  /* ── Axes: tube geometry + tick marks ───────────────────────────────────── */
  function buildAxes(norm, d, outTickData) {
    var grp  = new THREE.Group();
    var OG   = -1.0;
    var AX   =  1.12;
    var uTip = norm.u(d.max_u) + 0.12;
    var TK   = 0.06;
    var N    = d.xvals.length;

    function axis(p1, p2) { return makeTube(p1, p2, R_AXIS, AXIS_HEX); }
    function tick(p1, p2) { return makeTube(p1, p2, R_TICK, AXIS_HEX); }
    function v(x, y, z)   { return new THREE.Vector3(x, y, z); }

    grp.add(axis(v(OG, OG, OG), v(AX,   OG,  OG)));
    grp.add(axis(v(OG, OG, OG), v(OG,  uTip, OG)));
    grp.add(axis(v(OG, OG, OG), v(OG,   OG,  AX)));

    for (var xi = 0; xi < N_TICKS; xi++) {
      var t  = xi / (N_TICKS - 1);
      var xv = d.xvals[0] + t * (d.xvals[N-1] - d.xvals[0]);
      var px = norm.x(xv);
      grp.add(tick(v(px, OG - TK, OG), v(px, OG + TK, OG)));
      outTickData.push({ pos: v(px, OG - TK * 2.5, OG), label: _fmt(xv), dx: -12, dy: 4 });
    }

    for (var zi = 0; zi < N_TICKS; zi++) {
      var tz = zi / (N_TICKS - 1);
      var yv = d.yvals[0] + tz * (d.yvals[N-1] - d.yvals[0]);
      var pz = norm.y(yv);
      grp.add(tick(v(OG - TK, OG, pz), v(OG + TK, OG, pz)));
      outTickData.push({ pos: v(OG - TK * 2.5, OG, pz), label: _fmt(yv), dx: -24, dy: 4 });
    }

    for (var ui = 0; ui < N_TICKS; ui++) {
      var tu = ui / (N_TICKS - 1);
      var uv = d.min_u + tu * (d.max_u - d.min_u);
      var pu = norm.u(uv);
      grp.add(tick(v(OG - TK, pu, OG), v(OG + TK, pu, OG)));
      outTickData.push({ pos: v(OG - TK * 2.5, pu, OG), label: _fmt(uv), dx: -30, dy: -6 });
    }

    return grp;
  }

  /* ── Ideal point marker ──────────────────────────────────────────────────── */
  function buildIdealMarker(d, norm) {
    var grp  = new THREE.Group();
    var peakX = norm.x(d.ideal_x);
    var peakY = norm.u(d.ideal_u);
    var peakZ = norm.y(d.ideal_y);

    var floorDot = new THREE.Mesh(
      new THREE.SphereGeometry(0.04, 14, 14),
      new THREE.MeshLambertMaterial({ color: 0x222222 })
    );
    floorDot.position.set(peakX, -1.0, peakZ);
    grp.add(floorDot);

    grp.add(makeDashedTube(
      new THREE.Vector3(peakX, -1.0,   peakZ),
      new THREE.Vector3(peakX, peakY,  peakZ),
      R_DROP, 0x555555, 0.10, 0.06
    ));

    return grp;
  }

  /* ── HTML label helpers ──────────────────────────────────────────────────── */
  function makeSpan(text, fontSize, color, bold) {
    var s = document.createElement("span");
    s.textContent = text;
    s.style.cssText = [
      "position:absolute",
      "pointer-events:none",
      "font-family:" + FONT,
      "font-size:" + (fontSize || 11) + "px",
      "color:" + (color || TEXT_COLOR),
      "white-space:nowrap",
      bold ? "font-weight:600" : ""
    ].join(";");
    return s;
  }

  function _fmt(val) {
    return val.toFixed(2).replace(/\.?0+$/, "");
  }

  /* ── HTMLWidgets factory ─────────────────────────────────────────────────── */
  HTMLWidgets.widget({
    name: "utility_surface_canvas",
    type: "output",

    factory: function (el, width, height) {
      var renderer  = null;
      var scene     = null;
      var camera    = null;
      var animId    = null;
      var labelDiv      = null;
      var axisLabels    = [];
      var tickData      = [];
      var tickEls       = [];
      var contourLabels = [];

      /* Spherical camera state */
      var theta = INIT_THETA;
      var phi   = INIT_PHI;

      var isDragging = false;
      var lastMX = 0, lastMY = 0;

      function cleanup() {
        if (animId)   { cancelAnimationFrame(animId); animId = null; }
        if (renderer) { renderer.dispose(); renderer.forceContextLoss(); }
        el.innerHTML = "";
        labelDiv      = null;
        axisLabels    = [];
        tickData      = [];
        tickEls       = [];
        contourLabels = [];
      }

      /* Move camera to current (theta, phi) on its orbit sphere.
         Near the poles, camera.up must be horizontal to avoid the
         lookAt degeneracy (forward ≈ ±up → undefined orientation). */
      function applyCameraPosition() {
        var clamped = Math.max(PHI_EPS, Math.min(Math.PI - PHI_EPS, phi));
        camera.position.setFromSphericalCoords(CAM_RADIUS, clamped, theta);
        if (clamped < 0.3) {
          camera.up.set(0, 0, -1);
        } else if (clamped > Math.PI - 0.3) {
          camera.up.set(0, 0, 1);
        } else {
          camera.up.set(0, 1, 0);
        }
        camera.lookAt(0, 0, 0);
      }

      return {
        renderValue: function (d) {
          cleanup();
          theta = INIT_THETA;
          phi   = INIT_PHI;

          var N      = d.xvals.length;
          var xRange = d.xvals[N-1] - d.xvals[0] || 1;
          var yRange = d.yvals[N-1] - d.yvals[0] || 1;
          var uRange = d.max_u - d.min_u || 1;

          var norm = {
            x: function (v) { return (v - d.xvals[0]) / xRange * 2 - 1; },
            y: function (v) { return (v - d.yvals[0]) / yRange * 2 - 1; },
            u: function (u) { return (u - d.min_u) / uRange * 1.5 - 1.0; },
            t: function (u) { return (u - d.min_u) / uRange; }
          };

          /* Scene */
          scene = new THREE.Scene();
          scene.background = BG_COLOR;

          camera = new THREE.PerspectiveCamera(36, width / height, 0.01, 100);
          applyCameraPosition();

          renderer = new THREE.WebGLRenderer({ antialias: true });
          renderer.setPixelRatio(window.devicePixelRatio || 1);
          renderer.setSize(width, height);
          el.style.position = "relative";
          el.appendChild(renderer.domElement);

          /* Lighting */
          scene.add(new THREE.AmbientLight(0xffffff, 0.72));
          var dl = new THREE.DirectionalLight(0xffffff, 0.32);
          dl.position.set(3, 6, 3);
          scene.add(dl);

          /* Scene objects (static — camera moves, not the scene) */
          tickData = [];
          scene.add(buildSurface(d, norm));
          scene.add(buildContours(d, norm));
          scene.add(buildAxes(norm, d, tickData));
          scene.add(buildIdealMarker(d, norm));

          /* Label overlay */
          labelDiv = document.createElement("div");
          labelDiv.style.cssText =
            "position:absolute;top:0;left:0;width:100%;height:100%;pointer-events:none;";
          el.appendChild(labelDiv);

          var labels = d.axis_labels || ["Dimension 1", "Dimension 2", "Utility"];
          axisLabels = [
            { el: makeSpan(labels[0], 12, TEXT_COLOR, true),
              pos: new THREE.Vector3( 1.22, -1.0, -1.0), dx:  4, dy:  0 },
            { el: makeSpan(labels[1], 12, TEXT_COLOR, true),
              pos: new THREE.Vector3(-1.0, -1.0,  1.22), dx:  4, dy:  0 },
            { el: makeSpan(labels[2], 12, TEXT_COLOR, true),
              pos: new THREE.Vector3(-1.0,  0.65, -1.0), dx: -8, dy: -16 }
          ];
          axisLabels.forEach(function (item) { labelDiv.appendChild(item.el); });

          tickEls = tickData.map(function (td) {
            var span = makeSpan(td.label, 9, TEXT_LIGHT, false);
            labelDiv.appendChild(span);
            return span;
          });

          contourLabels = [];
          (d.level_values || []).forEach(function (level, li) {
            if (level <= d.min_u || level >= d.max_u) return;
            var hexStr = "#" + DARK2[li % DARK2.length].toString(16).padStart(6, "0");
            var span = makeSpan("u\u202f=\u202f" + _fmt(level), 9, hexStr, false);
            labelDiv.appendChild(span);
            contourLabels.push({
              el:  span,
              pos: new THREE.Vector3(1.18, norm.u(level), -1.0),
              dx:  4,
              dy: -6
            });
          });

          if (d.title) {
            var titleEl = makeSpan(d.title, 14, TEXT_COLOR, true);
            titleEl.style.top       = "10px";
            titleEl.style.left      = "50%";
            titleEl.style.transform = "translateX(-50%)";
            labelDiv.appendChild(titleEl);
          }


          /* ── D-pad ───────────────────────────────────────────────────────────── */
          var STEP = Math.PI / 18;   /* 10° per click */

          var ctrlDiv = document.createElement("div");
          ctrlDiv.style.cssText = [
            "position:absolute", "bottom:8px", "left:10px",
            "display:grid",
            "grid-template-columns:repeat(3,26px)",
            "grid-template-rows:repeat(3,26px)",
            "gap:2px"
          ].join(";");
          el.appendChild(ctrlDiv);

          function makeBtn(label, col, row, title, onClick) {
            var b = document.createElement("button");
            b.textContent = label;
            b.title = title;
            b.style.cssText = [
              "grid-column:" + col, "grid-row:" + row,
              "width:26px", "height:26px",
              "padding:0", "margin:0",
              "background:rgba(250,250,250,0.88)",
              "border:1px solid #bbb",
              "border-radius:3px",
              "font-family:" + FONT,
              "font-size:13px", "line-height:1",
              "color:#444", "cursor:pointer",
              "display:flex", "align-items:center", "justify-content:center"
            ].join(";");
            b.addEventListener("click", onClick);
            ctrlDiv.appendChild(b);
          }

          makeBtn("▲", 2, 1, "Tilt up (toward top view)",
            function () { phi -= STEP; applyCameraPosition(); });
          makeBtn("◀", 1, 2, "Orbit left",
            function () { theta -= STEP; applyCameraPosition(); });
          makeBtn("↺", 2, 2, "Reset to default view",
            function () { theta = INIT_THETA; phi = INIT_PHI; applyCameraPosition(); });
          makeBtn("▶", 3, 2, "Orbit right",
            function () { theta += STEP; applyCameraPosition(); });
          makeBtn("▼", 2, 3, "Tilt down (toward side view)",
            function () { phi += STEP; applyCameraPosition(); });

          /* ── Preset view buttons ─────────────────────────────────────────────── */
          var presetDiv = document.createElement("div");
          presetDiv.style.cssText = [
            "position:absolute", "bottom:90px", "left:10px",
            "display:flex", "flex-direction:column", "gap:3px"
          ].join(";");
          el.appendChild(presetDiv);

          function makePresetBtn(label, title, onClick) {
            var b = document.createElement("button");
            b.textContent = label;
            b.title = title;
            b.style.cssText = [
              "padding:2px 6px", "margin:0",
              "background:rgba(250,250,250,0.88)",
              "border:1px solid #bbb",
              "border-radius:3px",
              "font-family:" + FONT,
              "font-size:10px", "line-height:1.4",
              "color:#444", "cursor:pointer",
              "text-align:center"
            ].join(";");
            b.addEventListener("click", onClick);
            presetDiv.appendChild(b);
          }

          makePresetBtn("Top", "Bird's-eye — ICs as concentric rings",
            function () {
              phi = PHI_EPS;  theta = INIT_THETA;
              applyCameraPosition();
            });
          makePresetBtn("Front", "Side-on — utility profile",
            function () {
              phi = Math.PI / 2;  theta = 0;
              applyCameraPosition();
            });

          /* ── Mouse drag ──────────────────────────────────────────────────────── */
          var cvs = renderer.domElement;
          cvs.style.cursor = "grab";
          cvs.addEventListener("mousedown", function (e) {
            isDragging = true;
            lastMX = e.clientX; lastMY = e.clientY;
            cvs.style.cursor = "grabbing";
          });
          window.addEventListener("mouseup", function () {
            isDragging = false;
            if (cvs) cvs.style.cursor = "grab";
          });
          window.addEventListener("mousemove", function (e) {
            if (!isDragging) return;
            theta -= (e.clientX - lastMX) * 0.008;
            phi   += (e.clientY - lastMY) * 0.008;
            applyCameraPosition();
            lastMX = e.clientX; lastMY = e.clientY;
          });

          /* ── Animation loop ──────────────────────────────────────────────────── */
          var w = width, h = height;

          function project(worldPos) {
            var ndc = worldPos.clone().project(camera);
            return {
              x: ( ndc.x * 0.5 + 0.5) * w,
              y: (-ndc.y * 0.5 + 0.5) * h
            };
          }

          function animate() {
            animId = requestAnimationFrame(animate);
            renderer.render(scene, camera);

            axisLabels.forEach(function (item) {
              var s = project(item.pos);
              item.el.style.left = (s.x + item.dx) + "px";
              item.el.style.top  = (s.y + item.dy) + "px";
            });
            tickData.forEach(function (td, k) {
              var s = project(td.pos);
              tickEls[k].style.left = (s.x + td.dx) + "px";
              tickEls[k].style.top  = (s.y + td.dy) + "px";
            });
            contourLabels.forEach(function (item) {
              var s = project(item.pos);
              item.el.style.left = (s.x + item.dx) + "px";
              item.el.style.top  = (s.y + item.dy) + "px";
            });

          }
          animate();
        },

        resize: function (w, h) {
          width = w; height = h;
          if (!renderer) return;
          camera.aspect = w / h;
          camera.updateProjectionMatrix();
          renderer.setSize(w, h);
        }
      };
    }
  });
})();
