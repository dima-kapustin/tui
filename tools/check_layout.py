#!/usr/bin/env python3
"""Samples decoded sixel pixels at key layout positions to verify the editor
geometry: menu bar, header text, grid, and the overview strip."""
import sys

sys.path.insert(0, 'tools')
import decode_sixel  # noqa: E402

data = open(sys.argv[1], 'rb').read()
i = data.find(b'\x1bP')
e = data.find(b'\x1b\\', i)
img = decode_sixel.decode_dcs(data[i:e + 2])
h = len(img)
w = len(img[0])
print("image: %dx%d px" % (w, h))

def at(x, y):
    if 0 <= y < h and 0 <= x < w:
        c = img[y][x]
        return c if c is not None else (0, 0, 0)
    return None

def describe(name, x, y):
    print("%-28s (%4d,%4d) = %s" % (name, x, y, at(x, y)))

menu_bar_bg = (24, 26, 34)
bg = (10, 10, 14)
empty = (34, 34, 42)

# The menu bar occupies the top cell row (32 px with the 16x32 fallback cell).
describe("menu bar fill", 8, 16)
describe("panel bg (below bar)", 8, 40)
describe("title text area", 20, 10)
describe("grid cell (0,0)", 16 + 2, 32 + 104 + 2)
describe("grid gap", 16 + 15, 32 + 104 + 2)
describe("hex text", 500, 32 + 110)
# Find the strip: scan downward for the highlight color of the selected 'A'.
for y in range(h - 1, 0, -1):
    if at(8, y) == (215, 180, 40) or at(8, y) == (212, 177, 40):
        print("strip highlight found at y=%d" % y)
        describe("strip 'A' highlight", 8, y + 16)
        break
print("last row color at x=8:", at(8, h - 1))
