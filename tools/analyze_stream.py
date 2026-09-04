#!/usr/bin/env python3
"""Extracts the sequence of sixel images (with their cursor positions) from a
captured tui++ terminal stream and summarizes each one: position, size, and a
digest of the pixels. Used to diff the emitted streams between builds."""
import sys

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


def digest(img):
    if img is None:
        return "decode-failed"
    h = len(img)
    w = len(img[0]) if h else 0
    painted = 0
    # Sample a few pixels: top-left, center, bottom-right of the image.
    samples = []
    for y, x in ((0, 0), (h // 2, w // 2), (h - 1, w - 1)):
        if img[y][x] is not None:
            samples.append(img[y][x])
    for row_px in img:
        for p in row_px:
            if p is not None:
                painted += 1
    return "w=%d h=%d painted=%d samples=%s" % (w, h, painted, samples)


def main():
    data = open(sys.argv[1], 'rb').read()
    images = extract_images(data)
    print("total images:", len(images))
    for n, (row, col, img) in enumerate(images):
        print("%3d: at row=%d col=%d  %s" % (n, row, col, digest(img)))


if __name__ == '__main__':
    main()
