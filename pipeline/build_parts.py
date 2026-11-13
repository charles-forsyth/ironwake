# Replace fused moving parts with procedural ones and export <name>_parts.glb (body + PART_* with origin at the axle).
# blender -b -P build_parts.py -- in.glb cfg.json outdir name [--gpu]
# cfg[name] = {"parts":[{"type":"prop|wheel|sails","name":..,"c":[fx,fy,fz] (fractions of bbox),"axis":"X|Y|Z","r":radius (model units),
#                        "depth":[..] optional cylinder depth for removing fused geometry}]}
import bpy, bmesh, sys, os, json, math
from mathutils import Vector, Matrix
a = sys.argv[sys.argv.index("--") + 1:]
src, cfgf, outdir, name = a[0], a[1], a[2], a[3]
GPU = "--gpu" in a
cfg = json.load(open(cfgf))[name]
os.makedirs(outdir, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
ms = [o for o in bpy.data.objects if o.type == "MESH"]
for o in ms: o.select_set(True)
bpy.context.view_layer.objects.active = ms[0]
if len(ms) > 1: bpy.ops.object.join()
body = bpy.context.view_layer.objects.active
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
vs = [v.co.copy() for v in body.data.vertices]
lo = Vector([min(v[i] for v in vs) for i in range(3)]); hi = Vector([max(v[i] for v in vs) for i in range(3)]); sz = hi - lo
AX = {"X": 0, "Y": 1, "Z": 2}

def mat(color, metal=0.0, rough=0.5):
    m = bpy.data.materials.new("m"); m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]; b.inputs["Base Color"].default_value = color
    b.inputs["Metallic"].default_value = metal; b.inputs["Roughness"].default_value = rough
    return m
WOOD = mat((0.32, 0.2, 0.1, 1), 0.0, 0.7); BRASS = mat((0.7, 0.5, 0.2, 1), 0.9, 0.35); IRON = mat((0.08, 0.08, 0.09, 1), 0.8, 0.5)

def new_obj(nm, bm, m):
    me = bpy.data.meshes.new(nm); bm.to_mesh(me); bm.free(); me.materials.append(m)
    o = bpy.data.objects.new(nm, me); bpy.context.scene.collection.objects.link(o); return o

def box(bm, c, s, rotz=0.0, rotx=0.0):
    g = bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=s, verts=g["verts"])
    bmesh.ops.rotate(bm, cent=(0, 0, 0), matrix=Matrix.Rotation(rotx, 3, "X") @ Matrix.Rotation(rotz, 3, "Z"), verts=g["verts"])
    bmesh.ops.translate(bm, vec=c, verts=g["verts"])

def cyl(bm, c, r, h):
    g = bmesh.ops.create_cone(bm, cap_ends=True, segments=16, radius1=r, radius2=r, depth=h)
    bmesh.ops.translate(bm, vec=c, verts=g["verts"])

def orient(o, axis, c):
    # build parts with their shaft along local Z, then rotate so Z -> chosen axis, place origin at c
    me = o.data
    if axis == "X": R = Matrix.Rotation(math.radians(90), 4, "Y")
    elif axis == "Y": R = Matrix.Rotation(math.radians(-90), 4, "X")
    else: R = Matrix.Identity(4)
    me.transform(R); o.location = Vector(c)

def make_prop(r):
    bm = bmesh.new(); cyl(bm, (0, 0, 0), r * 0.13, r * 0.28)
    for k in range(4):
        ang = k * math.pi / 2
        # blade: flat box out from hub, pitched
        for seg in range(4):
            t0 = (seg + 0.5) / 4; rr = r * (0.18 + 0.8 * t0)
            w = r * (0.20 - 0.07 * t0)
            g = bmesh.ops.create_cube(bm, size=1.0)
            bmesh.ops.scale(bm, vec=(r * 0.2, w, r * 0.025), verts=g["verts"])
            bmesh.ops.rotate(bm, cent=(0, 0, 0), matrix=Matrix.Rotation(math.radians(28 + 18 * (1 - t0)), 3, "X"), verts=g["verts"])
            bmesh.ops.translate(bm, vec=(rr, 0, 0), verts=g["verts"])
            bmesh.ops.rotate(bm, cent=(0, 0, 0), matrix=Matrix.Rotation(ang, 3, "Z"), verts=g["verts"])
    return new_obj("p", bm, WOOD)

