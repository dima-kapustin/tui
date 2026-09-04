#!/usr/bin/env python3
"""Extracts one sixel image (by index) from a captured stream and writes its
decoded pixels as a raw RGB file (for the encode_tool byte-identity test)."""
import re
import sys

sys.path.insert(0, 'tools')
import decode_sixel  # noqa: E402


def extract_images(data):
    images = []
    i = 0
    n = len(data)
    while i < n:
        if data[i:i + 2] == b'\x1bP' and data[i:i + 8] == b'\x1bP0;1;0q':
            e = data.find(b'\x1b\\', i)
            if e < 0:
                break
            images.append(decode_sixel.decode_dcs(data[i:e + 2]))
            i = e + 2
        else:
            i += 1
    return images


def main():
    stream, index, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]
    images = extract_images(open(stream, 'rb').read())
    img = images[index]
    h, w = len(img), len(img[0]) if img else 0
    with open(out, 'wb') as f:
        for row in img:
            for p in row:
                if p is None:
                    p = (0, 0, 0)
                f.write(bytes(p))
    print("image", index, ":", w, "x", h, "written to", out)


if __name__ == '__main__':
    main()
