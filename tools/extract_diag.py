#!/usr/bin/env python3
"""Extracts the probe's plain-text diagnostics from the captured stream."""
import re
import sys

data = open(sys.argv[1], 'rb').read()
# The diagnostics are plain ASCII lines between the ESC sequences.
text = data.decode('latin-1')
for line in text.splitlines():
    s = line.strip()
    if s.startswith('-- after') or s.startswith('armed:') or s.startswith('New ') or s.startswith('Open') or s.startswith('Save') or s.startswith('Exit') or s.startswith('(') or s.startswith('frame ') or s.startswith('File menu'):
        print(s)
