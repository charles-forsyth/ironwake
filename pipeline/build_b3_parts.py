# Build procedural moving parts onto clean bodies3 hulls:
#   1) b3_paddle_steamer_body -> models/parts/b3_paddle_steamer_parts.glb + .json (PART_wheel_port, PART_wheel_stbd)
#   2) b3_salvage_tender_body -> models/parts/b3_salvage_tender_parts.glb + .json (PART_crane, PART_bell)
# Usage: blender -b -P build_b3_parts.py -- <bodies3_dir> <parts_outdir> <scratch_proof_dir>
import bpy, bmesh, sys, os, json, math
from mathutils import Vector, Matrix

a = sys.argv[sys.argv.index("--") + 1:]
in_dir, out_dir, proof_dir = a[0], a[1], a[2]
os.makedirs(out_dir, exist_ok=True)
os.makedirs(proof_dir, exist_ok=True)

def make_mat(nm, c, metal=0.0, rough=0.65):
    m = bpy.data.materials.new(nm)
    m.use_nodes = True
    bsdf = m.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = c
    bsdf.inputs["Metallic"].default_value = metal
    bsdf.inputs["Roughness"].default_value = rough
    return m

def bm_to_obj(nm, bm, mat):
    me = bpy.data.meshes.new(nm)
    bm.to_mesh(me)
    bm.free()
    me.materials.append(mat)
    o = bpy.data.objects.new(nm, me)
    bpy.context.scene.collection.objects.link(o)
    return o

def add_cyl(bm, p0, p1, r0, r1=None, segs=14):
    if r1 is None:
        r1 = r0
    d = p1 - p0
    if d.length < 1e-5:
        return
    mid = (p0 + p1) * 0.5
    g = bmesh.ops.create_cone(bm, cap_ends=True, segments=segs, radius1=r0, radius2=r1, depth=d.length)
    rot = Vector((0, 0, 1)).rotation_difference(d.normalized()).to_matrix().to_4x4()
    bmesh.ops.transform(bm, matrix=Matrix.Translation(mid) @ rot, verts=g["verts"])

def add_box(bm, center, sx, sy, sz, rot_mat=None):
    g = bmesh.ops.create_cube(bm, size=1.0)
    m = Matrix.Diagonal((sx, sy, sz, 1.0))
    if rot_mat is not None:
        m = rot_mat.to_4x4() @ m
    bmesh.ops.transform(bm, matrix=Matrix.Translation(center) @ m, verts=g["verts"])

def load_joined_body(glb_path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=glb_path)
    ms = [o for o in bpy.data.objects if o.type == "MESH"]
    for o in ms:
        o.select_set(True)
    bpy.context.view_layer.objects.active = ms[0]
    if len(ms) > 1:
        bpy.ops.object.join()
    b = bpy.context.view_layer.objects.active
    b.name = "HullBody"
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    return b

def setup_front_lit_cam(sz):
    sc = bpy.context.scene
    sc.render.engine = "BLENDER_EEVEE"
    sc.render.resolution_x = sc.render.resolution_y = 760
    w = bpy.data.worlds.new("w")
    sc.world = w
    w.color = (0.40, 0.44, 0.48)
    cam = bpy.data.objects.new("c", bpy.data.cameras.new("c"))
    sc.collection.objects.link(cam)
    sc.camera = cam
    cam.data.type = "ORTHO"
    cam.data.clip_end = 10000
    cam.data.ortho_scale = max(sz) * 1.18
    sun = bpy.data.objects.new("s", bpy.data.lights.new("s", "SUN"))
    sun.data.energy = 3.2
    sun.parent = cam
    sun.rotation_euler = (math.radians(20), math.radians(-16), 0)
    sc.collection.objects.link(sun)
    return sc, cam

def shoot(sc, cam, eye, target, path):
    cam.location = eye
    cam.rotation_euler = (target - eye).to_track_quat("-Z", "Y").to_euler()
    sc.render.filepath = path
    bpy.ops.render.render(write_still=True)