def make_wheel(r, width):
    bm = bmesh.new(); cyl(bm, (0, 0, 0), r * 0.1, width * 1.1)
    for zz in (-width * 0.5, width * 0.5):
        # rim ring from 24 short boxes
        for k in range(24):
            ang = k * 2 * math.pi / 24
            box(bm, (r * math.cos(ang), r * math.sin(ang), zz), (r * 0.28, r * 0.05, r * 0.05), rotz=ang + math.pi / 2)
        for k in range(8):
            ang = k * 2 * math.pi / 8
            box(bm, (r * 0.5 * math.cos(ang), r * 0.5 * math.sin(ang), zz), (r * 0.9, r * 0.04, r * 0.04), rotz=ang)
    for k in range(12):
        ang = k * 2 * math.pi / 12
        box(bm, (r * math.cos(ang), r * math.sin(ang), 0), (r * 0.16, r * 0.04, width), rotz=ang)
    return new_obj("w", bm, WOOD)

def make_sails(r):
    bm = bmesh.new(); cyl(bm, (0, 0, 0), r * 0.05, r * 0.12)
    for k in range(4):
        ang = k * math.pi / 2 + math.radians(45)
        c, s = math.cos(ang), math.sin(ang)
        box(bm, (c * r * 0.5, s * r * 0.5, 0), (r * 1.0, r * 0.03, r * 0.03), rotz=ang)          # stock
        for side in (-1, 1):                                                                         # sail frame
            ox, oy = -s * r * 0.11 * side, c * r * 0.11 * side
            for i in range(7):
                t = 0.22 + 0.13 * i
                box(bm, (c * r * t + ox, s * r * t + oy, 0), (r * 0.01, r * 0.17, r * 0.01), rotz=ang)  # lattice slats
        box(bm, (c * r * 0.6 - s * r * 0.22, s * r * 0.6 + c * r * 0.22, 0), (r * 0.8, r * 0.01, r * 0.01), rotz=ang)
        box(bm, (c * r * 0.6 + s * r * 0.22, s * r * 0.6 - c * r * 0.22, 0), (r * 0.8, r * 0.01, r * 0.01), rotz=ang)
    return new_obj("s", bm, WOOD)


def make_oar(length):
    # local +Z = outboard. Origin at the rowlock. Inboard (grip) 1/3 of the length, outboard 2/3 incl. spoon blade.
    bm = bmesh.new(); L = length
    cyl(bm, (0, 0, L * (0.667 - 0.333) / 2.0 - 0.0), L * 0.011, L * 0.80)                  # loom, centred so it spans -0.333L .. +0.467L
    cyl(bm, (0, 0, L * 0.12), L * 0.020, L * 0.07)                                              # leather collar at the rowlock side
    cyl(bm, (0, 0, -L * 0.30), L * 0.016, L * 0.07)                                             # grip
    for k in range(6):                                                                          # spoon blade, slightly cupped and tapering
        t = k / 5.0; z = L * (0.50 + 0.17 * t); w = L * (0.070 - 0.012 * abs(t - 0.45))
        box(bm, (0, -L * 0.006 * math.sin(math.pi * t), z), (w, L * 0.005, L * 0.034))
    return new_obj("o", bm, WOOD)

# face-level removal: delete every face whose centre lies inside the part's swept cylinder (axis, radius*1.05, +-depth)
bm = bmesh.new(); bm.from_mesh(body.data); bm.faces.ensure_lookup_table()
report = {"model": name, "parts": []}
kill = set()
for part in cfg["parts"]:
    c = lo + Vector([part["c"][i] * sz[i] for i in range(3)]); ax = AX[part["axis"]]; r = part["r"]
    dep = part.get("depth", 0.12 * max(sz)); n = 0
    for f in bm.faces:
        d = f.calc_center_median() - c; along = d[ax]; perp = (d.length_squared - along * along) ** 0.5
        if abs(along) <= dep and perp <= r * 1.05: kill.add(f.index); n += 1
    part["removed_faces"] = n
# debug: paint the faces about to be removed red, keep a copy for a render
dbg_idx = sorted(kill)
dbg = body.copy(); dbg.data = body.data.copy(); bpy.context.scene.collection.objects.link(dbg)
dm = bpy.data.materials.new("r"); dm.use_nodes = True
db = dm.node_tree.nodes["Principled BSDF"]; db.inputs["Base Color"].default_value = (1, 0, 0, 1); db.inputs["Emission Color"].default_value = (1, 0, 0, 1); db.inputs["Emission Strength"].default_value = 3
dbg.data.materials.append(dm); ri = len(dbg.data.materials) - 1
ks = set(dbg_idx)
for pl in dbg.data.polygons:
    if pl.index in ks: pl.material_index = ri
dbg.name = "DEBUG_KILL"
for i in dbg_idx:
    f = bm.faces[i]
    if f.is_valid: pass
