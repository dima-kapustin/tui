#!/usr/bin/env python3
"""Dumps one full band of the sixel stream, splitting it into passes."""
import sys

data = open(sys.argv[1], 'rb').read()
i = data.find(b'\x1bP0;1;0q')
e = data.find(b'\x1b\\', i)
payload = data[i + 8:e]

j = 0
n = len(payload)
while j < n and payload[j:j + 1] != b'"':
    j += 1
if payload[j:j + 1] == b'"':
    while j < n and payload[j:j + 1] not in (b'#', b'!', b'$') and not (0x3F <= payload[j] <= 0x7E):
        j += 1
while j < n and payload[j:j + 1] == b'#':
    m = j + 1
    while m < n and payload[m:m + 1] != b';':
        m += 1
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

band = int(sys.argv[2]) if len(sys.argv) > 2 else 49
data_b = bands[band]
print("band %d (%d bytes)" % (band, len(data_b)))
# split on $ (rewinds)
passes = data_b.split(b'$')
for k, p in enumerate(passes):
    print("  pass %d (%d bytes): %r" % (k, len(p), p))
