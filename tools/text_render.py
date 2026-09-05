#!/usr/bin/env python3
"""Reconstructs the final text screen from a captured tui++ escape stream.

Usage: tools/text_render.py resize_probe_stream.bin [--dump] [--lines]

Prints a compact per-row rendering (using ANSI colors when stdout is a
terminal) and a summary of every vertical-line cell with a light-cyan
foreground (the demo's ice border color), so a doubled left border is easy
to spot."""

import sys


def parse_stream(data):
    rows = {}  # y -> {x -> [char, fg, bg, attrs]}
    y = 1
    x = 1
    fg = None  # (r, g, b) or None
    bg = None
    attrs = set()
    i = 0
    n = len(data)
    while i < n:
        c = data[i]
        if c == 0x1B:  # ESC
            if i + 1 >= n:
                break
            d = data[i + 1]
            if d == ord('['):
                k = i + 2
                buf = []
                while k < n and data[k] not in b'ABCDEFGHJKSTfmhlp@' and data[k] != 0x1B:
                    buf.append(chr(data[k]))
                    k += 1
                if k >= n:
                    break
                if data[k] == 0x1B:
                    # A bare CSI (no final byte): a no-op for terminals too.
                    # Resume at the nested ESC instead of skipping it.
                    i = k
                    continue
                final = chr(data[k])
                body = ''.join(buf)
                if final == 'H' or final == 'f':
                    parts = body.split(';')
                    row = int(parts[0]) if parts[0] else 1
                    col = int(parts[1]) if len(parts) > 1 and parts[1] else 1
                    y, x = row, col
                elif final == 'm':
                    if not body:
                        fg = bg = None
                        attrs = set()
                    for sgr in body.split(';'):
                        s = int(sgr or '0')
                        if s == 0:
                            fg = bg = None
                            attrs = set()
                        elif s == 1:
                            attrs.add('bold')
                        elif s == 22:
                            attrs.discard('bold')
                        elif s == 7:
                            attrs.add('inverse')
                        elif s == 27:
                            attrs.discard('inverse')
                        elif s == 39:
                            fg = None
                        elif s == 49:
                            bg = None
                elif final == 'J' and body == '2':
                    pass  # clear (sixel backend)
                i = k + 1
            elif d == ord('P'):
                e = data.find(b'\x1b\\', i)
                i = e + 2 if e >= 0 else n
            elif d == ord(']'):
                e = data.find(b'\x07', i)
                if e < 0:
                    e = data.find(b'\x1b\\', i)
                i = e + 1 if e >= 0 else n
            else:
                i += 2
            continue
        elif c in (ord('\r'),):
            i += 1
            continue
        elif c == ord('\n'):
            y += 1
            i += 1
            continue
        else:
            # multi-byte UTF-8 char
            ln = 1
            if c >= 0xF0:
                ln = 4
            elif c >= 0xE0:
                ln = 3
            elif c >= 0xC0:
                ln = 2
            ch = data[i:i + ln].decode('utf-8', 'replace')
            cell = rows.setdefault(y, {})
            cell[x] = [ch, fg, bg, set(attrs)]
            x += 1
            i += ln
    return rows


def main():
    data = open(sys.argv[1], 'rb').read()
    rows = parse_stream(data)
    dump = '--dump' in sys.argv
    lines = '--lines' in sys.argv

    maxy = max(rows) if rows else 0
    print(f"rows: {maxy}")
    vert_cells = []
    for y in range(1, maxy + 1):
        row = rows.get(y, {})
        if not row:
            continue
        maxx = max(row)
        if dump:
            out = []
            for x in range(1, maxx + 1):
                ch, fg, bg, attrs = row.get(x, [' ', None, None, set()])
                seq = []
                if fg:
                    seq.append(f"38;2;{fg[0]};{fg[1]};{fg[2]}")
                if bg:
                    seq.append(f"48;2;{bg[0]};{bg[1]};{bg[2]}")
                out.append(f"\x1b[{';'.join(seq)}m{ch}\x1b[0m" if seq else ch)
            print(f"{y:2d} " + ''.join(out))
        for x, (ch, fg, bg, attrs) in row.items():
            if ch in ('\u2502', '\u2503', '\u2551', '\u2506', '\u250a'):  # vertical lines
                if fg and fg[2] >= 0xC0 and fg[1] >= 0xC0 and fg[0] <= 0xC0:  # light cyan-ish
                    vert_cells.append((y, x, ch, fg))
    if lines:
        print("vertical line cells (light-cyan fg):")
        for y, x, ch, fg in vert_cells:
            print(f"  row {y} col {x} {ch!r} fg={fg}")


if __name__ == '__main__':
    main()
