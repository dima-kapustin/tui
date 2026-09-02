#!/usr/bin/env python3
"""Shows the bytes just before the first sixel image in a capture."""
import sys

data = open(sys.argv[1], 'rb').read()
i = data.find(b'\x1bP0;1;0q')
print("image at byte", i)
print(repr(data[i - 40:i + 20]))
