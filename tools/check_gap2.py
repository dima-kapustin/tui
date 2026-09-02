#!/usr/bin/env python3
"""Checks the gap rows between grid rows for wrong (light) colors."""
import sys

sys.path.insert(0, 'tools')
import decode_sixel  # noqa: E402

data = open(sys.argv[1], 'rb').read()
i = data.find(b'\x1bP')
e = data.find(b'\x1b\\', i)
img = decode_sixel.decode_dcs(data[i:e + 2])
h = len(img)
w = len(img[0])

MENU = 32
GRID_X = 16
GRID_Y = 104
STEP = 16
CELL = 14


def at(x, y):
    if 0 <= y < h and 0 <= x < w and img[y][x] is not None:
        return img[y][x]
    return None


INK = (224, 224, 229)
BG = (7, 7, 12)
EMPTY = (33, 33, 40)

bad = 0
for r in range(32):
    for gy in range(CELL, STEP):  # the gap pixels of row r
        y = MENU + GRID_Y + r * STEP + gy
        for x in range(GRID_X, GRID_X + 16 * STEP):
            c = at(x, y)
            if c == INK or c == EMPTY:
                bad += 1
                if bad <= 20:
                    print("gap row %d gap-px %d: x=%d color=%s" % (r, gy, x, c))
print("total wrong gap pixels:", bad)
