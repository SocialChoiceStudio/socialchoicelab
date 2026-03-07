"""Placeholder: package import test (activated once B4 cffi loader is built).

The Python binding is implemented in B4. Until then, this file ensures
pytest collects at least one item so the runner does not exit with code 5.
"""

import pytest


@pytest.mark.skip(reason="Python binding not yet built (B4 pending)")
def test_version_is_string():
    import socialchoicelab  # noqa: PLC0415
    assert isinstance(socialchoicelab.__version__, str)
    assert len(socialchoicelab.__version__) > 0
