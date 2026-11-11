#!/usr/bin/env python3
"""Top view of the Cyberzoo Gazebo model, drawn from its COLLADA meshes (no GPU needed).

Each triangle is drawn from above with its texture (affine per triangle) or its color, the lowest first.
Output: a PNG in the model frame, x to the right, y up, centered on the model origin.
"""
import sys, os, math
import xml.etree.ElementTree as ET
import numpy as np
from PIL import Image, ImageDraw

NS = {"c": "http://www.collada.org/2005/11/COLLADASchema"}
MODEL = sys.argv[1]            # .../models/cyberzoo
OUT = sys.argv[2]              # output png
RES = float(sys.argv[3]) if len(sys.argv) > 3 else 0.01   # m per pixel
HALF = float(sys.argv[4]) if len(sys.argv) > 4 else 8.0   # half width covered (m)
PARTS = ["cyberzoo_floor", "cyberzoo_surroundings", "cyberzoo_toolcart", "cyberzoo_poles", "cyberzoo_net"]
OFFSET = np.array([-5.0, -5.0, 0.0])   # the <pose> of each visual in cyberzoo.sdf
ZMAX = 2.6                             # above: lamps, cameras, top of the net (left out)

N = int(round(2 * HALF / RES))
img = Image.new("RGBA", (N, N), (235, 235, 235, 255))


def to_px(x, y):
    return ((x + HALF) / RES, (HALF - y) / RES)


def floats(el):
    return np.array([float(v) for v in el.text.split()])


