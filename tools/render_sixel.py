#!/usr/bin/env python3
"""Renders the sixel images captured from tui++ output into a PNG, drawing
each image at its reported cursor position so the layout can be inspected."""
import re
import sys

try:
    from PIL import Image
except ImportError:
    print("needs Pillow: pip install Pillow")
    sys.exit(1)

sys.path.insert(0, 'tools')
import decode_sixel  # noqa: E402


def extract_images(data):
    row = 1
    col = 1
    images = []
    i = 0
    n = len(data)
    while i < n:
        if data[i:i + 2] == b'\x1b[':
            j = data.find(b'H', i)
            if j < 0:
                break
            body = data[i + 2:j]
            if body.startswith(b'?') or body.startswith(b'>'):
                i += 2
                continue
            if body == b'':
                row = 1
                col = 1
            else:
                parts = body.split(b';')
                try:
                    row = int(parts[0])
                    col = int(parts[1]) if len(parts) > 1 else 1
                except ValueError:
                    pass
            i = j + 1
        elif data[i:i + 2] == b'\x1bP' and data[i:i + 8] == b'\x1bP0;1;0q':
            e = data.find(b'\x1b\\', i)
            if e < 0:
                break
            img = decode_sixel.decode_dcs(data[i:e + 2])
            images.append((row, col, img))
            i = e + 2
        else:
            i += 1
    return images


def main(path, out):
    data = open(path, 'rb').read()
    images = extract_images(data)
    if not images:
        print("no images found")
        return

    cell_w, cell_h = 16, 32  # matches the default SixelScreen cell size
    rows = 24
    cols = 80
    canvas = Image.new('RGB', (cols * cell_w, rows * cell_h), (0, 0, 0))
    px = canvas.load()
    for (row, col, img) in images:
        if img is None:
            continue
        x0 = (col - 1) * cell_w
        y0 = (row - 1) * cell_h
        h = len(img)
        w = len(img[0])
        for y in range(h):
            for x in range(w):
                c = img[y][x]
                if c is not None and 0 <= x0 + x < canvas.width and 0 <= y0 + y < canvas.height:
                    px[x0 + x, y0 + y] = c
    canvas.save(out)
    print("wrote %s (%dx%d)" % (out, canvas.width, canvas.height))


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else 'fontedit.png')
