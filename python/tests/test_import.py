"""Smoke test: verify the package imports and exposes a version string."""

import socialchoicelab


def test_version_is_string():
    assert isinstance(socialchoicelab.__version__, str)
    assert len(socialchoicelab.__version__) > 0
