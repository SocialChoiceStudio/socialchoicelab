# socialchoicelab — R Binding

R bindings for the `socialchoicelab` C library, using `.Call()` via a
registered C entry-point table.

## Local development setup

### 1. Build the C library (one-time, or after C source changes)

Run from the repository root:

```bash
cmake -S . -B build
cmake --build build
```

This produces `build/libscs_api.dylib` (macOS) or `build/libscs_api.so` (Linux).

### 2. Set environment variables (one-time)

The package needs two variables at compile time and one at runtime. Add these
to `r/.Renviron` (create it if it does not exist):

```
SCS_LIB_PATH=/path/to/socialchoicelab/build
SCS_INCLUDE_PATH=/path/to/socialchoicelab/include
DYLD_LIBRARY_PATH=/path/to/socialchoicelab/build
```

Replace `/path/to/socialchoicelab` with the actual repo root. RStudio reads
`r/.Renviron` automatically when the RStudio project (`r/r.Rproj`) is open.

### 3. Open the RStudio project

Open `r/r.Rproj` in RStudio. This sets the working directory and loads `.Renviron`.

### 4. Install the package

Run at the RStudio console:

```r
devtools::install()
```

You should see `* DONE (socialchoicelab)` at the end.

### 5. Verify the setup

```r
library(socialchoicelab)
calculate_distance(c(0, 0), c(3, 4))  # should return 5
```

## Interactive exploration

Open `devScript_MAIN.R` in RStudio and run lines with Cmd+Enter (or
Ctrl+Enter). The script walks through all major user-facing functions.

## Running tests

```r
devtools::test()
```

## After C library changes

Rebuild `libscs_api` first, then reinstall the R package:

```r
devtools::install()
```

A full session restart (Cmd+Shift+F10) is recommended to unload the old DLL
before reinstalling.

## After R source changes only (no C changes)

```r
devtools::install()
```

Or for a faster iteration loop without rebuilding:

```r
devtools::load_all()
```