bmesh.ops.delete(bm, geom=[bm.faces[i] for i in dbg_idx], context="FACES")
bm.to_mesh(body.data); bm.free(); body.data.update()
for part in cfg["parts"]:
    c = lo + Vector([part["c"][i] * sz[i] for i in range(3)])
    o = {"prop": lambda: make_prop(part["r"]), "wheel": lambda: make_wheel(part["r"], part.get("width", part["r"] * 0.7)), "sails": lambda: make_sails(part["r"]), "oar": lambda: make_oar(part["r"])}[part["type"]]()
    o.name = "PART_" + part["name"]
    if part["type"] == "oar":
        # oar built along local +Z; point it outboard (+X for stbd, -X for port), angled slightly aft, resting at the rowlock
        sgn = 1.0 if part.get("side") == "stbd" else -1.0
        o.data.transform(Matrix.Rotation(math.radians(90 * sgn), 4, "Y"))           # +Z -> +/-X (outboard)
        o.data.transform(Matrix.Rotation(math.radians(18 * sgn), 4, "Z"))           # both sides sweep aft (-Y) (flipped from first try)
        o.data.transform(Matrix.Rotation(math.radians(8 * sgn), 4, "Y"))           # roll about the fore-aft axis: outboard end dips on both sides
        o.location = Vector(c)
    else:
        orient(o, part["axis"], c)
    report["parts"].append({"name": part["name"], "type": part["type"], "axis": part["axis"], "pivot": list(c), "radius": part["r"], "removed_faces": part["removed_faces"]})
dbg_obj = bpy.data.objects["DEBUG_KILL"]; dbg_obj.hide_render = True; dbg_obj.hide_set(True)
bpy.ops.export_scene.gltf(filepath=f"{outdir}/{name}_parts.glb", export_format="GLB", use_visible=True)
json.dump(report, open(f"{outdir}/{name}_parts.json", "w"), indent=1)
# proof renders: three ortho views, parts red-tinted copy not needed; plain look + a 0/30 deg spin pair
sc = bpy.context.scene
if GPU:
    sc.render.engine = "CYCLES"; sc.cycles.samples = 48; sc.cycles.use_denoising = True
    pr = bpy.context.preferences.addons["cycles"].preferences
    pr.compute_device_type = "CUDA"; pr.get_devices()
    for d in pr.devices: d.use = d.type != "CPU"
    sc.cycles.device = "GPU"
else:
    try: sc.render.engine = "BLENDER_EEVEE"
    except Exception: sc.render.engine = "CYCLES"
sc.render.resolution_x = sc.render.resolution_y = 800
w = bpy.data.worlds.new("w"); sc.world = w; w.color = (0.45, 0.5, 0.55)
sun = bpy.data.objects.new("s", bpy.data.lights.new("s", "SUN")); sun.data.energy = 3; sun.rotation_euler = (math.radians(50), 0, math.radians(30)); sc.collection.objects.link(sun)
cam = bpy.data.objects.new("c", bpy.data.cameras.new("c")); sc.collection.objects.link(cam); sc.camera = cam
cam.data.type = "ORTHO"; cam.data.clip_end = 10000; cam.data.ortho_scale = max(sz) * 1.1
ctr = lo + sz / 2; D = sz.length * 2
dbg_obj.hide_render = False; body.hide_render = True
for o in bpy.data.objects:
    if o.name.startswith("PART_"): o.hide_render = True
dbg_obj.hide_render = False
for o in bpy.data.objects:
    if o.name.startswith("PART_"): o.hide_render = False
body.hide_render = True
for vn, loc, rot in (("top", (0, 0, D), (0, 0, 0)), ("side", (0, -D, 0), (90, 0, 0))):
    cam.location = ctr + Vector(loc); cam.rotation_euler = [math.radians(r_) for r_ in rot]
    sc.render.filepath = f"{outdir}/{name}_kill_{vn}.png"; bpy.ops.render.render(write_still=True)
body.hide_render = False; dbg_obj.hide_render = True
for ang in (0, 30):
    for o in bpy.data.objects:
        if o.name.startswith("PART_"):
            p = next(p for p in report["parts"] if "PART_" + p["name"] == o.name)
            o.rotation_euler = [math.radians(ang) if i == AX[p["axis"]] else 0 for i in range(3)] if True else (0, 0, 0)
    for vn, loc, rot in (("top", (0, 0, D), (0, 0, 0)), ("front", (D, 0, 0), (90, 0, 90)), ("side", (0, -D, 0), (90, 0, 0))):
        cam.location = ctr + Vector(loc); cam.rotation_euler = [math.radians(r_) for r_ in rot]
        sc.render.filepath = f"{outdir}/{name}_bp_{vn}_{ang}.png"; bpy.ops.render.render(write_still=True)
print("BUILD_DONE", name, [(p["name"], p["removed_faces"]) for p in report["parts"]])
