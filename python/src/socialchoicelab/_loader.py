"""cffi ABI-mode loader for libscs_api.

Locates and opens the pre-built libscs_api shared library, then parses the
C declarations from scs_api.h so that all C API functions are callable via
the module-level ``_lib`` and ``_ffi`` singletons.

Library search order
--------------------
1. ``SCS_LIB_PATH`` environment variable (directory containing the library).
2. Standard system library paths (LD_LIBRARY_PATH / DYLD_LIBRARY_PATH /
   PATH depending on platform).

Set ``SCS_LIB_PATH`` to the ``build/`` directory of the socialchoicelab
C++ project for local development, or to the directory where the pre-built
library artifact was downloaded in CI.
"""

from __future__ import annotations

import os
import sys
from pathlib import Path

import cffi

# ---------------------------------------------------------------------------
# Locate libscs_api
# ---------------------------------------------------------------------------

def _find_library() -> str:
    """Return the path to libscs_api, or raise ImportError."""
    if sys.platform == "win32":
        lib_names = ["scs_api.dll"]
    elif sys.platform == "darwin":
        lib_names = ["libscs_api.dylib"]
    else:
        lib_names = ["libscs_api.so"]

    # 1. SCS_LIB_PATH environment variable.
    env_path = os.environ.get("SCS_LIB_PATH")
    if env_path:
        for name in lib_names:
            candidate = Path(env_path) / name
            if candidate.exists():
                return str(candidate)
        raise ImportError(
            f"SCS_LIB_PATH is set to '{env_path}' but none of "
            f"{lib_names} were found there. "
            f"Check that libscs_api has been built (cmake --build build) "
            f"and that SCS_LIB_PATH points to the directory containing it."
        )

    # 2. Let cffi try to find it on the system path.
    # This covers installed distributions where the library is on LD_LIBRARY_PATH.
    for name in lib_names:
        try:
            ffi_probe = cffi.FFI()
            lib = ffi_probe.dlopen(name)
            return name  # cffi resolved it; use the bare name
        except OSError:
            continue

    raise ImportError(
        f"Could not locate libscs_api ({lib_names}). "
        f"Set the SCS_LIB_PATH environment variable to the directory "
        f"containing the library, e.g.:\n"
        f"  export SCS_LIB_PATH=/path/to/socialchoicelab/build"
    )


# ---------------------------------------------------------------------------
# Parse C declarations
# ---------------------------------------------------------------------------

def _load_declarations() -> str:
    """Return the C declarations needed by cffi.

    These are extracted from scs_api.h with SCS_API and other macros stripped.
    Kept as a string here so that cffi ABI mode does not require a C compiler.
    Populated fully in phase B3/B4; this stub declares only what is needed to
    verify the library loads correctly.
    """
    return """
        /* Stub — full declarations added in phases B3/B4. */
        int scs_api_version(int* out_major, int* out_minor, int* out_patch,
                            char* err_buf, int err_buf_len);
    """


# ---------------------------------------------------------------------------
# Module-level singletons
# ---------------------------------------------------------------------------

_ffi = cffi.FFI()
_ffi.cdef(_load_declarations())

try:
    _lib_path = _find_library()
    _lib = _ffi.dlopen(_lib_path)
except ImportError as exc:
    raise ImportError(str(exc)) from None
