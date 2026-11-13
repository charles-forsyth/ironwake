# Attach procedural brig rig to a bare-masted hull: topmasts, jibboom, square yards + curved roached sails,
# two curved two-sided headsails (fore staysail + jib), and a fore-and-aft gaff spanker (boom, gaff, trapezoidal sail).
# Usage: blender -b -P rig_brig.py -- in.glb outdir name
import bpy, bmesh, sys, os, json, math
from mathutils import Vector, Matrix

a = sys.argv[sys.argv.index("--") + 1:]
src, outdir, name = a[0], a[1], a[2]
os.makedirs(outdir, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
ms = [o for o in bpy.data.objects if o.type == "MESH"]
for o in ms:
    o.select_set(True)
bpy.context.view_layer.objects.active = ms[0]
if len(ms) > 1:
    bpy.ops.object.join()
b = bpy.context.view_layer.objects.active
b.name = "HullBody"
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

vs = [v.co.copy() for v in b.data.vertices]
lo = Vector([min(v[i] for v in vs) for i in range(3)])
hi = Vector([max(v[i] for v in vs) for i in range(3)])
sz = hi - lo
L = 0 if sz.x >= sz.y else 1
T = 1 - L

# bow = the end with fewer vertices in its outer 6% (thin bowsprit); BOW = -1 means bow at low end of long axis
def _end(sign):
    thr = lo[L] + 0.06 * sz[L] if sign < 0 else hi[L] - 0.06 * sz[L]
    return len([v for v in vs if (v[L] <= thr if sign < 0 else v[L] >= thr)])

_nneg, _npos = _end(-1), _end(+1)
BOW = -1 if _nneg < _npos else +1
AFT = -BOW
print("BOW sign", BOW, "verts at ends", _nneg, _npos)

def mat(nm, c, r=0.72):
    m = bpy.data.materials.new(nm)
    m.use_nodes = True
    p = m.node_tree.nodes["Principled BSDF"]
    p.inputs["Base Color"].default_value = c
    p.inputs["Roughness"].default_value = r
    return m

WOOD = mat("IW_SparWood", (0.24, 0.15, 0.075, 1.0), 0.72)
IRON = mat("IW_MastCap", (0.07, 0.07, 0.075, 1.0), 0.55)
CANVAS = mat("IW_SailCanvas", (0.76, 0.68, 0.53, 1.0), 0.88)
ROPE = mat("IW_StayRope", (0.16, 0.12, 0.08, 1.0), 0.82)

def obj(nm, bm, m):
    me = bpy.data.meshes.new(nm)
    bm.to_mesh(me)
    bm.free()
    me.materials.append(m)
    o = bpy.data.objects.new(nm, me)
    bpy.context.scene.collection.objects.link(o)
    return o

def P(l, t, z):
    v = [0.0, 0.0, 0.0]
    v[L] = l
    v[T] = t
    v[2] = z
    return Vector(v)

def cyl_between(bm, p0, p1, r0, r1=None, segs=12):
    if r1 is None:
        r1 = r0
    d = p1 - p0
    if d.length < 1e-5:
        return
    mid = (p0 + p1) * 0.5
    g = bmesh.ops.create_cone(bm, cap_ends=True, segments=segs, radius1=r0, radius2=r1, depth=d.length)
    rot = Vector((0, 0, 1)).rotation_difference(d.normalized()).to_matrix().to_4x4()
    bmesh.ops.transform(bm, matrix=Matrix.Translation(mid) @ rot, verts=g["verts"])

def box_at(bm, center, sx, sy, sz_):
    g = bmesh.ops.create_cube(bm, size=1.0)
    sc = Matrix.Diagonal((sx, sy, sz_, 1.0))
    bmesh.ops.transform(bm, matrix=Matrix.Translation(center) @ sc, verts=g["verts"])

def build_two_sided_grid(bm, grid, normal_off):
    nv = len(grid) - 1
    nu = len(grid[0]) - 1
    for off_vec, flip in ((Vector((0, 0, 0)), False), (normal_off, True)):
        vgrid = [[bm.verts.new(p + off_vec) for p in row] for row in grid]
        for j in range(nv):
            for i in range(nu):
                f = (vgrid[j][i], vgrid[j][i + 1], vgrid[j + 1][i + 1], vgrid[j + 1][i])
                bm.faces.new(reversed(f) if flip else f)
    # stitch perimeter edges for watertight thin sheet
    return bm

def square_sail_local(bm, w, h, roach=0.055):
    # Local coords: origin at yard centre (0,0,0), lateral along Y, down along -Z, billow forward along BOW * X
    nu, nv = 10, 7
    th = 0.008
    grid = []
    for j in range(nv + 1):
        vv = j / nv
        row = []
        for i in range(nu + 1):
            u = i / nu - 0.5  # -0.5 .. +0.5
            # Slight taper toward clews, arched foot (roach) lifting the middle so it clears stays/deck
            w_ = w * (1.0 - 0.10 * vv)
            foot_lift = roach * h * (1.0 - (2.0 * u) ** 2) * (vv ** 2)
            z = -vv * h + foot_lift
            belly = 0.085 * w * (1.0 - 0.65 * (2.0 * u) ** 2) * math.sin(math.pi * min(1.0, vv * 0.94 + 0.03))
            row.append(Vector((belly * BOW, u * w_, z)))
        grid.append(row)
    return build_two_sided_grid(bm, grid, Vector((-th * BOW, 0.0, 0.0)))

def fore_aft_quad_sail(bm, pivot, p_throat, p_peak, p_clew, p_tack, camber_lat=0.028):
    # Quad in local coordinates relative to `pivot`: luff (tack->throat), head (throat->peak), leech (peak->clew), foot (tack->clew)
    nu, nv = 8, 8
    th = 0.007
    th_loc = p_throat - pivot
    pk_loc = p_peak - pivot
    cl_loc = p_clew - pivot
    tk_loc = p_tack - pivot
    grid = []
    for j in range(nv + 1):
        v = j / nv  # 0 = foot (tack->clew), 1 = head (throat->peak)
        p_left = tk_loc.lerp(th_loc, v)
        p_right = cl_loc.lerp(pk_loc, v)
        row = []
        for i in range(nu + 1):
            u = i / nu  # 0 = luff, 1 = leech
            p = p_left.lerp(p_right, u)
            # Subtle leech roach outward and realistic leeward camber
            leech_roach = 0.022 * math.sin(math.pi * v) * (u ** 2)
            p[L] += AFT * leech_roach
            camber = camber_lat * math.sin(math.pi * u) * (0.35 + 0.65 * math.sin(math.pi * v))
            p[T] += camber
            row.append(p)
        grid.append(row)
    off = [0.0, 0.0, 0.0]
    off[T] = th
    return build_two_sided_grid(bm, grid, Vector(off))

def tri_headsail(bm, pivot, p_tack, p_head, p_clew, camber_lat=0.024):
    # Degenerate-free fan/quad grid from head to foot (tack..clew), relative to pivot
    nu, nv = 7, 8
    th = 0.006
    tk = p_tack - pivot
    hd = p_head - pivot
    cl = p_clew - pivot
    grid = []
    for j in range(nv + 1):
        v = j / nv  # 0 = foot (tack..clew), 1 = near head (narrowing cleanly to 4% width at head cringle)
        span = 1.0 - 0.94 * v
        row = []
        for i in range(nu + 1):
            u = i / nu  # 0 = luff (tack->head), 1 = leech (clew->head)
            base_luff = tk.lerp(hd, v)
            base_leech = cl.lerp(hd, v)
            p = base_luff.lerp(base_leech, u)
            # Slight stay sag + hollow leech + lateral wind belly
            foot_arch = 0.015 * math.sin(math.pi * u) * ((1.0 - v) ** 1.5)
            p[2] += foot_arch
            camber = camber_lat * math.sin(math.pi * u) * math.sin(math.pi * min(1.0, v * 0.9 + 0.08))
            p[T] += camber
            row.append(p)
        grid.append(row)
    off = [0.0, 0.0, 0.0]
    off[T] = th
    return build_two_sided_grid(bm, grid, Vector(off))

# Detect mast X centres from high vertices on the bare-masted hull
z_probe = lo.z + 0.65 * sz.z
high_v = [v for v in vs if v.z > z_probe]
mid_l = (lo[L] + hi[L]) * 0.5
fore_vs = [v for v in high_v if (v[L] < mid_l if BOW < 0 else v[L] > mid_l)]
main_vs = [v for v in high_v if (v[L] >= mid_l if BOW < 0 else v[L] <= mid_l)]
fore_l = sum(v[L] for v in fore_vs) / len(fore_vs) if fore_vs else -0.148 * (-BOW)
main_l = sum(v[L] for v in main_vs) / len(main_vs) if main_vs else 0.339 * (-BOW)
fore_top_hull = max(v.z for v in fore_vs) if fore_vs else hi.z
main_top_hull = max(v.z for v in main_vs) if main_vs else hi.z
print(f"Detected fore_l={fore_l:.4f} (top {fore_top_hull:.4f}), main_l={main_l:.4f} (top {main_top_hull:.4f})")

# 1. Static spar additions (topmasts, topgallant masts, crosstrees/caps, jibboom, key stays) -> joined to HullBody
bm_static = bmesh.new()
bm_caps = bmesh.new()
bm_stays = bmesh.new()

mast_specs = [
    # (mast_idx, lpos, hull_top, topmast_top, tgallant_top, tiers: [(yard_z, yard_w, sail_h)])
    (0, fore_l, fore_top_hull, 0.67, 0.84, [
        (0.25, 0.55, 0.22),   # Fore course: foot at z=+0.03 (generous clearance above forecastle -0.11)
        (0.54, 0.45, 0.26),   # Fore topsail: deep main driving sail, foot at z=+0.28
        (0.75, 0.29, 0.17),   # Fore topgallant: smaller upper sail, foot at z=+0.58
    ]),
    (1, main_l, main_top_hull, 0.71, 0.89, [
        (0.33, 0.56, 0.19),   # Main course: high-cut brig course clearing spanker throat/boom cleanly (foot z=+0.14)
        (0.60, 0.47, 0.25),   # Main topsail: deep topsail, foot at z=+0.35
        (0.81, 0.30, 0.17),   # Main topgallant: smaller upper sail, foot at z=+0.64
    ]),
]

for mi, lpos, htop, tm_top, tg_top, _ in mast_specs:
    step_f = BOW * 0.018
    rake_tm = AFT * 0.014
    rake_tg = AFT * 0.026
    # Lower mast cap & trestletrees around hull masthead
    cap_z = htop - 0.035
    box_at(bm_caps, P(lpos + step_f * 0.5, 0.0, cap_z), 0.055 if L == 0 else 0.095, 0.095 if L == 0 else 0.055, 0.018)
    # Topmast (stepped just forward of lower masthead with subtle aft rake)
    cyl_between(bm_static, P(lpos + step_f, 0.0, cap_z - 0.05), P(lpos + step_f + rake_tm, 0.0, tm_top), 0.015, 0.011, 12)
    # Topmast crosstrees / cap
    box_at(bm_caps, P(lpos + step_f * 1.35 + rake_tm, 0.0, tm_top - 0.025), 0.042 if L == 0 else 0.075, 0.075 if L == 0 else 0.042, 0.014)
    # Topgallant + royal pole
    cyl_between(bm_static, P(lpos + step_f * 1.6 + rake_tm, 0.0, tm_top - 0.04), P(lpos + step_f * 1.6 + rake_tg, 0.0, tg_top), 0.010, 0.0055, 10)

# Jibboom extending forward along bowsprit angle
bow_tip_hull = (lo[L] + 0.02) if BOW < 0 else (hi[L] - 0.02)
jibboom_base = P(bow_tip_hull - BOW * 0.16, 0.0, -0.062)
jibboom_tip = P(bow_tip_hull + BOW * 0.18, 0.0, -0.005)
cyl_between(bm_static, jibboom_base, jibboom_tip, 0.012, 0.007, 10)

# Clean standing stays (forestay, jibstay, mainstay, backstays)
cyl_between(bm_stays, P(bow_tip_hull - BOW * 0.22, 0.0, -0.075), P(fore_l, 0.0, fore_top_hull - 0.04), 0.0035, 0.0035, 6)
cyl_between(bm_stays, jibboom_tip, P(fore_l + BOW * 0.018, 0.0, 0.65), 0.003, 0.003, 6)
cyl_between(bm_stays, P(fore_l, 0.0, 0.02), P(main_l, 0.0, main_top_hull - 0.03), 0.0035, 0.0035, 6)
cyl_between(bm_stays, P(fore_l + BOW * 0.018, 0.0, 0.64), P(main_l + BOW * 0.03, 0.0, 0.86), 0.0028, 0.0028, 6)

o_stat = obj("_static_spars", bm_static, WOOD)
o_caps = obj("_static_caps", bm_caps, IRON)
o_stay = obj("_static_stays", bm_stays, ROPE)
bpy.ops.object.select_all(action='DESELECT')
for o in (b, o_stat, o_caps, o_stay):
    o.select_set(True)
bpy.context.view_layer.objects.active = b
bpy.ops.object.join()

report = {"model": name, "long_axis": "XY"[L], "bow_sign": BOW, "parts": []}

# 2. Moving square yards & square sails on both masts
for mi, lpos, htop, tm_top, tg_top, tiers in mast_specs:
    for yi, (yz, wd, sh) in enumerate(tiers):
        fwd_off = BOW * (0.028 if yi == 0 else (0.042 if yi == 1 else 0.052))
        yl = lpos + fwd_off
        piv = P(yl, 0.0, yz)
        # Tapered yard centred at origin so object location = pivot
        bm_y = bmesh.new()
        cyl_between(bm_y, P(0.0, -wd * 0.5, 0.0), P(0.0, 0.0, 0.0), 0.0055, 0.0105, 10)
        cyl_between(bm_y, P(0.0, 0.0, 0.0), P(0.0, wd * 0.5, 0.0), 0.0105, 0.0055, 10)
        y_obj = obj(f"PART_yard_m{mi}_{yi}", bm_y, WOOD)
        y_obj.location = piv

        bm_s = bmesh.new()
        square_sail_local(bm_s, wd * 0.94, sh, roach=0.08 if yi == 0 else 0.045)
        s_obj = obj(f"PART_sail_m{mi}_{yi}", bm_s, CANVAS)
        if L == 1:
            s_obj.rotation_euler = (0.0, 0.0, math.radians(90.0))
        s_obj.location = piv

        report["parts"].append({"name": y_obj.name, "pivot": [round(x, 5) for x in piv], "kind": "yard", "mast": mi, "tier": yi})
        report["parts"].append({"name": s_obj.name, "pivot": [round(x, 5) for x in piv], "kind": "sail", "mast": mi, "tier": yi})

# 3. Fore-and-aft Gaff Spanker abaft the mainmast (boom + gaff + quadrilateral spanker sail)
# Pivot on mainmast centreline so rotating around Z sheets boom, gaff and spanker together
stern_tip = hi[L] if AFT > 0 else lo[L]
boom_piv = P(main_l, 0.0, 0.085)
boom_jaws = P(main_l + AFT * 0.020, 0.0, 0.085)
boom_end = P(stern_tip + AFT * 0.075, 0.0, 0.165)

gaff_piv = P(main_l, 0.0, 0.345)
gaff_jaws = P(main_l + AFT * 0.020, 0.0, 0.345)
gaff_peak = P(stern_tip - AFT * 0.060, 0.0, 0.585)

bm_boom = bmesh.new()
cyl_between(bm_boom, boom_jaws - boom_piv, boom_end - boom_piv, 0.010, 0.0065, 10)
o_boom = obj("PART_boom_spanker", bm_boom, WOOD)
o_boom.location = boom_piv

bm_gaff = bmesh.new()
cyl_between(bm_gaff, gaff_jaws - gaff_piv, gaff_peak - gaff_piv, 0.009, 0.0055, 10)
o_gaff = obj("PART_gaff_spanker", bm_gaff, WOOD)
o_gaff.location = gaff_piv

bm_spanker = bmesh.new()
p_tack = P(main_l + AFT * 0.028, 0.0, 0.096)
p_throat = P(main_l + AFT * 0.028, 0.0, 0.334)
p_peak = P(stern_tip - AFT * 0.072, 0.0, 0.570)
p_clew = P(stern_tip + AFT * 0.055, 0.0, 0.172)
fore_aft_quad_sail(bm_spanker, boom_piv, p_throat, p_peak, p_clew, p_tack, camber_lat=0.026)
o_spanker = obj("PART_sail_spanker", bm_spanker, CANVAS)
o_spanker.location = boom_piv

report["parts"].append({"name": "PART_boom_spanker", "pivot": [round(x, 5) for x in boom_piv], "kind": "spanker_boom", "mast": 1})
report["parts"].append({"name": "PART_gaff_spanker", "pivot": [round(x, 5) for x in gaff_piv], "kind": "spanker_gaff", "mast": 1})
report["parts"].append({"name": "PART_sail_spanker", "pivot": [round(x, 5) for x in boom_piv], "kind": "spanker_sail", "mast": 1})

# 4. Two curved two-sided headsails: PART_fore_staysail (inner, stem to fore-top) and PART_jib (outer, jibboom to fore-topmast)
stay_tack = P(fore_l + BOW * 0.44, 0.0, -0.075)
stay_head = P(fore_l + BOW * 0.035, 0.0, fore_top_hull - 0.065)
stay_clew = P(fore_l + BOW * 0.14, 0.0, -0.015)
bm_fstay = bmesh.new()
tri_headsail(bm_fstay, stay_tack, stay_tack, stay_head, stay_clew, camber_lat=0.020)
o_fstay = obj("PART_fore_staysail", bm_fstay, CANVAS)
o_fstay.location = stay_tack
report["parts"].append({"name": "PART_fore_staysail", "pivot": [round(x, 5) for x in stay_tack], "kind": "staysail"})

jib_tack = jibboom_tip + P(-BOW * 0.015, 0.0, 0.005)
jib_head = P(fore_l + BOW * 0.022, 0.0, 0.640)
jib_clew = P(fore_l + BOW * 0.48, 0.0, -0.010)
bm_jib = bmesh.new()
tri_headsail(bm_jib, jib_tack, jib_tack, jib_head, jib_clew, camber_lat=0.024)
o_jib = obj("PART_jib", bm_jib, CANVAS)
o_jib.location = jib_tack
report["parts"].append({"name": "PART_jib", "pivot": [round(x, 5) for x in jib_tack], "kind": "jib"})

bpy.ops.export_scene.gltf(filepath=f"{outdir}/{name}_rigged.glb", export_format="GLB")
json.dump(report, open(f"{outdir}/{name}_rigged.json", "w"), indent=1)

# Proof renders: side, front, top, and 3/4 bow & stern perspective views
all_vs = []
for o in bpy.data.objects:
    if o.type == "MESH":
        mw = o.matrix_world
        all_vs.extend([mw @ v.co for v in o.data.vertices])
rlo = Vector([min(v[i] for v in all_vs) for i in range(3)])
rhi = Vector([max(v[i] for v in all_vs) for i in range(3)])
rsz = rhi - rlo
rc = (rlo + rhi) * 0.5

sc = bpy.context.scene
try:
    sc.render.engine = "BLENDER_EEVEE"
except Exception:
    sc.render.engine = "CYCLES"
sc.render.resolution_x = sc.render.resolution_y = 900
w = bpy.data.worlds.new("w")
sc.world = w
w.color = (0.38, 0.43, 0.49)

sun = bpy.data.objects.new("s", bpy.data.lights.new("s", "SUN"))
sun.data.energy = 3.2
sun.rotation_euler = (math.radians(48), math.radians(15), math.radians(35))
sc.collection.objects.link(sun)

fill = bpy.data.objects.new("f", bpy.data.lights.new("f", "SUN"))
fill.data.energy = 1.1
fill.rotation_euler = (math.radians(65), 0, math.radians(-145))
sc.collection.objects.link(fill)

cam = bpy.data.objects.new("c", bpy.data.cameras.new("c"))
sc.collection.objects.link(cam)
sc.camera = cam
cam.data.type = "ORTHO"
cam.data.clip_end = 10000
cam.data.ortho_scale = max(rsz) * 1.12
D = rsz.length * 2.2

def look_at(cam_obj, eye, target):
    cam_obj.location = eye
    direction = target - eye
    cam_obj.rotation_euler = direction.to_track_quat('-Z', 'Y').to_euler()

ortho_views = {
    "side": rc + P(0.0, -D, 0.0),
    "front": rc + P(BOW * D, 0.0, 0.0),
    "top": rc + Vector((0.0, 0.0, D)),
    "3q_bow": rc + P(BOW * D * 0.72, -D * 0.68, D * 0.34),
    "3q_stern": rc + P(AFT * D * 0.72, -D * 0.68, D * 0.34),
}
for vn, eye in ortho_views.items():
    look_at(cam, eye, rc)
    sc.render.filepath = f"{outdir}/{name}_rig_{vn}.png"
    bpy.ops.render.render(write_still=True)

print("RIG_DONE", name, len(report["parts"]), "parts", "rig_bbox", tuple(round(x, 3) for x in rsz))
