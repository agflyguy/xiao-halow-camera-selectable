#!/usr/bin/env python3
"""Inject camera title banner + /camera_info fetch into index_ov3660_html_gz."""

from __future__ import annotations

import gzip
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "main" / "camera_index.h"
VAR = "index_ov3660_html_gz"
MARKER = 'id="cam-title"'
BANNER = (
    '<h1 id="cam-title" style="margin:0 0 4px;font-size:18px;font-weight:400;color:#EFEFEF"></h1>\n'
    '<p id="cam-ip" style="margin:0 0 10px;color:#aaa;font-size:13px"></p>\n'
    '<script>function applyCameraInfo(j){if(!j)return;if(j.title){document.title=j.title;'
    'var t=document.getElementById("cam-title");if(t)t.textContent=j.title;}'
    'var p=document.getElementById("cam-ip");if(p&&j.ip)p.textContent=(j.static_ip?"Static ":"")+j.ip;}'
    'fetch("/camera_info").then(function(r){return r.json()}).then(applyCameraInfo).catch(function(){});'
    '</script>\n'
)


def format_array(data: bytes) -> str:
    lines: list[str] = []
    for i in range(0, len(data), 16):
        chunk = data[i : i + 16]
        lines.append(" " + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
    if lines:
        lines[-1] = lines[-1].rstrip(",")
    return "\n".join(lines)


def main() -> None:
    text = HEADER.read_text()
    m = re.search(rf"const uint8_t {VAR}\[\] = \{{(.*?)\}};", text, re.S)
    if not m:
        sys.exit(f"Missing {VAR} in {HEADER}")
    nums = [int(x, 0) for x in re.findall(r"0x[0-9a-fA-F]+", m.group(1))]
    html = gzip.decompress(bytes(nums)).decode("utf-8")
    if MARKER in html:
        print(f"{VAR}: already patched")
        return
    if "<body>" not in html:
        sys.exit("No <body> tag found")
    html = html.replace("<body>", "<body>\n" + BANNER, 1)
    data = gzip.compress(html.encode("utf-8"), compresslevel=9, mtime=0)
    block = (
        f"//File: index_ov3660.html.gz, Size: {len(data)}\n"
        f"#define {VAR}_len {len(data)}\n"
        f"const uint8_t {VAR}[] = {{\n"
        f"{format_array(data)}\n"
        f"}};"
    )
    pattern = (
        rf"//File: index_ov3660\.html\.gz, Size: \d+\n"
        rf"#define {VAR}_len \d+\n"
        rf"const uint8_t {VAR}\[\] = \{{\n.*?\n\}};"
    )
    new_text, n = re.subn(pattern, block, text, count=1, flags=re.S)
    if n != 1:
        sys.exit(f"Could not replace {VAR} block")
    HEADER.write_text(new_text)
    print(f"Patched {VAR}_len={len(data)}")


if __name__ == "__main__":
    main()
