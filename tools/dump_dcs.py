#!/usr/bin/env python3
"""Dumps the cursor position + declared size of every sixel DCS in a captured
stream, plus the bytes of the last few images, for stream debugging."""
import re
import sys

data = open(sys.argv[1], 'rb').read()
starts = [m.start() for m in re.finditer(rb'\x1bP0;1;0q', data)]
print("dces:", len(starts))
for s in starts[-10:]:
    e = data.find(b'\x1b\\', s)
    payload = data[s:e]
    m = re.match(rb'\x1bP0;1;0q"1;1;(\d+);(\d+)', payload)
    before = data[:s]
    last_h = before.rfind(b'\x1b[')
    hseg = before[last_h + 2:].split(b'H')[0]
    dims = (m.group(1).decode(), m.group(2).decode()) if m else ('?', '?')
    print("at", s, "cursor", hseg.decode(), "dims", dims, "payload_len", len(payload))
