# test_palette.R — C13.4: Unit tests for palette and theme utilities

# ---------------------------------------------------------------------------
# scl_palette
# ---------------------------------------------------------------------------

test_that("scl_palette returns correct number of RGBA strings", {
  colors <- scl_palette("dark2", n = 4L)
  expect_length(colors, 4L)
  expect_type(colors, "character")
  expect_true(all(grepl("^rgba\\(", colors)))
})

test_that("scl_palette auto selects okabe_ito for n <= 7", {
  colors_auto <- scl_palette("auto", n = 5L)
  colors_oi   <- scl_palette("okabe_ito", n = 5L)
  expect_identical(colors_auto, colors_oi)
})

test_that("scl_palette auto selects dark2 for n = 8", {
  colors_auto <- scl_palette("auto", n = 8L)
  colors_d2   <- scl_palette("dark2", n = 8L)
  expect_identical(colors_auto, colors_d2)
})

test_that("scl_palette auto selects paired for n > 8", {
  colors_auto   <- scl_palette("auto", n = 10L)
  colors_paired <- scl_palette("paired", n = 10L)
  expect_identical(colors_auto, colors_paired)
})

test_that("scl_palette cycles when n exceeds palette size", {
  colors <- scl_palette("okabe_ito", n = 10L)
  expect_length(colors, 10L)
  expect_identical(colors[1L], colors[8L])  # 7-color palette: slot 1 == slot 8
})

test_that("scl_palette respects alpha argument", {
  colors <- scl_palette("dark2", n = 2L, alpha = 0.5)
  expect_true(all(grepl(",0.5\\)", colors)))
})

test_that("scl_palette errors on unknown palette", {
  expect_error(scl_palette("nonexistent", n = 3L), "unknown palette")
})

test_that("scl_palette supports all named palettes", {
  for (pal in c("dark2", "set2", "okabe_ito", "paired")) {
    colors <- scl_palette(pal, n = 4L)
    expect_length(colors, 4L)
  }
})

# ---------------------------------------------------------------------------
# scl_theme_colors
# ---------------------------------------------------------------------------

test_that("scl_theme_colors returns named list with fill and line", {
  cols <- scl_theme_colors("winset", theme = "dark2")
  expect_type(cols, "list")
  expect_named(cols, c("fill", "line"))
  expect_match(cols$fill, "^rgba\\(")
  expect_match(cols$line, "^rgba\\(")
})

test_that("scl_theme_colors fill has lower alpha than line", {
  cols <- scl_theme_colors("winset", theme = "dark2")
  # Extract alpha values from rgba strings
  fill_alpha <- as.numeric(sub(".*,(.*?)\\)$", "\\1", cols$fill))
  line_alpha <- as.numeric(sub(".*,(.*?)\\)$", "\\1", cols$line))
  expect_lt(fill_alpha, line_alpha)
})

test_that("scl_theme_colors bw theme returns grays", {
  cols <- scl_theme_colors("yolk", theme = "bw")
  expect_match(cols$fill, "rgba\\(200,200,200")
  expect_match(cols$line, "rgba\\(30,30,30")
})

test_that("scl_theme_colors works for all layer types", {
  layer_types <- c("winset", "yolk", "uncovered_set", "convex_hull",
                   "voters", "alternatives")
  for (lt in layer_types) {
    cols <- scl_theme_colors(lt, theme = "dark2")
    expect_named(cols, c("fill", "line"), label = paste("layer:", lt))
  }
})

test_that("scl_theme_colors works for all themes", {
  for (th in c("dark2", "set2", "okabe_ito", "paired", "bw")) {
    cols <- scl_theme_colors("winset", theme = th)
    expect_type(cols, "list")
  }
})

test_that("scl_theme_colors errors on unknown theme", {
  expect_error(scl_theme_colors("winset", theme = "neon"), "unknown theme")
})

test_that("different layer types get different colours", {
  fill_ws <- scl_theme_colors("winset",        theme = "dark2")$fill
  fill_yk <- scl_theme_colors("yolk",           theme = "dark2")$fill
  fill_us <- scl_theme_colors("uncovered_set",  theme = "dark2")$fill
  fill_ch <- scl_theme_colors("convex_hull",    theme = "dark2")$fill
  fills <- c(fill_ws, fill_yk, fill_us, fill_ch)
  expect_equal(length(unique(fills)), 4L)
})

# ---------------------------------------------------------------------------
# Integration: layer functions use theme colours
# ---------------------------------------------------------------------------

test_that("plot_spatial_voting returns canvas widget with dark2 theme", {
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
  fig <- plot_spatial_voting(voters, sq = c(0.0, 0.0), theme = "dark2")
  expect_s3_class(fig, "spatial_voting_canvas")
  expect_true(grepl("^rgba\\(", fig$x$theme_colors$winset_fill))
})

test_that("plot_spatial_voting returns canvas widget with bw theme", {
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
  fig <- plot_spatial_voting(voters, sq = c(0.0, 0.0), theme = "bw")
  expect_s3_class(fig, "spatial_voting_canvas")
  expect_identical(fig$x$theme, "bw")
})

test_that("layer_winset respects explicit fill_color override", {
  skip_without_lib()
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6, -0.4, 0.8, 0.5, -0.7)
  sq     <- c(0.0, 0.0)
  ws     <- winset_2d(sq, voters)
  fig    <- plot_spatial_voting(voters, sq = sq)
  fig2   <- layer_winset(fig, ws, fill_color = "rgba(255,0,0,0.2)")
  paths  <- fig2$x$layers$winset_paths
  expect_true(length(paths) > 0L)
  expect_true(any(vapply(paths, function(p) identical(p$fill, "rgba(255,0,0,0.2)"), logical(1L))))
})

test_that("layer_ic color_by_voter uses palette", {
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
  sq     <- c(0.0, 0.0)
  fig    <- plot_spatial_voting(voters, sq = sq)
  fig2   <- layer_ic(fig, voters, sq, color_by_voter = TRUE, palette = "dark2")
  expect_length(fig2$x$layers$ic_curves, 3L)
})

test_that("layer_ic color_by_voter auto palette uses theme", {
  voters <- c(-1.0, -0.5, 0.0, 0.0, 0.8, 0.6)
  sq     <- c(0.0, 0.0)
  fig    <- plot_spatial_voting(voters, sq = sq, theme = "set2")
  fig2   <- layer_ic(fig, voters, sq, color_by_voter = TRUE, theme = "set2")
  expect_length(fig2$x$layers$ic_curves, 3L)
})
