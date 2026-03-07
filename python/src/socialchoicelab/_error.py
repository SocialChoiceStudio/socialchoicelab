"""Exception hierarchy for socialchoicelab.

Every C API call returns an int status code. The ``_check`` helper maps
non-zero codes to the appropriate exception subclass and raises it with the
message from err_buf.
"""

from __future__ import annotations

# C API return codes (must match scs_api.h).
_SCS_OK = 0
_SCS_ERROR_INVALID_ARGUMENT = 1
_SCS_ERROR_INTERNAL = 2
_SCS_ERROR_BUFFER_TOO_SMALL = 3
_SCS_ERROR_OUT_OF_MEMORY = 4

_ERR_BUF_SIZE = 512


class SCSError(RuntimeError):
    """Base class for all socialchoicelab errors."""


class SCSInvalidArgumentError(SCSError):
    """Raised when a C API call returns SCS_ERROR_INVALID_ARGUMENT."""


class SCSInternalError(SCSError):
    """Raised when a C API call returns SCS_ERROR_INTERNAL."""


class SCSBufferTooSmallError(SCSError):
    """Raised when a C API call returns SCS_ERROR_BUFFER_TOO_SMALL."""


class SCSOutOfMemoryError(SCSError):
    """Raised when a C API call returns SCS_ERROR_OUT_OF_MEMORY."""


_CODE_TO_EXCEPTION: dict[int, type[SCSError]] = {
    _SCS_ERROR_INVALID_ARGUMENT: SCSInvalidArgumentError,
    _SCS_ERROR_INTERNAL: SCSInternalError,
    _SCS_ERROR_BUFFER_TOO_SMALL: SCSBufferTooSmallError,
    _SCS_ERROR_OUT_OF_MEMORY: SCSOutOfMemoryError,
}


def _check(rc: int, err_buf) -> None:
    """Raise the appropriate SCSError if rc != SCS_OK.

    Parameters
    ----------
    rc:
        Return code from a C API function.
    err_buf:
        cffi ``char*`` buffer that was passed to the C API call and may
        contain a null-terminated error message.
    """
    if rc == _SCS_OK:
        return
    try:
        msg = _ffi_string(err_buf)
    except Exception:
        msg = f"C API error (code {rc})"
    exc_type = _CODE_TO_EXCEPTION.get(rc, SCSError)
    raise exc_type(msg)


def _ffi_string(buf) -> str:
    """Decode a cffi char* buffer to a Python str."""
    from socialchoicelab._loader import _ffi
    raw = _ffi.string(buf)
    return raw.decode("utf-8", errors="replace")


def new_err_buf():
    """Allocate a fresh error buffer for passing to C API calls."""
    from socialchoicelab._loader import _ffi
    return _ffi.new("char[]", _ERR_BUF_SIZE)
