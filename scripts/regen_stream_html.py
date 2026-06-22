#!/usr/bin/env python3
"""Regenerate index_ov3660_stream_html_gz in main/camera_index.h from source HTML."""

from __future__ import annotations

import gzip
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HTML = ROOT / "main" / "index_ov3660_stream.html"
HEADER = ROOT / "main" / "camera_index.h"
VAR = "index_ov3660_stream_html_gz"


def format_array(data: bytes) -> str:
    lines: list[str] = []
    for i in range(0, len(data), 16):
        chunk = data[i : i + 16]
        lines.append(" " + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
    if lines:
        lines[-1] = lines[-1].rstrip(",")
    return "\n".join(lines)


def patch_header(data: bytes) -> None:
    text = HEADER.read_text()
    pattern = (
        rf"//File: index_ov3660_stream\.html\.gz, Size: \d+\n"
        rf"#define {VAR}_len \d+\n"
        rf"const uint8_t {VAR}\[\] = \{{\n.*?\n\}};"
    )
    block = (
        f"//File: index_ov3660_stream.html.gz, Size: {len(data)}\n"
        f"#define {VAR}_len {len(data)}\n"
        f"const uint8_t {VAR}[] = {{\n"
        f"{format_array(data)}\n"
        f"}};"
    )
    new_text, n = re.subn(pattern, block, text, count=1, flags=re.S)
    if n != 1:
        sys.exit(f"Could not find {VAR} block in {HEADER}")
    HEADER.write_text(new_text)


def main() -> None:
    if not HTML.is_file():
        sys.exit(f"Missing source HTML: {HTML}")
    data = gzip.compress(HTML.read_bytes(), compresslevel=9, mtime=0)
    patch_header(data)
    title = re.search(rb"<title>(.*?)</title>", HTML.read_bytes(), re.I)
    print(f"Updated {HEADER.name}: {VAR}_len={len(data)}")
    if title:
        print(f"Title: {title.group(1).decode()}")


if __name__ == "__main__":
    main()
