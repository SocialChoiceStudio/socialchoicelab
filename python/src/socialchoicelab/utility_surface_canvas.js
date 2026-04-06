/**
 * 3D utility surface widget using Three.js.
 * Renders a voter's utility function as a surface over 2D policy space.
 * No dependencies beyond Three.js (bundled locally) and htmlwidgets.
 *
 * Payload fields (set by R/Python plot_utility_surface()):
 *   xvals       float[]   N x-axis grid values
 *   yvals       float[]   N y-axis grid values
 *   z           float[]   N*N utility values, row-major (z[i*N+j] = u(xvals[i], yvals[j]))
 *   ideal_x     float     ideal point x coordinate
 *   ideal_y     float     ideal point y coordinate
 *   ideal_u     float     utility at ideal point (surface peak)
 *   min_u       float     minimum utility on grid
 *   max_u       float     maximum utility on grid
 *   level_values float[]  utility values for contour rings
 *   axis_labels  string[] [xLabel, yLabel, "Utility"] (3 elements)
 *   title        string   plot title (may be null)
 */
/* global HTMLWidgets, THREE */
(function () {
  "use strict";

  HTMLWidgets.widget({
    name: "utility_surface_canvas",
    type: "output",

    factory: function (el, width, height) {
      var renderer = null;
      var scene = null;
      var camera = null;
      var pivot = null;
      var animId = null;
      var labelDiv = null;

      var isDragging = false;
      var lastMX = 0, lastMY = 0;
      var rotY = 0.55;
      var rotX = -0.38;

      /* ── helpers ─────────────────────────────────────────────── */

      function cleanup() {
        if (animId) { cancelAnimationFrame(animId); animId = null; }
        if (renderer) {
          renderer.dispose();
          renderer.forceContextLoss();
        }
        el.innerHTML = "";
        labelDiv = null;
      }

      function lerpColor(t, lo, hi) {
        return new THREE.Color(
          lo.r + t * (hi.r - lo.r),
          lo.g + t * (hi.g - lo.g),
          lo.b + t * (hi.b - lo.b)
        );
      }

      /* ── surface geometry ────────────────────────────────────── */

      function buildSurface(d, norm) {
        var N = d.xvals.length;
        var z = d.z;
        var positions = new Float32Array(N * N * 3);
        var colors    = new Float32Array(N * N * 3);

        var colorLo = new THREE.Color(0x3a6bbf);
        var colorHi = new THREE.Color(0xde3a2f);

        for (var i = 0; i < N; i++) {
          for (var j = 0; j < N; j++) {
            var idx = i * N + j;
            var u = z[idx];
            var px = norm.x(d.xvals[i]);
            var py = norm.u(u);
            var pz = norm.y(d.yvals[j]);
            positions[idx * 3]     = px;
            positions[idx * 3 + 1] = py;
            positions[idx * 3 + 2] = pz;
            var t = norm.t(u);
            var c = lerpColor(t, colorLo, colorHi);
            colors[idx * 3]     = c.r;
            colors[idx * 3 + 1] = c.g;
            colors[idx * 3 + 2] = c.b;
          }
        }

        var indices = [];
        for (var ii = 0; ii < N - 1; ii++) {
          for (var jj = 0; jj < N - 1; jj++) {
            var a = ii * N + jj;
            var b = (ii + 1) * N + jj;
            var c = ii * N + (jj + 1);
            var dd = (ii + 1) * N + (jj + 1);
            indices.push(a, b, c);
            indices.push(b, dd, c);
          }
        }

        var geo = new THREE.BufferGeometry();
        geo.setAttribute("position", new THREE.BufferAttribute(positions, 3));
        geo.setAttribute("color",    new THREE.BufferAttribute(colors, 3));
        geo.setIndex(indices);
        geo.computeVertexNormals();

        var mat = new THREE.MeshPhongMaterial({
          vertexColors: true,
          side: THREE.DoubleSide,
          shininess: 40,
          specular: new THREE.Color(0x222222)
        });

        return new THREE.Mesh(geo, mat);
      }

      /* ── contour rings (marching squares) ───────────────────── */

      function buildContours(d, norm) {
        var N = d.xvals.length;
        var z = d.z;
        var group = new THREE.Group();
        var ringColors = [0xe41a1c, 0xff7f00, 0x4daf4a, 0x377eb8, 0x984ea3,
                          0xa65628, 0xf781bf];
        var OFFSET = 0.003;

        d.level_values.forEach(function (level, li) {
          if (level <= d.min_u || level >= d.max_u) return;
          var pts = [];

          for (var i = 0; i < N - 1; i++) {
            for (var j = 0; j < N - 1; j++) {
              var z00 = z[i * N + j];
              var z10 = z[(i + 1) * N + j];
              var z01 = z[i * N + (j + 1)];
              var z11 = z[(i + 1) * N + (j + 1)];

              var edge = [];
              function tryEdge(x0, y0, u0, x1, y1, u1) {
                if ((u0 < level) === (u1 < level)) return;
                var t = (level - u0) / (u1 - u0);
                edge.push(new THREE.Vector3(
                  norm.x(x0 + t * (x1 - x0)),
                  norm.u(level) + OFFSET,
                  norm.y(y0 + t * (y1 - y0))
                ));
              }
              tryEdge(d.xvals[i],   d.yvals[j],   z00, d.xvals[i+1], d.yvals[j],   z10);
              tryEdge(d.xvals[i+1], d.yvals[j],   z10, d.xvals[i+1], d.yvals[j+1], z11);
              tryEdge(d.xvals[i],   d.yvals[j+1], z01, d.xvals[i+1], d.yvals[j+1], z11);
              tryEdge(d.xvals[i],   d.yvals[j],   z00, d.xvals[i],   d.yvals[j+1], z01);

              if (edge.length === 2) {
                pts.push(edge[0], edge[1]);
              }
            }
          }

          if (pts.length === 0) return;
          var geo = new THREE.BufferGeometry().setFromPoints(pts);
          var mat = new THREE.LineBasicMaterial({
            color: ringColors[li % ringColors.length],
            linewidth: 2
          });
          group.add(new THREE.LineSegments(geo, mat));
        });

        return group;
      }

      /* ── axes ────────────────────────────────────────────────── */

      function buildAxes(norm) {
        var group = new THREE.Group();
        var origin = new THREE.Vector3(-1.05, -0.02, -1.05);
        var len = 2.15;
        var uLen = 1.55;
        var color = 0x444444;

        group.add(new THREE.ArrowHelper(
          new THREE.Vector3(1, 0, 0), origin, len, color, 0.08, 0.04));
        group.add(new THREE.ArrowHelper(
          new THREE.Vector3(0, 1, 0), origin, uLen, color, 0.08, 0.04));
        group.add(new THREE.ArrowHelper(
          new THREE.Vector3(0, 0, 1), origin, len, color, 0.08, 0.04));

        return group;
      }

      /* ── floor grid ──────────────────────────────────────────── */

      function buildFloor() {
        var geo = new THREE.PlaneGeometry(2, 2, 8, 8);
        var mat = new THREE.MeshBasicMaterial({
          color: 0xe8e8e8,
          transparent: true,
          opacity: 0.35,
          side: THREE.DoubleSide
        });
        var plane = new THREE.Mesh(geo, mat);
        plane.rotation.x = -Math.PI / 2;
        plane.position.y = -0.02;

        var edges = new THREE.EdgesGeometry(geo);
        var lineMat = new THREE.LineBasicMaterial({ color: 0xbbbbbb, transparent: true, opacity: 0.4 });
        var grid = new THREE.LineSegments(edges, lineMat);
        grid.rotation.x = -Math.PI / 2;
        grid.position.y = -0.02;

        var group = new THREE.Group();
        group.add(plane);
        group.add(grid);
        return group;
      }

      /* ── ideal point marker ──────────────────────────────────── */

      function buildIdealMarker(d, norm) {
        var group = new THREE.Group();

        var peakX = norm.x(d.ideal_x);
        var peakY = norm.u(d.ideal_u);
        var peakZ = norm.y(d.ideal_y);

        var sphere = new THREE.Mesh(
          new THREE.SphereGeometry(0.045, 16, 16),
          new THREE.MeshPhongMaterial({ color: 0x111111 })
        );
        sphere.position.set(peakX, peakY + 0.005, peakZ);
        group.add(sphere);

        var floorSphere = new THREE.Mesh(
          new THREE.SphereGeometry(0.025, 12, 12),
          new THREE.MeshPhongMaterial({ color: 0x111111 })
        );
        floorSphere.position.set(peakX, -0.02, peakZ);
        group.add(floorSphere);

        var linePts = [
          new THREE.Vector3(peakX, -0.02, peakZ),
          new THREE.Vector3(peakX, peakY, peakZ)
        ];
        var lineGeo = new THREE.BufferGeometry().setFromPoints(linePts);
        var lineMat = new THREE.LineDashedMaterial({
          color: 0x444444, dashSize: 0.06, gapSize: 0.04
        });
        var line = new THREE.Line(lineGeo, lineMat);
        line.computeLineDistances();
        group.add(line);

        return group;
      }

      /* ── HTML label overlay ──────────────────────────────────── */

      function makeLabel(text, fontSize, color) {
        var span = document.createElement("span");
        span.textContent = text;
        span.style.cssText = [
          "position:absolute",
          "pointer-events:none",
          "font-family:sans-serif",
          "font-size:" + (fontSize || 12) + "px",
          "color:" + (color || "#333"),
          "white-space:nowrap",
          "background:rgba(250,250,250,0.0)"
        ].join(";");
        return span;
      }

      function project3D(vec3, cam, w, h) {
        var v = vec3.clone().project(cam);
        return {
          x: (v.x * 0.5 + 0.5) * w,
          y: (-v.y * 0.5 + 0.5) * h
        };
      }

      function updateLabels(d, norm, cam, w, h, labelData) {
        labelData.forEach(function (item) {
          var s = project3D(item.pos, cam, w, h);
          item.el.style.left = (s.x + item.dx) + "px";
          item.el.style.top  = (s.y + item.dy) + "px";
        });
      }

      /* ── main render ─────────────────────────────────────────── */

      return {
        renderValue: function (d) {
          cleanup();

          var N = d.xvals.length;
          var xRange = d.xvals[N-1] - d.xvals[0] || 1;
          var yRange = d.yvals[N-1] - d.yvals[0] || 1;
          var uRange = d.max_u - d.min_u || 1;

          var norm = {
            x: function (v) { return (v - d.xvals[0]) / xRange * 2 - 1; },
            y: function (v) { return (v - d.yvals[0]) / yRange * 2 - 1; },
            u: function (u) { return (u - d.min_u) / uRange * 1.5; },
            t: function (u) { return (u - d.min_u) / uRange; }
          };

          /* scene */
          scene = new THREE.Scene();
          scene.background = new THREE.Color(0xfafafa);

          camera = new THREE.PerspectiveCamera(38, width / height, 0.01, 100);
          camera.position.set(3.5, 2.2, 3.5);
          camera.lookAt(0, 0.5, 0);

          renderer = new THREE.WebGLRenderer({ antialias: true });
          renderer.setPixelRatio(window.devicePixelRatio || 1);
          renderer.setSize(width, height);
          el.style.position = "relative";
          el.appendChild(renderer.domElement);

          /* lighting */
          scene.add(new THREE.AmbientLight(0xffffff, 0.55));
          var dl = new THREE.DirectionalLight(0xffffff, 0.75);
          dl.position.set(4, 6, 4);
          scene.add(dl);
          var dl2 = new THREE.DirectionalLight(0xffffff, 0.2);
          dl2.position.set(-3, 2, -3);
          scene.add(dl2);

          /* pivot group — everything inside rotates together */
          pivot = new THREE.Group();
          scene.add(pivot);

          pivot.add(buildFloor());
          pivot.add(buildSurface(d, norm));
          pivot.add(buildContours(d, norm));
          pivot.add(buildAxes(norm));
          pivot.add(buildIdealMarker(d, norm));

          pivot.rotation.x = rotX;
          pivot.rotation.y = rotY;

          /* HTML label overlay */
          labelDiv = document.createElement("div");
          labelDiv.style.cssText = "position:absolute;top:0;left:0;width:100%;height:100%;pointer-events:none;";
          el.appendChild(labelDiv);

          var labels = d.axis_labels || ["Dimension 1", "Dimension 2", "Utility"];
          var labelData = [
            { el: makeLabel(labels[0], 13, "#333"), pos: new THREE.Vector3(1.15, -0.02, -1.1), dx: 4, dy: 0 },
            { el: makeLabel(labels[1], 13, "#333"), pos: new THREE.Vector3(-1.1, -0.02, 1.15), dx: 4, dy: 0 },
            { el: makeLabel(labels[2], 13, "#333"), pos: new THREE.Vector3(-1.1, 1.55, -1.1), dx: -8, dy: -18 }
          ];
          labelData.forEach(function (item) { labelDiv.appendChild(item.el); });

          if (d.title) {
            var titleEl = makeLabel(d.title, 15, "#222");
            titleEl.style.cssText += ";font-weight:600;top:10px;left:50%;transform:translateX(-50%)";
            labelDiv.appendChild(titleEl);
          }

          /* mouse drag rotation */
          var cvs = renderer.domElement;
          cvs.addEventListener("mousedown", function (e) {
            isDragging = true;
            lastMX = e.clientX;
            lastMY = e.clientY;
          });
          window.addEventListener("mouseup", function () { isDragging = false; });
          window.addEventListener("mousemove", function (e) {
            if (!isDragging) return;
            var dx = e.clientX - lastMX;
            var dy = e.clientY - lastMY;
            rotY += dx * 0.010;
            rotX += dy * 0.010;
            rotX = Math.max(-Math.PI / 2, Math.min(0.1, rotX));
            pivot.rotation.x = rotX;
            pivot.rotation.y = rotY;
            lastMX = e.clientX;
            lastMY = e.clientY;
          });

          /* animation loop */
          var w = width, h = height;
          function animate() {
            animId = requestAnimationFrame(animate);
            updateLabels(d, norm, camera, w, h, labelData);
            renderer.render(scene, camera);
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
