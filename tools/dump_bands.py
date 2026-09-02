#!/usr/bin/env python3
"""Dumps the sixel stream structure for the bands around a given pixel row."""
import sys

data = open(sys.argv[1], 'rb').read()
i = data.find(b'\x1bP0;1;0q')
e = data.find(b'\x1b\\', i)
payload = data[i + 8:e]

# Split into bands: the first band has no $-; later bands start with $-.
# Strip the leading raster attributes and palette definitions.
j = 0
n = len(payload)
# skip raster attributes
while j < n and payload[j:j + 1] != b'"':
    j += 1
if payload[j:j + 1] == b'"':
    while j < n and payload[j:j + 1] not in (b'#', b'!', b'$') and not (0x3F <= payload[j] <= 0x7E):
        j += 1
# skip palette definitions (#N;2;...)
while j < n and payload[j:j + 1] == b'#':
    m = j + 1
    while m < n and payload[m:m + 1] != b';':
        m += 1
    # skip "2;R;G;B"
    fields = 0
    while m < n and fields < 3:
        if payload[m:m + 1] == b';':
            fields += 1
        m += 1
    j = m

bands = []
cur = b''
while j < n:
    c = payload[j:j + 1]
    if c == b'$' and j + 1 < n and payload[j + 1:j + 2] == b'-' and cur:
        bands.append(cur)
        cur = b''
        j += 2
        continue
    cur += c
    j += 1
bands.append(cur)

print("total bands:", len(bands))
for b in (48, 49, 50, 51):
    if b < len(bands):
        print("band %d (%d bytes): %r" % (b, len(bands[b]), bands[b][:200]))
