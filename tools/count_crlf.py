#!/usr/bin/env python3
"""Counts CR/LF sequences in a capture and locates the first sixel image."""
import sys

data = open(sys.argv[1], 'rb').read()
print("bytes:", len(data))
print("CR CR LF count:", data.count(b'\r\r\n'))
print("LF count:", data.count(b'\n'))
i = data.find(b'\x1bP0;1;0q')
print("first image at byte", i)
print("LFs before the image:", data[:i].count(b'\n'))
