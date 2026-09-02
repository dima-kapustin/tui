#!/usr/bin/env python3
"""Rasterizes Cascadia Mono into the 16x32 monochrome bitmap font used by
tui++'s graphic (sixel) terminal.

The output is written to
  tui++/Inc/tui++/terminal/sixel/Font16x32.h

Method (same as the previous 8x16 raster, at twice the resolution):
  * every glyph is rendered at 2x the target cell (32x64 px) with the
    baseline fixed at row 44 (so, after the 2x2 downsample, the baseline
    sits on row 22 of the 16x32 cell -- caps rows 4..22, descenders to 28);
  * each 2x2 block is reduced by majority vote (>= 2 black pixels wins);
  * the glyph is centered horizontally inside the 16 columns;
  * rows are emitted as 2 bytes, bit 7 of byte 0 being the leftmost pixel.

Cascadia Mono is (c) Microsoft, SIL Open Font License
(https://openfontlicense.org), so the generated bitmap font carries that
attribution in the header it produces.
"""

from PIL import Image, ImageFont, ImageDraw
import os
import re
import sys

FONT_PATH = r"C:\Windows\Fonts\CascadiaMono.ttf"

W, H = 16, 32          # final cell size
SCALE = 2              # render scale factor (render / final)
RW, RH = W * SCALE, H * SCALE  # 32 x 64 render canvas

# The baseline must land on this row of the final raster (bottom of caps).
BASELINE = 22

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
OUT_PATH = os.path.normpath(os.path.join(SCRIPT_DIR, "..", "tui++", "Inc", "tui++", "terminal", "graphic", "Font16x32.h"))
PREVIEW_PATH = os.path.join(SCRIPT_DIR, "font_preview.png")


def find_font_size():
    """Returns the pixel size S whose ascent is 2*BASELINE (the baseline row
    in the render canvas), so drawing with anchor 'la' at (0,0) puts the
    baseline exactly there."""
    target = BASELINE * SCALE
    ref = ImageFont.truetype(FONT_PATH, 100)
    ascent_at_100 = ref.getmetrics()[0]
    guess = max(8, round(100.0 * target / ascent_at_100))
    font = ImageFont.truetype(FONT_PATH, guess)
    ascent = font.getmetrics()[0]
    # ascent scales linearly in the pixel size; correct one step.
    if ascent != target:
        guess = max(8, round(guess * target / ascent))
        font = ImageFont.truetype(FONT_PATH, guess)
        ascent = font.getmetrics()[0]
    print("font size %d, ascent %d (target %d)" % (guess, ascent, target), file=sys.stderr)
    return font


def downsample(canvas):
    """2x2 majority downsample of the RW x RH render into a W x H bitmap."""
    out = []
    px = canvas.load()
    for y in range(H):
        row = 0
        for x in range(W):
            ink = 0
            for dy in range(SCALE):
                for dx in range(SCALE):
                    ink += px[x * SCALE + dx, y * SCALE + dy]
            if ink >= SCALE * SCALE // 2:  # majority vote
                row |= 0x8000 >> x
        out.append(row)
    return out


def center(rows):
    """Centers the ink horizontally inside the 16 columns."""
    left = W
    right = -1
    for row in rows:
        for x in range(W):
            if row & (0x8000 >> x):
                left = min(left, x)
                right = max(right, x)
    if right < left:  # empty glyph
        return rows
    shift = (W - 1 - (left + right)) // 2
    if shift == 0:
        return rows
    shifted = []
    for row in rows:
        new_row = 0
        for x in range(W):
            if row & (0x8000 >> x) and 0 <= x + shift < W:
                new_row |= 0x8000 >> (x + shift)
        shifted.append(new_row)
    return shifted


def rasterize(font, code):
    canvas = Image.new("1", (RW, RH), 0)
    draw = ImageDraw.Draw(canvas)
    draw.text((0, 0), chr(code), font=font, fill=1, anchor="la")
    rows = downsample(canvas)
    return center(rows)


def main():
    font = find_font_size()

    glyphs = []
    for code in range(128):
        glyphs.append(rasterize(font, code))

    lines = []
    lines.append("// 16x32 monochrome bitmap font covering U+0000..U+007F, rasterized")
    lines.append("// from the Cascadia Mono typeface (c) Microsoft, SIL Open Font License")
    lines.append("// (https://openfontlicense.org), and reshaped into the 16x32 terminal cell.")
    lines.append("// Rows are 16 bits wide (2 bytes), bit 7 of byte 0 being the leftmost")
    lines.append("// pixel; caps sit on rows 5-22 (baseline 22), descenders reach row 27.")
    lines.append("#pragma once")
    lines.append("")
    lines.append("#include <cstdint>")
    lines.append("")
    lines.append("namespace tui {")
    lines.append("namespace detail {")
    lines.append("")
    lines.append("constexpr int FONT_WIDTH = 16;")
    lines.append("constexpr int FONT_HEIGHT = 32;")
    lines.append("")
    lines.append("constexpr uint8_t FONT16X32_BASIC[128][64] = {")
    for code, rows in enumerate(glyphs):
        repr_code = chr(code) if 0x20 <= code < 0x7F else " "
        bytes_ = []
        for row in rows:
            bytes_.append("0x%02X" % ((row >> 8) & 0xFF))
            bytes_.append("0x%02X" % (row & 0xFF))
        line = "    {" + ", ".join(bytes_) + "}, // U+%04X %r" % (code, repr_code)
        lines.append(line)
    lines.append("};")
    lines.append("")
    lines.append("}")
    lines.append("}")
    lines.append("")

    with open(OUT_PATH, "w", encoding="ascii") as f:
        f.write("\n".join(lines))

    make_preview(glyphs)
    print("wrote " + OUT_PATH, file=sys.stderr)
    print("wrote " + PREVIEW_PATH, file=sys.stderr)


def make_preview(glyphs):
    """Renders every printable glyph at 8x for a quick visual check."""
    scale = 8
    cols = 19
    pad = 8
    gw, gh = W * scale, H * scale
    rows = (95 + cols - 1) // cols
    img = Image.new("RGB", (pad + cols * (gw + pad), pad + rows * (gh + pad)), (24, 24, 32))
    draw = ImageDraw.Draw(img)
    for i in range(95):
        code = 0x20 + i
        cx = pad + (i % cols) * (gw + pad)
        cy = pad + (i // cols) * (gh + pad)
        for ry in range(H):
            # rebuild the 16-bit row: byte0 = bits 15..8, byte1 = bits 7..0
            row = (glyphs[code][ry] >> 8) | ((glyphs[code][ry] & 0xFF) << 8)
            for rx in range(W):
                if row & (0x8000 >> rx):
                    draw.rectangle([cx + rx * scale, cy + ry * scale, cx + rx * scale + scale - 1, cy + ry * scale + scale - 1], fill=(235, 235, 240))
        draw.rectangle([cx - 2, cy - 2, cx + gw + 1, cy + gh + 1], outline=(90, 90, 110))
    img.save(PREVIEW_PATH)


if __name__ == "__main__":
    main()
