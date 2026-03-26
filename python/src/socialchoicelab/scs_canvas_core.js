/**
 * Shared Canvas 2D primitives for SocialChoiceLab htmlwidgets.
 * Load before competition_canvas.js or spatial_voting_canvas.js.
 * Exposes window.ScsCanvasCore.
 */
(function(global) {
  "use strict";

  var FONT = "system-ui, -apple-system, BlinkMacSystemFont, sans-serif";
  var MONO = "ui-monospace, 'Cascadia Code', Menlo, monospace";

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

  var OVERLAY_COLORS = {
    centroid:        { pt: "rgba(185,10,10,0.95)",   stroke: "rgba(255,255,255,0.85)" },
    marginal_median: { pt: "rgba(100,0,180,0.90)",   stroke: "rgba(255,255,255,0.85)" },
    geometric_mean:  { pt: "rgba(100,55,0,0.95)",    stroke: "rgba(255,255,255,0.85)" },
    pareto_set:      { fill: "rgba(120,0,220,0.08)", border: "rgba(120,0,220,0.50)" }
  };

  var OVERLAY_LABELS = {
    centroid:        "Centroid",
    marginal_median: "Marginal Median",
    geometric_mean:  "Geometric Mean",
    pareto_set:      "Pareto Set"
  };

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

  /** Data x → pixel (2D square plot). */
  function xToPx2d(x, viewXMin, viewXMax, padLeft, plotSide) {
    var t = (x - viewXMin) / (viewXMax - viewXMin);
    return padLeft + t * plotSide;
  }

  /** Data y → pixel (2D, y up in data space). */
  function yToPx2d(y, viewYMin, viewYMax, padTop, plotSide) {
    var t = (y - viewYMin) / (viewYMax - viewYMin);
    return padTop + (1 - t) * plotSide;
  }

  function pxToX2d(cssPx, viewXMin, viewXMax, padLeft, plotSide) {
    return viewXMin + (cssPx - padLeft) / plotSide * (viewXMax - viewXMin);
  }

  function pxToY2d(cssPy, viewYMin, viewYMax, padTop, plotSide) {
    return viewYMin + (1 - (cssPy - padTop) / plotSide) * (viewYMax - viewYMin);
  }

  function xToPx1d(x, viewXMin, viewXMax, padLeft, plotW) {
    var t = (x - viewXMin) / (viewXMax - viewXMin);
    return padLeft + t * plotW;
  }

  function drawDiamond(ctx, px, py, size, fill, shadow) {
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
  }

  /** Ray-cast point-in-polygon; xy alternating x0,y0,x1,y1,... closed or open. */
  function pointInPolygon(px, py, flatXY) {
    if (!flatXY || flatXY.length < 6) return false;
    var n = flatXY.length / 2;
    var inside = false;
    for (var i = 0, j = n - 1; i < n; j = i++) {
      var xi = flatXY[i * 2], yi = flatXY[i * 2 + 1];
      var xj = flatXY[j * 2], yj = flatXY[j * 2 + 1];
      var inter = ((yi > py) !== (yj > py)) &&
        (px < (xj - xi) * (py - yi) / (yj - yi + 1e-20) + xi);
      if (inter) inside = !inside;
    }
    return inside;
  }

  global.ScsCanvasCore = {
    FONT: FONT,
    MONO: MONO,
    COLORS: COLORS,
    OVERLAY_COLORS: OVERLAY_COLORS,
    OVERLAY_LABELS: OVERLAY_LABELS,
    parseRGBA: parseRGBA,
    rgba: rgba,
    formatTick: formatTick,
    xToPx2d: xToPx2d,
    yToPx2d: yToPx2d,
    pxToX2d: pxToX2d,
    pxToY2d: pxToY2d,
    xToPx1d: xToPx1d,
    drawDiamond: drawDiamond,
    pointInPolygon: pointInPolygon
  };
})(typeof window !== "undefined" ? window : this);
