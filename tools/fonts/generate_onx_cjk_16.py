#!/usr/bin/env python3
"""Regenerate the PrintSphere ONX 16px CJK LVGL font subset."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
COMMON_PATH = ROOT / "tools/fonts/modern_chinese_common_2500.txt"
EXTRA_PATH = ROOT / "tools/fonts/onx_extra_symbols.txt"
FONT_PATH = ROOT / "managed_components/lvgl__lvgl/scripts/built_in_font/SourceHanSansSC-Normal.otf"
OUTPUT_PATH = ROOT / "main/include/font/onx_cjk_16.c"

EXPECTED_COMMON_CJK = 2500
SAMPLE_STRINGS = [
    "OpenNextion3.5寸外壳后倾",
    "OpenNextion3.5横向外壳",
    "打印机配置",
    "封面预览",
    "网络连接成功",
]


def read_chars(path: Path) -> list[str]:
    text = path.read_text(encoding="utf-8")
    chars: list[str] = []
    for ch in text:
        if ch == "#" or ch == "\n":
            continue
        if "\u4e00" <= ch <= "\u9fff":
            if ch not in chars:
                chars.append(ch)
    return chars


def read_chars_skip_comments(path: Path) -> list[str]:
    chars: list[str] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        for ch in stripped:
            if "\u4e00" <= ch <= "\u9fff" and ch not in chars:
                chars.append(ch)
    return chars


def extract_generated_chars(path: Path) -> set[str]:
    text = path.read_text(encoding="utf-8", errors="ignore")
    chars: set[str] = set()
    for match in re.finditer(r'/\* U\+([0-9A-Fa-f]+) ".*?" \*/', text):
        chars.add(chr(int(match.group(1), 16)))
    return chars


def main() -> int:
    common = read_chars_skip_comments(COMMON_PATH)
    if len(common) != EXPECTED_COMMON_CJK:
        print(f"expected {EXPECTED_COMMON_CJK} common CJK chars, got {len(common)}", file=sys.stderr)
        return 1

    extra = read_chars_skip_comments(EXTRA_PATH)
    symbols: list[str] = []
    for ch in common + extra:
        if ch not in symbols:
            symbols.append(ch)

    missing_samples = sorted({
        ch
        for sample in SAMPLE_STRINGS
        for ch in sample
        if "\u4e00" <= ch <= "\u9fff" and ch not in symbols
    })
    if missing_samples:
        print(f"sample glyphs missing from input charset: {''.join(missing_samples)}", file=sys.stderr)
        return 1

    cmd = [
        "npx",
        "--yes",
        "lv_font_conv",
        "--font",
        str(FONT_PATH),
        "-r",
        "0x20-0x7f",
        "--symbols",
        "".join(symbols),
        "--size",
        "16",
        "--bpp",
        "4",
        "--format",
        "lvgl",
        "--lv-font-name",
        "onx_cjk_16",
        "--output",
        str(OUTPUT_PATH),
        "--force-fast-kern-format",
    ]
    subprocess.run(cmd, cwd=ROOT, check=True)
    generated_text = OUTPUT_PATH.read_text(encoding="utf-8").rstrip() + "\n"
    generated_text = generated_text.replace(str(ROOT) + "/", "<repo>/")
    include_probe = """#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

"""
    include_anchor = "#ifdef LV_LVGL_H_INCLUDE_SIMPLE\n"
    if include_anchor in generated_text and "__has_include" not in generated_text:
        generated_text = generated_text.replace(include_anchor, include_probe + include_anchor, 1)
    OUTPUT_PATH.write_text(generated_text, encoding="utf-8")

    generated = extract_generated_chars(OUTPUT_PATH)
    generated_cjk = {ch for ch in generated if "\u4e00" <= ch <= "\u9fff"}
    required = set(common) | set(extra)
    missing_generated = sorted(required - generated_cjk)
    if missing_generated:
        print(f"generated font missing required CJK glyphs: {''.join(missing_generated[:80])}", file=sys.stderr)
        return 1

    print(f"common_cjk={len(common)}")
    print(f"extra_cjk={len(extra)}")
    print(f"merged_cjk={len(required)}")
    print(f"generated_total_glyphs={len(generated)}")
    print(f"generated_cjk_glyphs={len(generated_cjk)}")
    print(f"contains_横={'yes' if '横' in generated else 'no'}")
    print(f"output={OUTPUT_PATH}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
