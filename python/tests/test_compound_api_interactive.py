"""Tests for compound API interactive demo payload and validation."""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

_PYTHON_DIR = Path(__file__).resolve().parents[1]
if str(_PYTHON_DIR) not in sys.path:
    sys.path.insert(0, str(_PYTHON_DIR))

from compound_api_interactive_common import (  # noqa: E402
    build_payload,
    format_validation_report,
    validate_interactive_payload,
)


def test_validate_empty_payload_fails():
    checks, ok = validate_interactive_payload({})
    assert ok is False
    assert any(not c["ok"] for c in checks)


def test_format_validation_report_covers_checks():
    checks, ok = validate_interactive_payload({})
    text = format_validation_report(checks, ok)
    assert "compound_api_interactive" in text
    assert "SUMMARY" in text


def test_build_payload_passes_checks():
    try:
        payload = build_payload()
    except Exception as e:
        pytest.skip(f"Native library or bindings unavailable: {e}")
    assert payload["meta"]["binding"] == "python"
    assert payload["meta"]["build_all_ok"] is True
    stripped = {k: v for k, v in payload.items() if k != "meta"}
    checks2, ok2 = validate_interactive_payload(stripped)
    assert ok2 is True
    assert len(checks2) == len(payload["meta"]["build_checks"])
    for a, b in zip(payload["meta"]["build_checks"], checks2, strict=True):
        assert a["scenario"] == b["scenario"] and a["id"] == b["id"] and a["ok"] == b["ok"]
