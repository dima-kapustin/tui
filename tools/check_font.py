#!/usr/bin/env python3
"""Prints selected glyphs of the generated font as ASCII art for a quick
visual sanity check (baseline, stem sides, proportions)."""

import os
import re
import sys

HEADER = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tui++", "Inc", "tui++", "terminal", "graphic", "Font16x32.h"))

text = open(HEADER, encoding="ascii").read()
table = re.search(r"FONT16X32_BASIC\[128\]\[64\] = \{(.*?)\};", text, re.S).group(1)
rows = re.findall(r"\{([^}]*)\}", table)

def glyph(code):
    r = rows[code]
    return [int(b, 16) for b in re.findall(r"0x([0-9A-F]{2})", r)]

def art(code, label):
    g = glyph(code)
    print("=== U+%04X '%s' ===" % (code, label))
    for ry in range(32):
        row = (g[ry * 2] << 8) | g[ry * 2 + 1]
        line = ""
        for rx in range(16):
            line += "#" if row & (0x8000 >> rx) else "."
        # annotate the baseline row
        mark = "  <- baseline" if ry == 22 else ("  <- descender" if ry == 28 else "")
        print("%s%s" % (line, mark))
    print()

for code, label in [(0x41, "A"), (0x42, "B"), (0x62, "b"), (0x64, "d"), (0x70, "p"), (0x71, "q"), (0x65, "e"), (0x30, "0"), (0x69, "i"), (0x49, "I"), (0x6C, "l"), (0x6A, "j"), (0x67, "g"), (0x4D, "M"), (0x57, "W"), (0x26, "&"), (0x40, "@")]:
    art(code, label)
