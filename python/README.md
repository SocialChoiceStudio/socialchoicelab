# socialchoicelab — Python Binding

Python bindings for the `socialchoicelab` C library, using `cffi` in ABI mode.

## Local development setup

### 1. Build the C library (one-time, or after C source changes)

Run from the repository root:

```bash
cmake -S . -B build
cmake --build build
```

This produces `build/libscs_api.dylib` (macOS) or `build/libscs_api.so` (Linux).

### 2. Install the Python package in editable mode (one-time)

```bash
cd python
pip install -e ".[dev]"
```

The `-e` flag means editable: changes to `src/socialchoicelab/` take effect
immediately without reinstalling.

### 3. Set the library path environment variable

The package needs to find `libscs_api` at runtime. Set `SCS_LIB_PATH` to the
`build/` directory:

```bash
export SCS_LIB_PATH=/path/to/socialchoicelab/build
```

Add this to your shell profile (`~/.zshrc` or `~/.bash_profile`) so it persists
across sessions.

### 4. Verify the setup

```bash
python -c "import socialchoicelab as scl; print(scl.scs_version())"
```

You should see something like `{'major': 0, 'minor': 1, 'patch': 0}`.

## Interactive exploration

Open `notebooks/spatial_voting.ipynb` in Cursor (or JupyterLab) and run cells
with Shift+Enter. The notebook walks through distance, utility, majority
preference, winsets, voting rules, and the stream manager.

## Running tests

```bash
cd python
pytest
```

## After C library changes

If you modify C source and rebuild `libscs_api`, no reinstall is needed —
just restart your Python kernel/session so the new `.dylib` is loaded.
