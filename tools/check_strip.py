#!/usr/bin/env python3
"""Checks the strip area of the decoded fontedit image: the selected glyph's
highlight and the glyph rows must all be inside the rendered image."""
import sys

sys.path.insert(0, 'tools')
import decode_sixel  # noqa: E402

data = open(sys.argv[1], 'rb').read()
i = data.find(b'\x1bP')
e = data.find(b'\x1b\\', i)
img = decode_sixel.decode_dcs(data[i:e + 2])
h = len(img)
w = len(img[0])


def at(x, y):
    if 0 <= y < h and 0 <= x < w and img[y][x] is not None:
        return img[y][x]
    return (0, 0, 0)


found = []
for y in range(560, h):
    for x in range(0, w):
        c = at(x, y)
        if abs(c[0] - 215) <= 5 and abs(c[1] - 180) <= 5 and abs(c[2] - 40) <= 5:
            found.append((x, y))
print("highlight pixels:", len(found))
if found:
    xs = [f[0] for f in found]
    ys = [f[1] for f in found]
    print("highlight bbox: x %d..%d y %d..%d" % (min(xs), max(xs), min(ys), max(ys)))
print("strip row0 '!' pixel:", at(8, 670))
print("strip row1 '!' pixel:", at(8, 700))
print("bottom content row:", at(8, h - 1))