# ==============================================================================
# 1. PADDLE STEAMER: recessed wheel wells under enclosed sponsons + 2 wheels
# ==============================================================================
body = load_joined_body(f"{in_dir}/b3_paddle_steamer_body.glb")
WOOD = make_mat("IW_WheelWood", (0.28, 0.17, 0.085, 1.0), 0.0, 0.72)
RED_WOOD = make_mat("IW_FloatRed", (0.36, 0.11, 0.08, 1.0), 0.0, 0.68)
IRON = make_mat("IW_DarkIron", (0.06, 0.065, 0.07, 1.0), 0.75, 0.52)
WELL = make_mat("IW_WheelWell", (0.03, 0.03, 0.035, 1.0), 0.2, 0.90)
BRASS = make_mat("IW_Brass", (0.68, 0.50, 0.20, 1.0), 0.88, 0.34)

# Carve underside wheel-well recess in sponson bottom (only bottom floor faces at |y| in [0.232, 0.304], x in [-0.17, +0.27], z < -0.22)
bm_body = bmesh.new()
bm_body.from_mesh(body.data)
bm_body.faces.ensure_lookup_table()
del_faces = []
for f in bm_body.faces:
    fc = f.calc_center_median()
    if -0.175 <= fc.x <= 0.275 and 0.232 <= abs(fc.y) <= 0.304 and fc.z < -0.24 and f.normal.z < -0.25:
        del_faces.append(f)
removed_n = len(del_faces)
if del_faces:
    bmesh.ops.delete(bm_body, geom=del_faces, context="FACES")
bm_body.to_mesh(body.data)
bm_body.free()
body.data.update()

# Dark wheel-well shroud liner inside sponson so there's zero daylight gap looking up
bm_well = bmesh.new()
for sgn in (-1.0, 1.0):
    yc = sgn * 0.268
    # Top & fore/aft/inner dark liner walls above z=-0.35
    add_box(bm_well, Vector((0.050, yc, -0.16)), 0.44, 0.068, 0.02)
    add_box(bm_well, Vector((-0.170, yc, -0.26)), 0.02, 0.068, 0.20)
    add_box(bm_well, Vector((0.270, yc, -0.26)), 0.02, 0.068, 0.20)
o_well = bm_to_obj("_well_liner", bm_well, WELL)
bpy.ops.object.select_all(action="DESELECT")
body.select_set(True)
o_well.select_set(True)
bpy.context.view_layer.objects.active = body
bpy.ops.object.join()

def build_paddle_wheel(nm, r=0.208, width=0.060):
    # Axle along Y (local origin at hub centre (0,0,0); wheel plane is X-Z)
    bm_iron = bmesh.new()
    bm_wood = bmesh.new()
    # Central iron hub + brass end boss along Y
    add_cyl(bm_iron, Vector((0, -width * 0.62, 0)), Vector((0, width * 0.62, 0)), r * 0.16, r * 0.16, 16)
    for yy in (-width * 0.46, width * 0.46):
        # Outer & inner iron rim rings (24 segments in X-Z plane)
        for rr in (r * 0.90, r * 0.62):
            for k in range(24):
                a0 = k * 2.0 * math.pi / 24.0
                a1 = (k + 1) * 2.0 * math.pi / 24.0
                p0 = Vector((rr * math.cos(a0), yy, rr * math.sin(a0)))
                p1 = Vector((rr * math.cos(a1), yy, rr * math.sin(a1)))
                add_cyl(bm_iron, p0, p1, r * 0.022, r * 0.022, 6)
        # 10 radial spokes in X-Z plane
        for k in range(10):
            ang = k * 2.0 * math.pi / 10.0
            p1 = Vector((r * 0.92 * math.cos(ang), yy, r * 0.92 * math.sin(ang)))
            add_cyl(bm_iron, Vector((0, yy, 0)), p1, r * 0.024, r * 0.018, 6)
    # 12 heavy wooden paddle floats spanning width along Y
    for k in range(12):
        ang = k * 2.0 * math.pi / 12.0
        cx, cz = r * 0.92 * math.cos(ang), r * 0.92 * math.sin(ang)
        rm = Matrix.Rotation(-ang, 3, "Y")
        add_box(bm_wood, Vector((cx, 0.0, cz)), r * 0.22, width * 1.02, r * 0.042, rot_mat=rm)

    o_i = bm_to_obj(nm + "_iron", bm_iron, IRON)
    o_w = bm_to_obj(nm, bm_wood, RED_WOOD)
    bpy.ops.object.select_all(action="DESELECT")
    o_i.select_set(True)
    o_w.select_set(True)
    bpy.context.view_layer.objects.active = o_w
    bpy.ops.object.join()
    return o_w

