#!/usr/bin/env python3
"""Cut a top view (in its own frame, centered on a point) into web mercator tiles: <out>/<z>/<x>/<y>.png

  make_tiles.py IMAGE OUT --res 0.01 --lat0 51.990634 --lon0 4.376789 --x-bearing 247 [--zmin 18 --zmax 23]

IMAGE: x to the right, y up, its center at lat0/lon0, RES m per pixel. --x-bearing: the bearing (deg, from
north, clockwise) of the image x axis on the ground. Outside the image the tiles are transparent.
"""
import argparse, math, os
import numpy as np
from PIL import Image

ap = argparse.ArgumentParser()
ap.add_argument("image"); ap.add_argument("out")
ap.add_argument("--res", type=float, required=True)
ap.add_argument("--lat0", type=float, required=True); ap.add_argument("--lon0", type=float, required=True)
ap.add_argument("--x-bearing", type=float, required=True)
ap.add_argument("--crop", type=float, default=0, help="keep only this half width (m) around the center, in the image frame")
ap.add_argument("--crop-box", type=float, nargs=4, metavar=("XMIN", "XMAX", "YMIN", "YMAX"),
                help="keep only this box (m, image frame: x right, y up)")
ap.add_argument("--zmin", type=int, default=18); ap.add_argument("--zmax", type=int, default=23)
a = ap.parse_args()
if a.crop > 0 and not a.crop_box:
    a.crop_box = [-a.crop, a.crop, -a.crop, a.crop]

src = np.asarray(Image.open(a.image).convert("RGBA")).astype(np.float32)
H, W = src.shape[:2]
R = 6378137.0
coslat = math.cos(math.radians(a.lat0))
b = math.radians(a.x_bearing)
ex = np.array([math.sin(b), math.cos(b)])          # image x axis in (east, north)
ey = np.array([-ex[1], ex[0]])                      # image y axis: 90 deg left of x (right-handed, seen from above)


def sample(east, north):
    """bilinear, transparent outside"""
    u = (east * ex[0] + north * ex[1]) / a.res + W / 2          # image column
    v = H / 2 - (east * ey[0] + north * ey[1]) / a.res          # image row
    u0 = np.floor(u - 0.5).astype(int); v0 = np.floor(v - 0.5).astype(int)
    fu = (u - 0.5 - u0)[..., None]; fv = (v - 0.5 - v0)[..., None]
    out = np.zeros(u.shape + (4,), np.float32)
    acc = 0
    for du, dv, w in ((0, 0, (1 - fu) * (1 - fv)), (1, 0, fu * (1 - fv)), (0, 1, (1 - fu) * fv), (1, 1, fu * fv)):
        uu = u0 + du; vv = v0 + dv
        ok = (uu >= 0) & (uu < W) & (vv >= 0) & (vv < H)
        if a.crop_box:
            xm = (uu + 0.5 - W / 2) * a.res; ym = (H / 2 - vv - 0.5) * a.res
            ok &= (xm >= a.crop_box[0]) & (xm <= a.crop_box[1]) & (ym >= a.crop_box[2]) & (ym <= a.crop_box[3])
        ok = ok[..., None]
        out += np.where(ok, src[np.clip(vv, 0, H - 1), np.clip(uu, 0, W - 1)], 0) * w
    return out


def lonlat_to_tile(lon, lat, z):
    n = 2 ** z
    x = (lon + 180) / 360 * n
    y = (1 - math.asinh(math.tan(math.radians(lat))) / math.pi) / 2 * n
    return x, y


half = math.hypot(W, H) / 2 * a.res if not a.crop_box else math.hypot(max(map(abs, a.crop_box[:2])), max(map(abs, a.crop_box[2:])))                 # the image rotated: all of it within this radius
dlat = math.degrees(half / R); dlon = math.degrees(half / (R * coslat))
count = 0
for z in range(a.zmin, a.zmax + 1):
    x0, y0 = lonlat_to_tile(a.lon0 - dlon, a.lat0 + dlat, z)
    x1, y1 = lonlat_to_tile(a.lon0 + dlon, a.lat0 - dlat, z)
    for tx in range(int(x0), int(x1) + 1):
        for ty in range(int(y0), int(y1) + 1):
            # each pixel center of the tile: lon, lat, then east, north from lat0, lon0
            px = (np.arange(256) + 0.5) / 256
            gx, gy = np.meshgrid(tx + px, ty + px)
            n = 2 ** z
            lon = gx / n * 360 - 180
            lat = np.degrees(np.arctan(np.sinh(np.pi * (1 - 2 * gy / n))))
            east = np.radians(lon - a.lon0) * R * coslat
            north = np.radians(lat - a.lat0) * R
            # a tile smaller than the image pixels at this zoom: averaged over 4x4 sub pixels
            mpp = 2 * math.pi * R * coslat / n / 256
            if mpp > a.res * 1.5:
                k = 4
                sub = (np.arange(k) + 0.5) / k - 0.5
                acc = np.zeros((256, 256, 4), np.float32)
                for su in sub:
                    for sv in sub:
                        acc += sample(east + su * mpp, north - sv * mpp)
                tile = acc / (k * k)
            else:
                tile = sample(east, north)
            if tile[..., 3].max() < 1:
                continue
            d = os.path.join(a.out, str(z), str(tx))
            os.makedirs(d, exist_ok=True)
            Image.fromarray(np.clip(tile, 0, 255).astype(np.uint8), "RGBA").save(os.path.join(d, f"{ty}.png"), optimize=True)
            count += 1
print(f"{count} tiles in {a.out}")
