"""
Unit tests for svg_parser.write_polylines_to_txt.

Run with:  python -m pytest software/test_svg_parser.py
       or: python software/test_svg_parser.py
"""

from __future__ import annotations

import os
import tempfile
from pathlib import Path

import pytest

from svg_parser import write_polylines_to_txt


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _read(path: str) -> str:
    return Path(path).read_text(encoding="utf-8")


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

class TestWritePolylinesToTxt:
    def test_creates_file(self, tmp_path):
        out = tmp_path / "polylines.txt"
        write_polylines_to_txt([[(0.0, 0.0), (1.0, 2.0)]], str(out))
        assert out.exists()

    def test_header_timestamp_present(self, tmp_path):
        out = tmp_path / "polylines.txt"
        write_polylines_to_txt([[(0.0, 0.0)]], str(out))
        content = _read(str(out))
        assert "# Polylines export" in content
        assert "# Timestamp:" in content

    def test_header_image_name(self, tmp_path):
        out = tmp_path / "polylines.txt"
        write_polylines_to_txt([[(0.0, 0.0)]], str(out), image_name="dog.svg")
        content = _read(str(out))
        assert "# Source: dog.svg" in content

    def test_header_no_image_name_by_default(self, tmp_path):
        out = tmp_path / "polylines.txt"
        write_polylines_to_txt([[(0.0, 0.0)]], str(out))
        content = _read(str(out))
        assert "# Source:" not in content

    def test_polyline_count_in_header(self, tmp_path):
        out = tmp_path / "polylines.txt"
        polylines = [[(0.0, 0.0), (1.0, 1.0)], [(5.0, 5.0)]]
        write_polylines_to_txt(polylines, str(out))
        content = _read(str(out))
        assert "# Polylines: 2" in content

    def test_polyline_labels_zero_indexed(self, tmp_path):
        out = tmp_path / "polylines.txt"
        polylines = [[(0.0, 0.0)], [(1.0, 1.0)]]
        write_polylines_to_txt(polylines, str(out))
        content = _read(str(out))
        assert "Polyline 0" in content
        assert "Polyline 1" in content

    def test_end_markers_present(self, tmp_path):
        out = tmp_path / "polylines.txt"
        polylines = [[(0.0, 0.0), (1.0, 2.0)], [(3.0, 4.0)]]
        write_polylines_to_txt(polylines, str(out))
        content = _read(str(out))
        assert content.count("END") == 2

    def test_point_format(self, tmp_path):
        out = tmp_path / "polylines.txt"
        write_polylines_to_txt([[(1.5, 2.75)]], str(out))
        content = _read(str(out))
        assert "1.500,2.750" in content

    def test_empty_polylines_list(self, tmp_path):
        out = tmp_path / "polylines.txt"
        write_polylines_to_txt([], str(out))
        content = _read(str(out))
        assert "# Polylines: 0" in content
        # No polyline blocks should be written (only the header line contains "Polyline")
        assert "Polyline 0" not in content
        assert "END" not in content

    def test_raises_on_bad_path(self, tmp_path):
        bad_path = tmp_path / "nonexistent_subdir" / "polylines.txt"
        with pytest.raises(OSError):
            write_polylines_to_txt([[(0.0, 0.0)]], str(bad_path))

    def test_overwrites_existing_file(self, tmp_path):
        out = tmp_path / "polylines.txt"
        write_polylines_to_txt([[(0.0, 0.0)]], str(out))
        write_polylines_to_txt([[(9.0, 9.0)]], str(out))
        content = _read(str(out))
        assert "9.000,9.000" in content
        assert "0.000,0.000" not in content


if __name__ == "__main__":
    import sys
    # Allow running directly without pytest installed
    passed = failed = 0
    suite = TestWritePolylinesToTxt()
    for name in (m for m in dir(suite) if m.startswith("test_")):
        with tempfile.TemporaryDirectory() as td:
            try:
                getattr(suite, name)(Path(td))
                print(f"  PASS  {name}")
                passed += 1
            except Exception as exc:
                print(f"  FAIL  {name}: {exc}")
                failed += 1

    print(f"\n{passed} passed, {failed} failed")
    sys.exit(0 if failed == 0 else 1)