steamer_piv_port = Vector((0.050, -0.268, -0.248))
steamer_piv_stbd = Vector((0.050, +0.268, -0.248))
w_port = build_paddle_wheel("PART_wheel_port", r=0.212, width=0.060)
w_port.location = steamer_piv_port
w_stbd = build_paddle_wheel("PART_wheel_stbd", r=0.212, width=0.060)
w_stbd.location = steamer_piv_stbd

steamer_report = {
    "model": "b3_paddle_steamer",
    "long_axis": "X",
    "bow_sign": -1,
    "sponson_underside_faces_removed": removed_n,
    "parts": [
        {"name": "PART_wheel_port", "kind": "wheel", "axis": "Y", "pivot": [round(x, 5) for x in steamer_piv_port], "radius": 0.212},
        {"name": "PART_wheel_stbd", "kind": "wheel", "axis": "Y", "pivot": [round(x, 5) for x in steamer_piv_stbd], "radius": 0.212},
    ],
}
bpy.ops.export_scene.gltf(filepath=f"{out_dir}/b3_paddle_steamer_parts.glb", export_format="GLB")
json.dump(steamer_report, open(f"{out_dir}/b3_paddle_steamer_parts.json", "w"), indent=1)

# Render steamer proof (0 deg and 18 deg wheel rotation, side + low 3/4 + underside 3/4)
vs = [v.co for v in body.data.vertices]
lo = Vector([min(v[i] for v in vs) for i in range(3)])
hi = Vector([max(v[i] for v in vs) for i in range(3)])
sz = hi - lo; ctr = (lo + hi) * 0.5; D = sz.length * 2.1
sc, cam = setup_front_lit_cam(sz)
for ang in (0, 18):
    w_port.rotation_euler = (0.0, math.radians(ang), 0.0)
    w_stbd.rotation_euler = (0.0, math.radians(ang), 0.0)
    shoot(sc, cam, ctr + Vector((0, -D, -0.05 * D)), ctr, f"{proof_dir}/steamer_side_{ang}.png")
    shoot(sc, cam, ctr + Vector((-0.65 * D, -0.72 * D, 0.18 * D)), ctr, f"{proof_dir}/steamer_3q_{ang}.png")
    shoot(sc, cam, ctr + Vector((-0.55 * D, -0.75 * D, -0.25 * D)), ctr, f"{proof_dir}/steamer_low_{ang}.png")
print("STEAMER_DONE removed_faces=", removed_n)

# ==============================================================================
# 2. SALVAGE TENDER: turntable winch + pivoting A-frame crane + diving bell
# ==============================================================================
body2 = load_joined_body(f"{in_dir}/b3_salvage_tender_body.glb")
IRON2 = make_mat("IW_TenderIron", (0.09, 0.085, 0.085, 1.0), 0.78, 0.52)
RUST = make_mat("IW_BellIron", (0.22, 0.11, 0.08, 1.0), 0.55, 0.65)
BRASS2 = make_mat("IW_TenderBrass", (0.72, 0.53, 0.22, 1.0), 0.88, 0.32)
CABLE = make_mat("IW_SteelCable", (0.16, 0.17, 0.18, 1.0), 0.80, 0.45)