uprights = []   # (part, poly_px)
tris = []   # (zmean, poly_px, color, texture, uv)
for part in PARTS:
    path = os.path.join(MODEL, part + ".dae")
    root = ET.parse(path).getroot()
    unit = float(root.find("c:asset/c:unit", NS).get("meter"))
    # images, effects (color or texture), materials
    images = {i.get("id"): i.find("c:init_from", NS).text for i in root.iterfind(".//c:library_images/c:image", NS)}
    effects = {}
    for eff in root.iterfind(".//c:library_effects/c:effect", NS):
        prof = eff.find("c:profile_COMMON", NS)
        surf = {p.get("sid"): p for p in prof.iterfind("c:newparam", NS)}
        diff = prof.find(".//c:diffuse", NS)
        tex = diff.find("c:texture", NS) if diff is not None else None
        col = diff.find("c:color", NS) if diff is not None else None
        trans = prof.find(".//c:transparency/c:float", NS)
        alpha = float(trans.text) if trans is not None else 1.0
        if tex is not None:
            sampler = surf[tex.get("texture")]
            src = sampler.find(".//c:source", NS).text
            img_id = surf[src].find(".//c:init_from", NS).text
            effects[eff.get("id")] = ("tex", os.path.join(MODEL, images[img_id]), alpha)
        elif col is not None:
            effects[eff.get("id")] = ("col", tuple(float(v) for v in col.text.split()), alpha)
    materials = {m.get("id"): effects.get(m.find("c:instance_effect", NS).get("url")[1:])
                 for m in root.iterfind(".//c:library_materials/c:material", NS)}
    geoms = {g.get("id"): g for g in root.iterfind(".//c:library_geometries/c:geometry", NS)}
    lib_nodes = {n.get("id"): n for n in root.iterfind(".//c:library_nodes/c:node", NS)}

    def node_matrix(node):
        m = np.eye(4)
        for t in node:
            tag = t.tag.split("}")[1]
            if tag == "matrix":
                m = m @ floats(t).reshape(4, 4)
            elif tag == "translate":
                tm = np.eye(4); tm[:3, 3] = floats(t); m = m @ tm
        return m

    def add_geometry(geom, mat, binds):
        mesh = geom.find("c:mesh", NS)
        sources = {s.get("id"): floats(s.find("c:float_array", NS)).reshape(-1, int(s.find(".//c:accessor", NS).get("stride")))
                   for s in mesh.iterfind("c:source", NS)}
        verts = mesh.find("c:vertices", NS)
        vpos = sources[verts.find("c:input[@semantic='POSITION']", NS).get("source")[1:]]
        for tri in list(mesh.iterfind("c:triangles", NS)) + list(mesh.iterfind("c:polylist", NS)):
            inputs = tri.findall("c:input", NS)
            stride = max(int(i.get("offset")) for i in inputs) + 1
            flat = np.array([int(v) for v in tri.find("c:p", NS).text.split()]).reshape(-1, stride)
            vc = tri.find("c:vcount", NS)
            if vc is None:
                p = flat.reshape(-1, 3, stride)
            else:
                # polygons: fans of triangles
                fan, k = [], 0
                for n in (int(v) for v in vc.text.split()):
                    for i in range(1, n - 1):
                        fan.append([flat[k], flat[k + i], flat[k + i + 1]])
                    k += n
                if not fan:
                    continue
                p = np.array(fan)
            off_v = int([i for i in inputs if i.get("semantic") == "VERTEX"][0].get("offset"))
            tc = [i for i in inputs if i.get("semantic") == "TEXCOORD"]
            eff = materials.get(binds.get(tri.get("material"), ""), None)
            pts = vpos[p[:, :, off_v]]                             # (n,3,3)
            pts = np.concatenate([pts, np.ones(pts.shape[:2] + (1,))], axis=2) @ mat.T
            pts = pts[:, :, :3] * unit + OFFSET
            uvs = sources[tc[0].get("source")[1:]][p[:, :, int(tc[0].get("offset"))]][:, :, :2] if tc else None
            for k in range(len(pts)):
                t = pts[k]
                # upright (poles, the net): drawn as their footprint, on top, at any height
                n3 = np.cross(t[1] - t[0], t[2] - t[0])
                if np.linalg.norm(n3) > 1e-9 and abs(n3[2]) < 0.2 * np.linalg.norm(n3):
                    if part in ("cyberzoo_poles", "cyberzoo_net"):
                        uprights.append((part, [to_px(x, y) for x, y, _ in t]))
                    continue
                if t[:, 2].max() > ZMAX:
                    continue
                # the floor of the zoo above the floor of the hall around it (both at about 0 m)
                tris.append((t[:, 2].mean() + (0.1 if part == "cyberzoo_floor" else 0), [to_px(x, y) for x, y, _ in t], eff, None if uvs is None else uvs[k]))

    def walk(node, mat):
        mat = mat @ node_matrix(node)
        for ig in node.iterfind("c:instance_geometry", NS):
            binds = {b.get("symbol"): b.get("target")[1:] for b in ig.iterfind(".//c:instance_material", NS)}
            add_geometry(geoms[ig.get("url")[1:]], mat, binds)
        for inode in node.iterfind("c:instance_node", NS):
            walk(lib_nodes[inode.get("url")[1:]], mat)
        for child in node.iterfind("c:node", NS):
            walk(child, mat)

    for node in root.iterfind(".//c:library_visual_scenes/c:visual_scene/c:node", NS):
        walk(node, np.eye(4))

