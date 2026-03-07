# socialchoicelab — spatial social choice analysis.
#
# Public API is imported here as phases B3–B5 are implemented.
# For now, expose only the version and the library loader check.

from socialchoicelab._loader import _lib, _ffi  # noqa: F401 (eagerly loads libscs_api)

__version__ = "0.1.0"