# Static riveted bedplate ring bolted to aft working deck at (0.18, 0, 0.010)
bm_bed = bmesh.new()
add_cyl(bm_bed, Vector((0.18, 0.0, 0.005)), Vector((0.18, 0.0, 0.052)), 0.155, 0.145, 20)
o_bed = bm_to_obj("_crane_bedplate", bm_bed, IRON2)
bpy.ops.object.select_all(action="DESELECT")
body2.select_set(True)
o_bed.select_set(True)
bpy.context.view_layer.objects.active = body2
bpy.ops.object.join()

# PART_crane: origin at (0.18, 0.0, 0.052), slews around Z axis
crane_piv = Vector((0.18, 0.0, 0.052))
bm_crane = bmesh.new()
bm_cbrass = bmesh.new()
# Turntable platform + steam winch housing + drum (in crane local coords)
add_cyl(bm_crane, Vector((0, 0, 0.0)), Vector((0, 0, 0.028)), 0.140, 0.135, 18)
add_box(bm_crane, Vector((-0.03, 0.0, 0.075)), 0.14, 0.18, 0.09)
add_cyl(bm_cbrass, Vector((0.02, -0.105, 0.08)), Vector((0.02, 0.105, 0.08)), 0.042, 0.042, 14)
# Kingpost / backstay mast rising vertically on turntable
mast_top = Vector((-0.05, 0.0, 0.34))
add_cyl(bm_crane, Vector((-0.05, -0.06, 0.03)), mast_top, 0.018, 0.015, 10)
add_cyl(bm_crane, Vector((-0.05, +0.06, 0.03)), mast_top, 0.018, 0.015, 10)
# Twin-leg heavy A-frame derrick boom angling aft over the stern to local (0.88, 0, 0.44) -> world (1.06, 0, 0.492)
jib_head = Vector((0.88, 0.0, 0.44))
add_cyl(bm_crane, Vector((0.06, -0.10, 0.04)), jib_head + Vector((0, -0.022, 0)), 0.022, 0.015, 10)
add_cyl(bm_crane, Vector((0.06, +0.10, 0.04)), jib_head + Vector((0, +0.022, 0)), 0.022, 0.015, 10)
# Cross-bracing rungs along the A-frame boom
for t in (0.25, 0.50, 0.75):
    p_l = Vector((0.06, -0.10, 0.04)).lerp(jib_head + Vector((0, -0.022, 0)), t)
    p_r = Vector((0.06, +0.10, 0.04)).lerp(jib_head + Vector((0, +0.022, 0)), t)
    add_cyl(bm_crane, p_l, p_r, 0.010, 0.010, 8)
# Twin luffing tie-rods from kingpost top to jib head + hoist cable from winch drum to jib head
add_cyl(bm_crane, mast_top, jib_head + Vector((-0.02, -0.018, 0.01)), 0.006, 0.006, 8)
add_cyl(bm_crane, mast_top, jib_head + Vector((-0.02, +0.018, 0.01)), 0.006, 0.006, 8)
add_cyl(bm_crane, Vector((0.02, 0.0, 0.11)), jib_head, 0.005, 0.005, 8)
# Brass head sheave block
add_cyl(bm_cbrass, jib_head + Vector((0, -0.032, 0)), jib_head + Vector((0, +0.032, 0)), 0.032, 0.032, 14)

o_cr = bm_to_obj("PART_crane", bm_crane, IRON2)
o_cb = bm_to_obj("_crane_brass", bm_cbrass, BRASS2)
bpy.ops.object.select_all(action="DESELECT")
o_cr.select_set(True)
o_cb.select_set(True)
bpy.context.view_layer.objects.active = o_cr
bpy.ops.object.join()
o_cr.location = crane_piv

