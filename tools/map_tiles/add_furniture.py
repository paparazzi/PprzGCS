#!/usr/bin/env python3
"""Draw tables and chairs on a top view from render_cyberzoo.py (same frame: x right, y up, centered).

  add_furniture.py IN OUT RES HALF

A row of tables along the +y side of the Cyberzoo (south-south-east with the TU Delft frame), parallel
to the trusses, the chairs on the far side, facing the zoo.
"""
import sys
from PIL import Image, ImageDraw

IN, OUT = sys.argv[1], sys.argv[2]
RES = float(sys.argv[3]); HALF = float(sys.argv[4])

DEPTH = 0.8                 # m, the tables
# along 80 % of the side, from its +x end (west, on the left seen from there): x 5 to -3 m, touching:
# three long tables and a short one on the right, (length, chairs) from left to right
ROW = (5.0, -3.0)
SHORT = 1.6
LONG = (abs(ROW[0] - ROW[1]) - SHORT) / 3
TABLES = [(LONG, 2), (LONG, 2), (LONG, 2), (SHORT, 1)]
TABLE_Y = 5.0 + 0.15 + DEPTH / 2   # 15 cm out of the floor (its edge at 5 m)
CHAIR = 0.46

FLOOR = 5.0                 # the zoo floor: -5..5 m
HALL = (172, 174, 172, 255)  # the hall floor around it (the model has only part of it)

img = Image.open(IN).convert("RGBA")
# one even hall floor outside the zoo floor
hall = Image.new("RGBA", img.size, HALL)
mask = Image.new("L", img.size, 255)
f0 = ((-FLOOR + HALF) / RES, (HALF - FLOOR) / RES); f1 = ((FLOOR + HALF) / RES, (HALF + FLOOR) / RES)
ImageDraw.Draw(mask).rectangle((f0, f1), fill=0)
img.paste(hall, (0, 0), mask)
ov = Image.new("RGBA", img.size, (0, 0, 0, 0))
d = ImageDraw.Draw(ov)


def px(x, y):
    return ((x + HALF) / RES, (HALF - y) / RES)


def rect(cx, cy, w, h, **kw):
    x0, y0 = px(cx - w / 2, cy + h / 2); x1, y1 = px(cx + w / 2, cy - h / 2)
    d.rounded_rectangle((x0, y0, x1, y1), radius=0.03 / RES, **kw)


TABLES_X = []
x = ROW[0]
for length, chairs in TABLES:
    tx = x - length / 2
    x -= length
    TABLES_X.append(tx)
    # chairs behind the table (away from the zoo), seat and back
    for k in range(chairs):
        cx = tx + (k - (chairs - 1) / 2) * length / chairs
        cy = TABLE_Y + DEPTH / 2 + CHAIR / 2 + 0.05
        rect(cx + 0.02, cy - 0.02, CHAIR, CHAIR, fill=(0, 0, 0, 60))                       # shadow
        rect(cx, cy, CHAIR, CHAIR, fill=(60, 64, 72, 255), outline=(30, 32, 36, 255), width=2)
        rect(cx, cy + CHAIR / 2 - 0.05, CHAIR, 0.08, fill=(40, 42, 48, 255))               # back
    # the table top, light with a darker edge, and its shadow
    rect(tx + 0.03, TABLE_Y - 0.03, length, DEPTH, fill=(0, 0, 0, 70))
    rect(tx, TABLE_Y, length, DEPTH, fill=(222, 214, 196, 255), outline=(150, 140, 120, 255), width=3)

# a computer screen on the left part of the left table (+x: the left of who sits there, facing the zoo):
# at the zoo side of the table, facing the chair, its foot and a keyboard in front of it
sx = TABLES_X[0] + TABLES[0][0] / 4
sy = TABLE_Y - DEPTH / 2 + 0.12
rect(sx, sy + 0.09, 0.22, 0.14, fill=(95, 98, 104, 255))                                  # foot
rect(sx + 0.015, sy - 0.015, 0.60, 0.05, fill=(0, 0, 0, 70))                              # shadow
rect(sx, sy, 0.60, 0.05, fill=(18, 20, 24, 255), outline=(8, 8, 10, 255), width=2)        # screen
rect(sx, sy + 0.32, 0.44, 0.14, fill=(70, 72, 78, 255), outline=(45, 46, 50, 255), width=2)  # keyboard

img.alpha_composite(ov)
img.convert("RGB").save(OUT)
print(f"{len(TABLES)} tables ({LONG:.2f} m and {SHORT} m), {sum(c for _, c in TABLES)} chairs -> {OUT}")
