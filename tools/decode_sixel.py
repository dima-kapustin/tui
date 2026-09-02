#!/usr/bin/env python3
"""Decodes the sixel images emitted by tui++'s SixelEncoder (a simplified
decoder for that encoder's exact output), tracking the cursor position between
images so each one's on-screen location is reported. Used to verify the font
editor's incremental repaints."""

import re
import sys

BAND = 6


def decode_dcs(payload):
    # Raster attributes: `" Pan ; Pad ; Ph ; Pv ` -- Ph = width, Pv = height.
    m = re.match(rb'\x1bP0;1;0q"(?:1;1;)?(\d+);(\d+)', payload)
    if not m:
        return None
    width = int(m.group(1))
    height = int(m.group(2))
    img = [[None] * width for _ in range(height)]

    attr_end = m.end()
    data = payload[attr_end:]
    if data.endswith(b'\x1b\\'):
        data = data[:-2]

    palette = {}
    cur = 0
    x = 0
    y = 0
    i = 0
    n = len(data)
    while i < n:
        c = data[i]
        if c == ord('#'):
            m2 = re.match(rb'#(\d+);2;(\d+);(\d+);(\d+)', data[i:])
            if m2:
                idx = int(m2.group(1))
                r = int(m2.group(2)) * 255 // 100
                g = int(m2.group(3)) * 255 // 100
                b = int(m2.group(4)) * 255 // 100
                palette[idx] = (r, g, b)
                i += m2.end()
            else:
                m2 = re.match(rb'#(\d+)', data[i:])
                idx = int(m2.group(1))
                i += m2.end()
            cur = idx
        elif c == ord('!'):
            m2 = re.match(rb'!(\d+)(.)', data[i:])
            count = int(m2.group(1))
            ch = m2.group(2)[0]
            i += m2.end()
            val = ch - 0x3F
            for _ in range(count):
                if x < width:
                    for bit in range(BAND):
                        if val & (1 << bit) and y + bit < height:
                            img[y + bit][x] = palette.get(cur)
                    x += 1
        elif 0x3F <= c <= 0x7E:
            val = c - 0x3F
            if x < width:
                for bit in range(BAND):
                    if val & (1 << bit) and y + bit < height:
                        img[y + bit][x] = palette.get(cur)
                x += 1
            i += 1
        elif c == ord('$'):
            x = 0
            i += 1
        elif c == ord('-'):
            y += BAND
            x = 0
            i += 1
        else:
            i += 1
    return img


def main(path):
    data = open(path, 'rb').read()
    # Track the text cursor: CSI r;c H (also bare CSI H = 1;1). Sixel images
    # are drawn with their top-left corner at the cursor.
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
            # Only a plain CSI ... H (no leading '?' DEC-private marker)
            # moves the text cursor; ignore DEC modes like ESC[?1049h.
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
                    i += 2
                    continue
            i = j + 1
        elif data[i:i + 2] == b'\x1bP' and data[i:i + 8] == b'\x1bP0;1;0q':
            e = data.find(b'\x1b\\', i)
            if e < 0:
                break
            img = decode_dcs(data[i:e + 2])
            images.append((row, col, img))
            i = e + 2
        else:
            i += 1

    print("found %d sixel image(s)" % len(images))
    from collections import Counter
    for idx, (r, c, img) in enumerate(images):
        if img is None:
            print("  #%d: at row %d col %d -- decode failed" % (idx, r, c))
            continue
        h = len(img)
        w = len(img[0])
        cnt = Counter()
        for rowp in img:
            for px in rowp:
                if px is not None:
                    cnt[px] += 1
        top = "  #%d: %dx%d px at text row %d col %d" % (idx, w, h, r, c)
        colors = ", ".join("%d,%d,%d:%d" % (k[0], k[1], k[2], v) for k, v in cnt.most_common(6))
        print(top + "  [" + colors + "]")


if __name__ == "__main__":
    main(sys.argv[1])