# PART_bell: origin at world jib head (1.06, 0.0, 0.485), local offset (0.88, 0.0, 0.433) from PART_crane pivot
bell_piv = crane_piv + Vector((0.88, 0.0, 0.433))
bm_bell = bmesh.new()
bm_bbrass = bmesh.new()
# Hoist cable from sheave (0,0,0) down to bell crown (0,0,-0.22)
add_cyl(bm_bell, Vector((0, 0, 0.0)), Vector((0, 0, -0.22)), 0.006, 0.006, 8)
# Brass shackle + crown dome + flared diving bell skirt + heavy iron ballast rim (bottom at -0.44 -> world z=+0.045, aft of stern x=0.995!)
add_cyl(bm_bbrass, Vector((0, 0, -0.20)), Vector((0, 0, -0.23)), 0.028, 0.040, 12)
add_cyl(bm_bell, Vector((0, 0, -0.23)), Vector((0, 0, -0.32)), 0.068, 0.092, 16)
add_cyl(bm_bell, Vector((0, 0, -0.32)), Vector((0, 0, -0.42)), 0.092, 0.108, 16)
add_cyl(bm_bbrass, Vector((0, 0, -0.42)), Vector((0, 0, -0.445)), 0.114, 0.114, 16)
# 4 brass viewing portholes around diving bell
for k in range(4):
    ang = k * math.pi * 0.5
    px, py = 0.088 * math.cos(ang), 0.088 * math.sin(ang)
    p_in = Vector((0.065 * math.cos(ang), 0.065 * math.sin(ang), -0.30))
    p_out = Vector((px, py, -0.30))
    add_cyl(bm_bbrass, p_in, p_out, 0.022, 0.022, 12)

o_bl = bm_to_obj("PART_bell", bm_bell, RUST)
o_bb = bm_to_obj("_bell_brass", bm_bbrass, BRASS2)
bpy.ops.object.select_all(action="DESELECT")
o_bl.select_set(True)
o_bb.select_set(True)
bpy.context.view_layer.objects.active = o_bl
bpy.ops.object.join()
o_bl.location = bell_piv

tender_report = {
    "model": "b3_salvage_tender",
    "long_axis": "X",
    "bow_sign": -1,
    "parts": [
        {
            "name": "PART_crane",
            "kind": "crane_slew",
            "axis": "Z",
            "pivot": [round(x, 5) for x in crane_piv],
            "slew_deg": 28.0,
        },
        {
            "name": "PART_bell",
            "kind": "pendulum_bell",
            "parent": "PART_crane",
            "pivot_world": [round(x, 5) for x in bell_piv],
            "offset_from_crane": [0.88, 0.0, 0.433],
        },
    ],
}
bpy.ops.export_scene.gltf(filepath=f"{out_dir}/b3_salvage_tender_parts.glb", export_format="GLB")
json.dump(tender_report, open(f"{out_dir}/b3_salvage_tender_parts.json", "w"), indent=1)

# Render tender proof (0 deg vs 26 deg crane slew + 8 deg bell swing)
all_vs = []
for o in (body2, o_cr, o_bl):
    mw = o.matrix_world
    all_vs.extend([mw @ v.co for v in o.data.vertices])
lo2 = Vector([min(v[i] for v in all_vs) for i in range(3)])
hi2 = Vector([max(v[i] for v in all_vs) for i in range(3)])
sz2 = hi2 - lo2; ctr2 = (lo2 + hi2) * 0.5; D2 = sz2.length * 2.1
sc2, cam2 = setup_front_lit_cam(sz2)
for ang in (0, 26):
    rad = math.radians(ang)
    o_cr.rotation_euler = (0.0, 0.0, rad)
    # Move bell pivot with rotated crane jib head + small pendulum swing
    rot_off = Matrix.Rotation(rad, 3, "Z") @ Vector((0.88, 0.0, 0.433))
    o_bl.location = crane_piv + rot_off
    o_bl.rotation_euler = (math.radians(7.0 if ang else 0.0), 0.0, rad)
    shoot(sc2, cam2, ctr2 + Vector((0, -D2, 0.08 * D2)), ctr2, f"{proof_dir}/tender_side_{ang}.png")
    shoot(sc2, cam2, ctr2 + Vector((0.68 * D2, -0.70 * D2, 0.36 * D2)), ctr2, f"{proof_dir}/tender_3q_{ang}.png")
    shoot(sc2, cam2, ctr2 + Vector((0, 0, D2)), ctr2, f"{proof_dir}/tender_top_{ang}.png")
print("TENDER_DONE")