textures = {}
draw = ImageDraw.Draw(img)
tris.sort(key=lambda t: t[0])
for z, poly, eff, uv in tris:
    xs = [p[0] for p in poly]; ys = [p[1] for p in poly]
    if max(xs) < 0 or min(xs) > N or max(ys) < 0 or min(ys) > N:
        continue
    area = abs((xs[1]-xs[0])*(ys[2]-ys[0]) - (xs[2]-xs[0])*(ys[1]-ys[0]))
    if area < 0.5:      # seen edge-on from above (walls, the vertical net)
        continue
    if eff is None:
        draw.polygon(poly, fill=(180, 180, 180, 255)); continue
    kind, val, alpha = eff
    a = int(255 * alpha)
    if kind == "col":
        draw.polygon(poly, fill=tuple(int(255 * c) for c in val[:3]) + (a,)); continue
    if val not in textures:
        textures[val] = Image.open(val).convert("RGBA")
    tex = textures[val]
    tw, th = tex.size
    # the pixel -> texture pixel affine map of this triangle (texture repeated: tiled large enough)
    U = np.array([[u * tw, (1 - v) * th] for u, v in uv])
    shift = np.floor(U.min(axis=0) / [tw, th]) * [tw, th]
    U -= shift
    reps = np.ceil(U.max(axis=0) / [tw, th]).astype(int) + 1
    if reps.prod() > 400:
        avg = tex.resize((1, 1)).getpixel((0, 0))
        draw.polygon(poly, fill=avg[:3] + (a,)); continue
    big = Image.new("RGBA", (tw * reps[0], th * reps[1]))
    for i in range(reps[0]):
        for j in range(reps[1]):
            big.paste(tex, (i * tw, j * th))
    P = np.array([[x, y, 1] for x, y in poly])
    try:
        A = np.linalg.solve(P, U)       # [x y 1] @ A = [tu tv]
    except np.linalg.LinAlgError:
        continue
    x0, y0 = int(max(0, math.floor(min(xs)))), int(max(0, math.floor(min(ys))))
    x1, y1 = int(min(N, math.ceil(max(xs)))), int(min(N, math.ceil(max(ys))))
    if x1 <= x0 or y1 <= y0:
        continue
    # PIL affine: output (x,y) -> input (a x + b y + c, d x + e y + f), in the patch coordinates
    coeffs = (A[0, 0], A[1, 0], A[2, 0] + A[0, 0] * x0 + A[1, 0] * y0,
              A[0, 1], A[1, 1], A[2, 1] + A[0, 1] * x0 + A[1, 1] * y0)
    patch = big.transform((x1 - x0, y1 - y0), Image.AFFINE, coeffs, resample=Image.BILINEAR)
    mask = Image.new("L", patch.size, 0)
    ImageDraw.Draw(mask).polygon([(x - x0, y - y0) for x, y in poly], fill=a)
    if patch.mode == "RGBA":
        mask = Image.fromarray(np.minimum(np.array(mask), np.array(patch.split()[3])))
    img.paste(patch, (x0, y0), mask)

# the poles (one in each corner of the model) and the net between them: the net of the model is a
# simplified plane on two sides only
pole_pts = np.array([q for pp, poly in uprights if pp == "cyberzoo_poles" for q in poly])
cx, cy = N / 2, N / 2
centers = []
for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
    q = pole_pts[((pole_pts[:, 0] - cx) * sx > 0) & ((pole_pts[:, 1] - cy) * sy > 0)]
    if len(q):
        centers.append(tuple(q.mean(axis=0)))
# the sides are trusses (seen from above: two rails and the zig-zag between them), TRUSS m wide
TRUSS = 0.29
rail = (55, 55, 55, 255)
ov = Image.new("RGBA", img.size, (0, 0, 0, 0))
od = ImageDraw.Draw(ov)
if len(centers) == 4:
    w = TRUSS / RES
    for i in range(4):
        p0 = np.array(centers[i]); p1 = np.array(centers[(i + 1) % 4])
        d = p1 - p0; L = np.linalg.norm(d); d /= L
        nrm = np.array([-d[1], d[0]]) * w / 2
        od.polygon([tuple(p0 + nrm), tuple(p1 + nrm), tuple(p1 - nrm), tuple(p0 - nrm)], fill=(200, 200, 200, 110))
        for sgn in (1, -1):
            od.line([tuple(p0 + sgn * nrm), tuple(p1 + sgn * nrm)], fill=rail, width=3)
        steps = max(1, int(L / w))
        pts = [tuple(p0 + d * L * k / steps + (nrm if k % 2 else -nrm)) for k in range(steps + 1)]
        od.line(pts, fill=rail, width=2)
    # a square tower in each corner
    for x, y in centers:
        h = w / 2
        od.rectangle((x - h, y - h, x + h, y + h), outline=rail, width=3, fill=(120, 120, 120, 255))
img.alpha_composite(ov)
img.convert("RGB").save(OUT)
print(f"{len(tris)} triangles, {N}x{N} px, {RES*100:.1f} cm/px -> {OUT}")
