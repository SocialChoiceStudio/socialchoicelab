"""StreamManager: reproducible pseudo-random number streams.

Wraps the ``SCS_StreamManager`` C handle. All PRNG calls go through named
streams derived from a single master seed, enabling full reproducibility.
Register stream names before drawing from them.

Typical usage::

    with StreamManager(master_seed=42, stream_names=["voters", "alts"]) as sm:
        x = sm.uniform_real("voters", -1.0, 1.0)
        n = sm.uniform_int("alts", 0, 4)

The class implements the context-manager protocol for deterministic cleanup
and also registers a ``__del__`` guard as a safety net.
"""

from __future__ import annotations

from socialchoicelab._error import SCSError, _check, new_err_buf
from socialchoicelab._loader import _ffi, _lib

__all__ = ["StreamManager"]

_ERR_BUF_SIZE = 512


class StreamManager:
    """Wraps an ``SCS_StreamManager`` opaque handle.

    All PRNG draws go through named streams derived from a single master
    seed, enabling exact reproducibility. Register stream names before
    drawing from them.

    Parameters
    ----------
    master_seed:
        Master seed for all streams. Values up to 2**53 are represented
        exactly as ``uint64_t``.
    stream_names:
        Optional list of stream names to register immediately after
        creation.
    """

    def __init__(self, master_seed: int | float, stream_names: list[str] | None = None) -> None:
        err = new_err_buf()
        ptr = _lib.scs_stream_manager_create(int(master_seed), err, _ERR_BUF_SIZE)
        if ptr == _ffi.NULL:
            from socialchoicelab._error import _ffi_string
            raise SCSError(
                f"scs_stream_manager_create failed: {_ffi_string(err)}"
            )
        self._ptr = ptr
        self._destroyed = False
        if stream_names is not None:
            self.register(stream_names)

    def __del__(self) -> None:
        self._destroy()

    def __enter__(self) -> "StreamManager":
        return self

    def __exit__(self, *_) -> None:
        self._destroy()

    def _destroy(self) -> None:
        if not self._destroyed and self._ptr != _ffi.NULL:
            _lib.scs_stream_manager_destroy(self._ptr)
            self._ptr = _ffi.NULL
            self._destroyed = True

    # ------------------------------------------------------------------
    # Stream management
    # ------------------------------------------------------------------

    def register(self, stream_names: list[str]) -> "StreamManager":
        """Register allowed stream names.

        Only registered names may be used with PRNG draw methods.

        Parameters
        ----------
        stream_names:
            List of stream name strings to register.

        Returns
        -------
        self
        """
        encoded = [s.encode() for s in stream_names]
        arr = _ffi.new("const char *[]", [_ffi.new("char[]", s) for s in encoded])
        err = new_err_buf()
        _check(_lib.scs_register_streams(self._ptr, arr, len(stream_names), err, _ERR_BUF_SIZE), err)
        return self

    def reset_all(self, master_seed: int | float) -> "StreamManager":
        """Reset all streams with a new master seed.

        Parameters
        ----------
        master_seed:
            New master seed.

        Returns
        -------
        self
        """
        err = new_err_buf()
        _check(_lib.scs_reset_all(self._ptr, int(master_seed), err, _ERR_BUF_SIZE), err)
        return self

    def reset_stream(self, stream_name: str, seed: int | float) -> "StreamManager":
        """Reset a single named stream to a new seed.

        Parameters
        ----------
        stream_name:
            Name of the stream to reset.
        seed:
            New seed for this stream.

        Returns
        -------
        self
        """
        err = new_err_buf()
        _check(
            _lib.scs_reset_stream(self._ptr, stream_name.encode(), int(seed), err, _ERR_BUF_SIZE),
            err,
        )
        return self

    def skip(self, stream_name: str, n: int) -> "StreamManager":
        """Discard *n* values from a named stream.

        Parameters
        ----------
        stream_name:
            The stream to advance.
        n:
            Number of draws to discard.

        Returns
        -------
        self
        """
        err = new_err_buf()
        _check(
            _lib.scs_skip(self._ptr, stream_name.encode(), int(n), err, _ERR_BUF_SIZE),
            err,
        )
        return self

    # ------------------------------------------------------------------
    # PRNG draw methods
    # ------------------------------------------------------------------

    def uniform_real(self, stream_name: str, min: float = 0.0, max: float = 1.0) -> float:
        """Draw a uniform real number in [min, max).

        Parameters
        ----------
        stream_name:
            Named stream to draw from.
        min:
            Lower bound (inclusive).
        max:
            Upper bound (exclusive).

        Returns
        -------
        float
        """
        out = _ffi.new("double *")
        err = new_err_buf()
        _check(
            _lib.scs_uniform_real(
                self._ptr, stream_name.encode(), float(min), float(max), out, err, _ERR_BUF_SIZE
            ),
            err,
        )
        return float(out[0])

    def normal(self, stream_name: str, mean: float = 0.0, stddev: float = 1.0) -> float:
        """Draw a normal variate N(mean, stddev²).

        Parameters
        ----------
        stream_name:
            Named stream to draw from.
        mean:
            Distribution mean.
        stddev:
            Distribution standard deviation (must be > 0).

        Returns
        -------
        float
        """
        out = _ffi.new("double *")
        err = new_err_buf()
        _check(
            _lib.scs_normal(
                self._ptr, stream_name.encode(), float(mean), float(stddev), out, err, _ERR_BUF_SIZE
            ),
            err,
        )
        return float(out[0])

    def bernoulli(self, stream_name: str, probability: float) -> bool:
        """Draw a Bernoulli(probability) variate.

        Parameters
        ----------
        stream_name:
            Named stream to draw from.
        probability:
            Success probability in [0, 1].

        Returns
        -------
        bool
        """
        out = _ffi.new("int *")
        err = new_err_buf()
        _check(
            _lib.scs_bernoulli(
                self._ptr, stream_name.encode(), float(probability), out, err, _ERR_BUF_SIZE
            ),
            err,
        )
        return bool(out[0])

    def uniform_int(self, stream_name: str, min: int, max: int) -> int:
        """Draw a uniform integer in [min, max] (inclusive on both ends).

        Parameters
        ----------
        stream_name:
            Named stream to draw from.
        min:
            Lower bound (inclusive).
        max:
            Upper bound (inclusive).

        Returns
        -------
        int
        """
        out = _ffi.new("int64_t *")
        err = new_err_buf()
        _check(
            _lib.scs_uniform_int(
                self._ptr, stream_name.encode(), int(min), int(max), out, err, _ERR_BUF_SIZE
            ),
            err,
        )
        return int(out[0])

    def uniform_choice(self, stream_name: str, n: int) -> int:
        """Draw a uniform choice index in [0, n).

        Parameters
        ----------
        stream_name:
            Named stream to draw from.
        n:
            Number of choices (must be ≥ 1).

        Returns
        -------
        int
            Index in ``[0, n)``.
        """
        out = _ffi.new("int64_t *")
        err = new_err_buf()
        _check(
            _lib.scs_uniform_choice(
                self._ptr, stream_name.encode(), int(n), out, err, _ERR_BUF_SIZE
            ),
            err,
        )
        return int(out[0])

    # ------------------------------------------------------------------
    # Internal accessor used by other binding modules
    # ------------------------------------------------------------------

    def _handle(self):
        """Return the raw cffi SCS_StreamManager* pointer."""
        return self._ptr
