# Automated post-generation b4 processor for Ironwake Card P28.
# Converts models/bodies4/*.glb (GPU job 2249) into self-contained Unreal Engine import
# folders (FBX + 2K PNG + materials.json + parts.json) under models/ue_ready/p28/:
#   1) PatrolAirshipV4 (70.0 m hull) + PART_prop_port, PART_prop_stbd, PART_prop_stern
#   2) SeaGullV4 (1.30 m wingspan) split cleanly at |y| > 0.08 * span into body + PART_wing_L, PART_wing_R
#   3) SailorCrewStand (1.78 m standing helmsman/dockhand) + SailorCrewRower (seated gig oarsman pose)
#   4) TownTavernV4 (11.0 m), TownChandleryV4 (11.5 m), TownFoundryV4 (15.0 m) with base at z = 0
#
# Usage:
#   env -u PYTHONPATH /opt/homebrew/bin/blender -b --factory-startup -P cluster/prep_b4_assets.py
#   env -u PYTHONPATH /opt/homebrew/bin/blender -b --factory-startup -P cluster/prep_b4_assets.py -- <bodies4_dir> <out_root>
#
# Optional AssetFurnace SkinTokens skeletal auto-rigging + Kimodo retargeting for b4_sailor_crew.glb:
#   # 0) Ensure SkinTokens venv + weights are installed once (~1.6 GB HF weights, MIT license):
#   env -u PYTHONPATH ${SKINTOKENS_ROOT:-vendor/assetfurnace}/.venv/bin/python \
#     ${SKINTOKENS_ROOT:-vendor/assetfurnace}/scripts/bootstrap_autorig.py --yes
#   # 1) Fit humanoid skeleton + skin weights (~65s on Apple Silicon MPS or CUDA GPU):
#   (cd ${SKINTOKENS_ROOT:-vendor/assetfurnace}/vendor/SkinTokens && \
#     env -u PYTHONPATH SKINTOKENS_PORT=59875 SKINTOKENS_BPY_PORT=59876 \
#     ${SKINTOKENS_ROOT:-vendor/assetfurnace}/vendor/SkinTokens/.venv/bin/python demo.py \
#     --input ${IRONWAKE_ASSETS:-.}/models/bodies4/b4_sailor_crew.glb \
#     --output ${IRONWAKE_ASSETS:-.}/models/ue_ready/p28/sailor_crew_rigged.glb \
#     --use_transfer)
#   # 2) Prune cross-limb skin weight outliers:
#   env -u PYTHONPATH ${SKINTOKENS_ROOT:-vendor/assetfurnace}/vendor/SkinTokens/.venv/bin/python \
#     ${SKINTOKENS_ROOT:-vendor/assetfurnace}/scripts/clean_skin_weights.py \
#     --in ${IRONWAKE_ASSETS:-.}/models/ue_ready/p28/sailor_crew_rigged.glb \
#     --out ${IRONWAKE_ASSETS:-.}/models/ue_ready/p28/sailor_crew_rigged.glb
#   # 3) Retarget any preset clip (idle, walk, sit, wave, etc. in image_to_3dlab/motions/*.npz):
#   env -u PYTHONPATH ${SKINTOKENS_ROOT:-vendor/assetfurnace}/vendor/SkinTokens/.venv/bin/python \
#     ${SKINTOKENS_ROOT:-vendor/assetfurnace}/scripts/kimodo_retarget.py \
#     --rig ${IRONWAKE_ASSETS:-.}/models/ue_ready/p28/sailor_crew_rigged.glb \
#     --motion ${SKINTOKENS_ROOT:-vendor/assetfurnace}/image_to_3dlab/motions/idle.npz \
#     --out ${IRONWAKE_ASSETS:-.}/models/ue_ready/p28/sailor_crew_idle.glb \
#     --arm-spread 12 --speed 1

import json
import math
import os
import sys

import bmesh  # type: ignore
import bpy  # type: ignore
import numpy as np
from mathutils import Euler, Matrix, Vector  # type: ignore

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DEFAULT_IN = os.path.join(REPO_ROOT, "models", "bodies4")
DEFAULT_OUT = os.path.join(REPO_ROOT, "models", "ue_ready", "p28")

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
IN_DIR = os.path.abspath(argv[0]) if len(argv) >= 1 else DEFAULT_IN
OUT_ROOT = os.path.abspath(argv[1]) if len(argv) >= 2 else DEFAULT_OUT
os.makedirs(OUT_ROOT, exist_ok=True)

SKINTOKENS_ROOT = os.environ.get("SKINTOKENS_ROOT", "vendor/assetfurnace")
ASSETS_ROOT = os.environ.get("IRONWAKE_ASSETS", REPO_ROOT)
SKINTOKENS_CMD = (
    f"cd {SKINTOKENS_ROOT}/vendor/SkinTokens && "
    f"env -u PYTHONPATH SKINTOKENS_PORT=59875 SKINTOKENS_BPY_PORT=59876 "
    f"{SKINTOKENS_ROOT}/vendor/SkinTokens/.venv/bin/python demo.py "
    f"--input {ASSETS_ROOT}/models/bodies4/b4_sailor_crew.glb "
    f"--output {ASSETS_ROOT}/models/ue_ready/p28/sailor_crew_rigged.glb "
    f"--use_transfer"
)


# ------------------------------------------------------------------------------
# Shared UE-ready helpers (matching pipeline/prep_b4_assets.py + iw_p21_import.py)
# ------------------------------------------------------------------------------
def remap_col(c):
    r, g, b = c[:3]
    if r > 0.90 and g > 0.90 and b > 0.90:
        return [0.62, 0.60, 0.54]
    return [round(float(r), 4), round(float(g), 4), round(float(b), 4)]


def get_verts(obj):
    return np.array([tuple(v.co) for v in obj.data.vertices], dtype=np.float64)


def make_pbr_mat(name, rgba, metal=0.0, rough=0.65):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = rgba
    bsdf.inputs["Metallic"].default_value = metal
    bsdf.inputs["Roughness"].default_value = rough
    return m


def bm_to_obj(name, bm, mat):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    me.materials.append(mat)
    o = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(o)
    return o


def add_cyl(bm, p0, p1, r0, r1=None, segs=16):
    if r1 is None:
        r1 = r0
    d = p1 - p0
    if d.length < 1e-6:
        return
    mid = (p0 + p1) * 0.5
    g = bmesh.ops.create_cone(bm, cap_ends=True, segments=segs, radius1=r0, radius2=r1, depth=d.length)
    rot = Vector((0.0, 0.0, 1.0)).rotation_difference(d.normalized()).to_matrix().to_4x4()
    bmesh.ops.transform(bm, matrix=Matrix.Translation(mid) @ rot, verts=g["verts"])


def add_box(bm, center, sx, sy, sz, rot_mat=None):
    g = bmesh.ops.create_cube(bm, size=1.0)
    m = Matrix.Diagonal((sx, sy, sz, 1.0))
    if rot_mat is not None:
        m = rot_mat.to_4x4() @ m
    bmesh.ops.transform(bm, matrix=Matrix.Translation(center) @ m, verts=g["verts"])


def load_clean_glb(glb_path, dielectric_default_rough=0.78):
    if not os.path.isfile(glb_path):
        raise FileNotFoundError(f"Missing required GLB: {glb_path}")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=glb_path)
    meshes = []
    for o in list(bpy.context.scene.objects):
        if o.type == "MESH" and len(o.data.polygons) > 0:
            mw = o.matrix_world.copy()
            o.parent = None
            if o.data.users > 1:
                o.data = o.data.copy()
            o.data.transform(mw)
            o.matrix_world = Matrix.Identity(4)
            meshes.append(o)
    for o in list(bpy.context.scene.objects):
        if o not in meshes:
            bpy.data.objects.remove(o)
    bpy.ops.object.select_all(action="DESELECT")
    for o in meshes:
        o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    if len(meshes) > 1:
        bpy.ops.object.join()
    body = bpy.context.active_object

    # Fix glTF default metallicFactor=1.0 when Hunyuan3D exports albedo-only materials without a metallic texture
    for m in body.data.materials:
        if not m or not m.use_nodes:
            continue
        bsdf = next((n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED"), None)
        if not bsdf:
            continue
        has_metal_tex = any(l.to_node == bsdf and l.to_socket.name == "Metallic" for l in m.node_tree.links)
        if not has_metal_tex and bsdf.inputs["Metallic"].default_value > 0.95:
            bsdf.inputs["Metallic"].default_value = 0.0
            bsdf.inputs["Roughness"].default_value = dielectric_default_rough
    return body


def save_used_textures(objs, tex_dir, prefix):
    used_imgs = set()
    used_mats = {m for o in objs for m in o.data.materials if m and m.use_nodes}
    for m in used_mats:
        for n in m.node_tree.nodes:
            if n.type == "TEX_IMAGE" and n.image and n.image.size[0] > 0:
                used_imgs.add(n.image)
    if not used_imgs:
        return {}
    os.makedirs(tex_dir, exist_ok=True)
    saved = {}
    for img in sorted(used_imgs, key=lambda x: x.name):
        if max(img.size) > 2048:
            f = 2048.0 / max(img.size)
            img.scale(int(img.size[0] * f), int(img.size[1] * f))
        clean = img.name.replace(".", "_").replace(" ", "_").replace("#", "")
        fn_rel = f"tex/T_{prefix}_{clean}.png"
        fn_abs = os.path.join(os.path.dirname(tex_dir), fn_rel)
        img.filepath_raw = fn_abs
        img.file_format = "PNG"
        try:
            img.save()
        except Exception:
            img.save_render(fn_abs)
        saved[img.name] = fn_rel
    return saved


def extract_matjson(objs, saved_tex):
    out = {}
    used = {m.name for o in objs for m in o.data.materials if m}
    for m in bpy.data.materials:
        if m.name not in used or not m.use_nodes:
            continue
        bsdf = next((n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED"), None)
        if not bsdf:
            continue
        e = {
            "color": remap_col(list(bsdf.inputs["Base Color"].default_value)),
            "rough": round(max(0.32, float(bsdf.inputs["Roughness"].default_value)), 3),
            "metal": round(float(bsdf.inputs["Metallic"].default_value), 3),
        }
        for l in m.node_tree.links:
            if l.to_node != bsdf:
                continue
            stack = [l.from_node]
            found_img = None
            while stack:
                x = stack.pop()
                if x.type == "TEX_IMAGE" and x.image:
                    found_img = x.image.name
                    break
                for inp in x.inputs:
                    for ll in inp.links:
                        stack.append(ll.from_node)
            if found_img and found_img in saved_tex:
                key = {
                    "Base Color": "base",
                    "Metallic": "mr",
                    "Roughness": "mr",
                    "Normal": "normal",
                }.get(l.to_socket.name)
                if key:
                    e[key] = saved_tex[found_img]
        if "base" in e:
            e["color"] = [1.0, 1.0, 1.0]
        out[m.name] = e
    return out


def export_fbx(obj, filepath):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(
        filepath=filepath,
        use_selection=True,
        apply_scale_options="FBX_SCALE_ALL",
        axis_forward="-Z",
        axis_up="Y",
        object_types={"MESH"},
        mesh_smooth_type="FACE",
        path_mode="RELATIVE",
        embed_textures=False,
        bake_space_transform=False,
    )


def export_asset_bundle(base_name, body, scale_val, parts_list=None, extra_meta=None):
    """Export <BaseName>/ and optional <BaseName>_<part>/ folders under OUT_ROOT."""
    parts_list = parts_list or []
    body.name = base_name
    body_dir = os.path.join(OUT_ROOT, base_name)
    os.makedirs(body_dir, exist_ok=True)
    saved_tex = save_used_textures([body], os.path.join(body_dir, "tex"), base_name)
    with open(os.path.join(body_dir, "materials.json"), "w") as f:
        json.dump(extract_matjson([body], saved_tex), f, indent=1)
    export_fbx(body, os.path.join(body_dir, f"{base_name}.fbx"))

    parts_out = {}
    for p in parts_list:
        o = p["obj"]
        part_key = p["name"] if p["name"].startswith("PART_") else f"PART_{p['name']}"
        piv = Vector(p["pivot_m"])
        # Shift mesh vertices so local origin (0,0,0) sits right on the rotation axle / hinge pivot
        o.data.transform(Matrix.Translation(-piv))
        o.location = Vector((0.0, 0.0, 0.0))
        sub_name = f"{base_name}_{part_key.replace('PART_', '')}"
        o.name = sub_name
        p_dir = os.path.join(OUT_ROOT, sub_name)
        os.makedirs(p_dir, exist_ok=True)
        p_tex = save_used_textures([o], os.path.join(p_dir, "tex"), sub_name)
        with open(os.path.join(p_dir, "materials.json"), "w") as f:
            json.dump(extract_matjson([o], p_tex), f, indent=1)
        export_fbx(o, os.path.join(p_dir, f"{sub_name}.fbx"))
        vp = get_verts(o)
        parts_out[part_key] = {
            "pivot_m": [round(float(x), 4) for x in piv],
            "kind": p.get("kind", "part"),
            "axis": p.get("axis", "X"),
            "mast": p.get("mast", -1),
            "tier": p.get("tier", -1),
            "parent": p.get("parent", ""),
            "asset": sub_name,
            "local_min_m": [round(float(x), 3) for x in vp.min(0)],
            "local_max_m": [round(float(x), 3) for x in vp.max(0)],
        }
        if "flap_sign" in p:
            parts_out[part_key]["flap_sign"] = p["flap_sign"]

    vb = get_verts(body)
    summary = {
        "base_name": base_name,
        "scale": round(float(scale_val), 4),
        "hull_min_m": [round(float(x), 3) for x in vb.min(0)],
        "hull_max_m": [round(float(x), 3) for x in vb.max(0)],
        "parts": parts_out,
    }
    if extra_meta:
        summary.update(extra_meta)
    with open(os.path.join(body_dir, "parts.json"), "w") as f:
        json.dump(summary, f, indent=1)
    print(f"P28_READY {base_name:18s} parts={len(parts_out)} min={summary['hull_min_m']} max={summary['hull_max_m']}")
    return summary


# ------------------------------------------------------------------------------
# 1. PatrolAirshipV4: 70 m hull + 3 procedural 4-blade brass/wood propellers
# ------------------------------------------------------------------------------
def build_propeller_mesh(name, radius_m, forward_spinner_sign=1.0, handedness=1.0, mat_wood=None, mat_brass=None):
    """Build a 4-blade Victorian laminated mahogany + polished brass airship propeller in world meters.
    Built around local origin (0,0,0) with spin shaft along X; export_asset_bundle translates to world pivot_m
    and then recentres local origin at (0,0,0)."""
    bm_wood = bmesh.new()
    bm_brass = bmesh.new()
    r = radius_m
    s_fwd = forward_spinner_sign  # +1 if nose spinner points +X (tractor), -1 if tail cone points -X (pusher)

    # Central brass hub barrel + aerodynamic nose/tail spinner cone along X
    add_cyl(bm_brass, Vector((-0.14 * r * s_fwd, 0.0, 0.0)), Vector((0.08 * r * s_fwd, 0.0, 0.0)), 0.135 * r, 0.135 * r, 18)
    add_cyl(bm_brass, Vector((0.08 * r * s_fwd, 0.0, 0.0)), Vector((0.28 * r * s_fwd, 0.0, 0.0)), 0.135 * r, 0.024 * r, 18)
    # Hub flange bolt ring
    add_cyl(bm_brass, Vector((-0.03 * r * s_fwd, 0.0, 0.0)), Vector((0.03 * r * s_fwd, 0.0, 0.0)), 0.162 * r, 0.162 * r, 18)

    for k in range(4):
        ang = k * (math.pi * 0.5)
        R_roll = Matrix.Rotation(ang, 3, "X")
        # Brass root shank & collar ferrule extending radially in Y-Z plane (base blade along +Y before R_roll)
        add_cyl(bm_brass, R_roll @ Vector((0.0, 0.11 * r, 0.0)), R_roll @ Vector((0.0, 0.25 * r, 0.0)), 0.052 * r, 0.044 * r, 12)
        add_cyl(bm_brass, R_roll @ Vector((0.0, 0.23 * r, 0.0)), R_roll @ Vector((0.0, 0.27 * r, 0.0)), 0.058 * r, 0.055 * r, 12)

        # Sculpted twisted laminated mahogany blade (6 radial stations from 0.24r to 0.98r)
        n_seg = 6
        for seg in range(n_seg):
            t = (seg + 0.5) / n_seg
            rad_y = r * (0.24 + 0.74 * t)
            seg_len = (0.74 * r / n_seg) * 1.10
            # Elliptical paddle chord tapering at root and tip
            chord = r * (0.115 + 0.095 * math.sin(math.pi * min(1.0, t * 0.92 + 0.08)))
            thick = r * (0.030 * (1.0 - 0.55 * t) + 0.008)
            # Aerodynamic washout twist: ~42 deg near root -> ~18 deg near tip
            pitch_rad = math.radians((42.0 - 24.0 * t) * handedness)
            R_seg = R_roll @ Matrix.Rotation(pitch_rad, 3, "Y")
            ctr = R_roll @ Vector((0.0, rad_y, 0.0))
            add_box(bm_wood, ctr, thick, seg_len, chord, rot_mat=R_seg)

            # Polished brass leading-edge erosion sheath on outer 55% of blade
            if t >= 0.42:
                le_off = R_seg @ Vector((0.0, 0.0, 0.46 * chord * handedness))
                add_box(bm_brass, ctr + le_off, thick * 1.08, seg_len * 1.01, chord * 0.16, rot_mat=R_seg)

    o_wood = bm_to_obj(name, bm_wood, mat_wood)
    o_brass = bm_to_obj(name + "_brass", bm_brass, mat_brass)
    bpy.ops.object.select_all(action="DESELECT")
    o_wood.select_set(True)
    o_brass.select_set(True)
    bpy.context.view_layer.objects.active = o_wood
    bpy.ops.object.join()
    return o_wood


def prep_patrol_airship():
    body = load_clean_glb(os.path.join(IN_DIR, "b4_patrol_airship_body.glb"), dielectric_default_rough=0.68)
    # Raw GLB has bow at -Y, stern at +Y, port at +X, stbd at -X -> +90 deg Z puts bow at +X, port at +Y, stbd at -Y
    body.data.transform(Matrix.Rotation(math.radians(90.0), 4, "Z"))
    vb = get_verts(body)
    mn, mx = vb.min(0), vb.max(0)
    span_x = mx[0] - mn[0]
    scale = 70.0 / span_x
    ctr_xy = 0.5 * (mn[:2] + mx[:2])
    G = Matrix.Scale(scale, 4) @ Matrix.Translation((-ctr_xy[0], -ctr_xy[1], -mn[2]))
    body.data.transform(G)

    # Measure scaled 70m airship geometry directly so nacelle & stern hubs are exact
    va = get_verts(body)
    amn, amx = va.min(0), va.max(0)
    span_y = amx[1] - amn[1]
    span_z = amx[2] - amn[2]

    # Locate port (+Y) and starboard (-Y) outboard nacelle pods (|y| > 0.30 * span_y, lower 45% of Z)
    def measure_pod(sign_y):
        mask = (va[:, 1] * sign_y > 0.30 * span_y) & (va[:, 2] < amn[2] + 0.45 * span_z)
        pod = va[mask]
        pmin, pmax = pod.min(0), pod.max(0)
        # Front (+X) nose cap tip of nacelle pod
        nose = pod[pod[:, 0] > pmax[0] - 0.06 * (pmax[0] - pmin[0])]
        cy = float(np.median(nose[:, 1]))
        cz = float(np.median(nose[:, 2]))
        pod_r = 0.5 * float(max(pmax[1] - pmin[1], pmax[2] - pmin[2]))
        return float(pmax[0]), cy, cz, pod_r

    px_port, py_port, pz_port, pr_port = measure_pod(+1.0)
    px_stbd, py_stbd, pz_stbd, pr_stbd = measure_pod(-1.0)

    # Measure stern cone apex along envelope centreline (junction of horizontal tail fins near amn[0])
    horiz_fins = va[(va[:, 0] < amn[0] + 0.08 * 70.0) & (np.abs(va[:, 1]) > 0.06 * span_y)]
    stern_z = float(np.median(horiz_fins[:, 2]))
    stern_cone = va[(va[:, 0] < amn[0] + 0.04 * 70.0) & (np.abs(va[:, 1]) < 0.04 * span_y) & (np.abs(va[:, 2] - stern_z) < 0.05 * span_z)]
    stern_cone_min_x = float(stern_cone[:, 0].min()) if len(stern_cone) else float(amn[0] + 1.0)
    tail_fin_min_x = float(amn[0])  # trailing edge of vertical dorsal/ventral fins (-35.0 m)

    mat_wood = make_pbr_mat("IW_PropWood", (0.26, 0.14, 0.07, 1.0), metal=0.0, rough=0.54)
    mat_brass = make_pbr_mat("IW_PropBrass", (0.72, 0.54, 0.22, 1.0), metal=0.88, rough=0.32)
    mat_iron = make_pbr_mat("IW_AirshipIron", (0.08, 0.085, 0.09, 1.0), metal=0.78, rough=0.48)

    # Join static riveted stern tail-shaft housing boss extending cleanly aft of tail_fin_min_x
    # plus short forward nacelle shaft collars so all 3 propellers seat mechanically with zero air gap
    stern_hub_x = tail_fin_min_x - 0.95
    nacelle_r_prop = 4.65  # 9.3 m diameter 4-blade nacelle screws (clears gondola by >3.6 m & envelope above)
    stern_r_prop = 3.85    # 7.7 m diameter 4-blade stern pusher screw

    bm_boss = bmesh.new()
    # Stern tapered shaft housing from inside stern cone out to stern_hub_x
    add_cyl(bm_boss, Vector((stern_cone_min_x + 0.80, 0.0, stern_z)), Vector((stern_hub_x + 0.35, 0.0, stern_z)), 0.82, 0.52, 18)
    # Nacelle nose shaft bosses
    piv_port = Vector((px_port + 0.55, py_port, pz_port))
    piv_stbd = Vector((px_stbd + 0.55, py_stbd, pz_stbd))
    add_cyl(bm_boss, Vector((px_port - 0.45, py_port, pz_port)), Vector((piv_port.x - 0.15, py_port, pz_port)), pr_port * 0.32, 0.56, 16)
    add_cyl(bm_boss, Vector((px_stbd - 0.45, py_stbd, pz_stbd)), Vector((piv_stbd.x - 0.15, py_stbd, pz_stbd)), pr_stbd * 0.32, 0.56, 16)
    o_boss = bm_to_obj("_airship_shaft_bosses", bm_boss, mat_iron)
    bpy.ops.object.select_all(action="DESELECT")
    body.select_set(True)
    o_boss.select_set(True)
    bpy.context.view_layer.objects.active = body
    bpy.ops.object.join()

    piv_stern = Vector((stern_hub_x, 0.0, stern_z))

    p_port = build_propeller_mesh("PART_prop_port", nacelle_r_prop, forward_spinner_sign=1.0, handedness=1.0, mat_wood=mat_wood, mat_brass=mat_brass)
    p_port.data.transform(Matrix.Translation(piv_port))

    p_stbd = build_propeller_mesh("PART_prop_stbd", nacelle_r_prop, forward_spinner_sign=1.0, handedness=-1.0, mat_wood=mat_wood, mat_brass=mat_brass)
    p_stbd.data.transform(Matrix.Translation(piv_stbd))

    p_stern = build_propeller_mesh("PART_prop_stern", stern_r_prop, forward_spinner_sign=-1.0, handedness=1.0, mat_wood=mat_wood, mat_brass=mat_brass)
    p_stern.data.transform(Matrix.Translation(piv_stern))

    parts = [
        {"name": "PART_prop_port", "obj": p_port, "pivot_m": piv_port, "axis": "X", "kind": "prop"},
        {"name": "PART_prop_stbd", "obj": p_stbd, "pivot_m": piv_stbd, "axis": "X", "kind": "prop"},
        {"name": "PART_prop_stern", "obj": p_stern, "pivot_m": piv_stern, "axis": "X", "kind": "prop"},
    ]
    return export_asset_bundle("PatrolAirshipV4", body, scale, parts_list=parts)


# ------------------------------------------------------------------------------
# 2. SeaGullV4: 1.3 m wingspan split cleanly at |y| > 0.08 * span
# ------------------------------------------------------------------------------
def cap_and_add_shoulder_plug(wing_obj, shoulder_piv, inboard_sign_y, feather_mat):
    """Fill planar cut boundary on wing & add a small smooth white feather socket dome
    centered on shoulder_piv so +/-35 deg wing flapping never exposes a gap."""
    bm = bmesh.new()
    bm.from_mesh(wing_obj.data)
    bndr = [e for e in bm.edges if e.is_boundary]
    if bndr:
        try:
            bmesh.ops.holes_fill(bm, edges=bndr, sides=0)
        except Exception:
            pass
    bm.to_mesh(wing_obj.data)
    bm.free()

    # Measure shoulder cut chord (x) and thickness (z) to size a snug rounded feather plug
    vw = get_verts(wing_obj)
    near = vw[np.abs(vw[:, 1] - shoulder_piv.y) < 0.015]
    if len(near) >= 4:
        rx = 0.42 * float(near[:, 0].max() - near[:, 0].min())
        rz = 0.44 * float(near[:, 2].max() - near[:, 2].min())
        ry = 0.018  # 1.8 cm inboard feather overlap inside shoulder socket
        bm_plug = bmesh.new()
        bmesh.ops.create_uvsphere(bm_plug, u_segments=14, v_segments=10, radius=1.0)
        bmesh.ops.transform(
            bm_plug,
            matrix=Matrix.Translation(shoulder_piv + Vector((0.0, inboard_sign_y * 0.004, 0.0)))
            @ Matrix.Diagonal((max(0.012, rx), ry, max(0.008, rz), 1.0)),
            verts=bm_plug.verts,
        )
        o_plug = bm_to_obj("_wing_plug", bm_plug, feather_mat)
        bpy.ops.object.select_all(action="DESELECT")
        wing_obj.select_set(True)
        o_plug.select_set(True)
        bpy.context.view_layer.objects.active = wing_obj
        bpy.ops.object.join()


def prep_sea_gull():
    gull = load_clean_glb(os.path.join(IN_DIR, "b4_sea_gull.glb"), dielectric_default_rough=0.76)
    # +90 deg Z puts beak at +X, tail at -X, left wing at +Y, right wing at -Y
    gull.data.transform(Matrix.Rotation(math.radians(90.0), 4, "Z"))
    vg = get_verts(gull)
    mn, mx = vg.min(0), vg.max(0)
    raw_span = mx[1] - mn[1]
    scale = 1.30 / raw_span
    ctr_xy = 0.5 * (mn[:2] + mx[:2])
    G = Matrix.Scale(scale, 4) @ Matrix.Translation((-ctr_xy[0], -ctr_xy[1], -mn[2]))
    gull.data.transform(G)

    vg_scaled = get_verts(gull)
    smn, smx = vg_scaled.min(0), vg_scaled.max(0)
    span_m = float(smx[1] - smn[1])  # 1.300 m
    cut_y = 0.08 * span_m             # 0.104 m exact

    # Planar bisect at y = +cut_y and y = -cut_y so UVs interpolate seamlessly along the cut
    bm = bmesh.new()
    bm.from_mesh(gull.data)
    for y_plane in (+cut_y, -cut_y):
        geom_all = list(bm.verts) + list(bm.edges) + list(bm.faces)
        bmesh.ops.bisect_plane(
            bm,
            geom=geom_all,
            dist=1e-6,
            plane_co=Vector((0.0, y_plane, 0.0)),
            plane_no=Vector((0.0, 1.0, 0.0)),
            clear_inner=False,
            clear_outer=False,
        )
    bm.to_mesh(gull.data)
    bm.free()

    # Split into 3 separate mesh objects by face median Y relative to +/- cut_y
    def extract_region(src_obj, new_name, keep_fn):
        cp = src_obj.copy()
        cp.data = src_obj.data.copy()
        cp.name = new_name
        bpy.context.scene.collection.objects.link(cp)
        bmx = bmesh.new()
        bmx.from_mesh(cp.data)
        del_faces = [f for f in bmx.faces if not keep_fn(f.calc_center_median().y)]
        if del_faces:
            bmesh.ops.delete(bmx, geom=del_faces, context="FACES")
        # Remove isolated vertices left after face deletion
        iso_verts = [v for v in bmx.verts if not v.link_faces]
        if iso_verts:
            bmesh.ops.delete(bmx, geom=iso_verts, context="VERTS")
        # Cap boundary loops on body
        bndr = [e for e in bmx.edges if e.is_boundary]
        if bndr and new_name == "SeaGullV4":
            try:
                bmesh.ops.holes_fill(bmx, edges=bndr, sides=0)
            except Exception:
                pass
        bmx.to_mesh(cp.data)
        bmx.free()
        return cp

    eps = 1e-5
    body_obj = extract_region(gull, "SeaGullV4", lambda y: abs(y) <= cut_y + eps)
    wing_l = extract_region(gull, "PART_wing_L", lambda y: y > cut_y + eps)
    wing_r = extract_region(gull, "PART_wing_R", lambda y: y < -(cut_y + eps))
    bpy.data.objects.remove(gull)

    # Compute exact shoulder hinge pivots from vertices lying on the +/- cut_y plane
    def shoulder_pivot(wing_obj, y_target):
        vw = get_verts(wing_obj)
        sl = vw[np.abs(vw[:, 1] - y_target) < 0.008]
        if len(sl) == 0:
            sl = vw[np.argsort(np.abs(vw[:, 1] - y_target))[:24]]
        return Vector((float(sl[:, 0].mean()), float(y_target), float(sl[:, 2].mean())))

    piv_l = shoulder_pivot(wing_l, +cut_y)
    piv_r = shoulder_pivot(wing_r, -cut_y)

    feather_mat = make_pbr_mat("IW_GullFeather", (0.86, 0.87, 0.88, 1.0), metal=0.0, rough=0.78)
    cap_and_add_shoulder_plug(wing_l, piv_l, inboard_sign_y=-1.0, feather_mat=feather_mat)
    cap_and_add_shoulder_plug(wing_r, piv_r, inboard_sign_y=+1.0, feather_mat=feather_mat)

    parts = [
        {"name": "PART_wing_L", "obj": wing_l, "pivot_m": piv_l, "axis": "X", "kind": "wing", "flap_sign": 1},
        {"name": "PART_wing_R", "obj": wing_r, "pivot_m": piv_r, "axis": "X", "kind": "wing", "flap_sign": -1},
    ]
    return export_asset_bundle(
        "SeaGullV4",
        body_obj,
        scale,
        parts_list=parts,
        extra_meta={"wingspan_m": round(span_m, 3), "wing_cut_y_m": round(cut_y, 4)},
    )


# ------------------------------------------------------------------------------
# 3. SailorCrewStand (1.78 m) & SailorCrewRower (seated gig rower pose)
# ------------------------------------------------------------------------------
def prep_sailor_stand():
    sailor = load_clean_glb(os.path.join(IN_DIR, "b4_sailor_crew.glb"), dielectric_default_rough=0.82)
    # +90 deg Z puts face/toes at +X, left arm at +Y, right arm at -Y
    sailor.data.transform(Matrix.Rotation(math.radians(90.0), 4, "Z"))
    vs = get_verts(sailor)
    mn, mx = vs.min(0), vs.max(0)
    scale = 1.78 / (mx[2] - mn[2])
    ctr_xy = 0.5 * (mn[:2] + mx[:2])
    G = Matrix.Scale(scale, 4) @ Matrix.Translation((-ctr_xy[0], -ctr_xy[1], -mn[2]))
    sailor.data.transform(G)
    return export_asset_bundle(
        "SailorCrewStand",
        sailor,
        scale,
        extra_meta={"height_m": 1.78, "skintokens_rig_cmd": SKINTOKENS_CMD},
    )


def prep_sailor_rower():
    sailor = load_clean_glb(os.path.join(IN_DIR, "b4_sailor_crew.glb"), dielectric_default_rough=0.82)
    sailor.data.transform(Matrix.Rotation(math.radians(90.0), 4, "Z"))
    vs = get_verts(sailor)
    mn, mx = vs.min(0), vs.max(0)
    scale = 1.78 / (mx[2] - mn[2])
    ctr_xy = 0.5 * (mn[:2] + mx[:2])
    G = Matrix.Scale(scale, 4) @ Matrix.Translation((-ctr_xy[0], -ctr_xy[1], -mn[2]))
    sailor.data.transform(G)

    # Weld duplicate seam vertices (preserves BMLoop UVs) so Bone Heat weights 100% of the mesh cleanly
    bm = bmesh.new()
    bm.from_mesh(sailor.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
    bm.to_mesh(sailor.data)
    bm.free()

    arm_data = bpy.data.armatures.new("SailorRowerRig")
    arm_obj = bpy.data.objects.new("SailorRowerRig", arm_data)
    bpy.context.scene.collection.objects.link(arm_obj)
    bpy.context.view_layer.objects.active = arm_obj
    bpy.ops.object.mode_set(mode="EDIT")
    eb = arm_data.edit_bones

    def add_bone(nm, head, tail, parent=None, connect=False):
        b = eb.new(nm)
        b.head = Vector(head)
        b.tail = Vector(tail)
        if parent:
            b.parent = parent
            b.use_connect = connect
        return b

    b_pelvis = add_bone("Pelvis", (-0.01, 0.0, 0.86), (-0.01, 0.0, 1.00))
    b_spine = add_bone("Spine", (-0.01, 0.0, 1.00), (-0.01, 0.0, 1.24), b_pelvis, True)
    b_chest = add_bone("Chest", (-0.01, 0.0, 1.24), (-0.01, 0.0, 1.47), b_spine, True)
    add_bone("Head", (-0.01, 0.0, 1.47), (0.01, 0.0, 1.76), b_chest, True)

    for side, s in (("L", 1.0), ("R", -1.0)):
        b_clav = add_bone(f"Clav_{side}", (-0.01, s * 0.05, 1.43), (-0.02, s * 0.19, 1.42), b_chest, False)
        b_uarm = add_bone(f"UArm_{side}", (-0.02, s * 0.19, 1.42), (-0.01, s * 0.35, 1.17), b_clav, True)
        add_bone(f"FArm_{side}", (-0.01, s * 0.35, 1.17), (0.07, s * 0.50, 0.90), b_uarm, True)

        b_thigh = add_bone(f"Thigh_{side}", (-0.01, s * 0.115, 0.86), (-0.03, s * 0.140, 0.47), b_pelvis, False)
        b_shin = add_bone(f"Shin_{side}", (-0.03, s * 0.140, 0.47), (-0.04, s * 0.148, 0.12), b_thigh, True)
        add_bone(f"Foot_{side}", (-0.04, s * 0.148, 0.12), (0.14, s * 0.155, 0.03), b_shin, True)

    bpy.ops.object.mode_set(mode="OBJECT")
    bpy.ops.object.select_all(action="DESELECT")
    sailor.select_set(True)
    arm_obj.select_set(True)
    bpy.context.view_layer.objects.active = arm_obj
    bpy.ops.object.parent_set(type="ARMATURE_AUTO")

    bpy.ops.object.select_all(action="DESELECT")
    sailor.select_set(True)
    bpy.context.view_layer.objects.active = sailor
    bpy.ops.object.mode_set(mode="WEIGHT_PAINT")
    bpy.ops.object.vertex_group_smooth(group_select_mode="ALL", factor=0.5, repeat=4)
    bpy.ops.object.mode_set(mode="OBJECT")

    bpy.context.view_layer.objects.active = arm_obj
    bpy.ops.object.mode_set(mode="POSE")
    pb = arm_obj.pose.bones

    def rot_world(bone_name, euler_xyz_deg):
        p = pb[bone_name]
        p.rotation_mode = "XYZ"
        R_w = Euler([math.radians(d) for d in euler_xyz_deg], "XYZ").to_matrix().to_4x4()
        M_rest = p.bone.matrix_local.to_3x3().to_4x4()
        p.matrix_basis = M_rest.inverted() @ R_w @ M_rest

    rot_world("Pelvis", (0.0, 4.0, 0.0))
    rot_world("Spine", (0.0, 10.0, 0.0))
    rot_world("Chest", (0.0, 6.0, 0.0))
    rot_world("Head", (0.0, -16.0, 0.0))
    for side, s in (("L", 1.0), ("R", -1.0)):
        rot_world(f"Thigh_{side}", (0.0, -84.0, s * 6.0))
        rot_world(f"Shin_{side}", (0.0, 74.0, 0.0))
        rot_world(f"Foot_{side}", (0.0, 10.0, 0.0))
        rot_world(f"UArm_{side}", (-s * 22.0, -56.0, s * 10.0))
        rot_world(f"FArm_{side}", (0.0, -28.0, -s * 14.0))

    bpy.ops.object.mode_set(mode="OBJECT")
    sm = sailor.modifiers.new("SmoothJoints", "CORRECTIVE_SMOOTH")
    sm.factor = 0.55
    sm.iterations = 6
    sm.smooth_type = "LENGTH_WEIGHTED"

    bpy.context.view_layer.objects.active = sailor
    for mod in list(sailor.modifiers):
        bpy.ops.object.modifier_apply(modifier=mod.name)
    sailor.parent = None
    bpy.data.objects.remove(arm_obj)

    vr = get_verts(sailor)
    rmn, rmx = vr.min(0), vr.max(0)
    sailor.data.transform(Matrix.Translation((-(rmn[0] + rmx[0]) * 0.5, -(rmn[1] + rmx[1]) * 0.5, -rmn[2])))
    vr_final = get_verts(sailor)
    # Record forward outstretched hand grip centers & thwart seat height for gig spawner
    fwd_hands = vr_final[(vr_final[:, 0] > vr_final[:, 0].max() - 0.14) & (vr_final[:, 2] > 0.65)]
    hand_l = fwd_hands[fwd_hands[:, 1] > 0.0]
    hand_r = fwd_hands[fwd_hands[:, 1] < 0.0]
    seat_slice = vr_final[(vr_final[:, 0] < -0.10) & (vr_final[:, 2] > 0.25) & (vr_final[:, 2] < 0.65)]
    seat_z = float(seat_slice[:, 2].min()) if len(seat_slice) else 0.42
    meta = {
        "pose": "seated_gig_rower",
        "seat_thwart_z_m": round(seat_z, 3),
        "hand_grip_L_m": [round(float(x), 3) for x in hand_l.mean(0)] if len(hand_l) else [0.34, 0.20, 0.85],
        "hand_grip_R_m": [round(float(x), 3) for x in hand_r.mean(0)] if len(hand_r) else [0.34, -0.20, 0.85],
        "skintokens_rig_cmd": SKINTOKENS_CMD,
    }
    return export_asset_bundle("SailorCrewRower", sailor, scale, extra_meta=meta)


# ------------------------------------------------------------------------------
# 4. Town Buildings: TownTavernV4 (11m), TownChandleryV4 (11.5m), TownFoundryV4 (15m)
# ------------------------------------------------------------------------------
def prep_building(glb_name, asset_name, target_height_m, rotz_deg, rough=0.80):
    bldg = load_clean_glb(os.path.join(IN_DIR, glb_name), dielectric_default_rough=rough)
    if abs(rotz_deg) > 1e-4:
        bldg.data.transform(Matrix.Rotation(math.radians(rotz_deg), 4, "Z"))
    vb = get_verts(bldg)
    mn, mx = vb.min(0), vb.max(0)
    scale = float(target_height_m) / float(mx[2] - mn[2])
    ctr_xy = 0.5 * (mn[:2] + mx[:2])
    G = Matrix.Scale(scale, 4) @ Matrix.Translation((-ctr_xy[0], -ctr_xy[1], -mn[2]))
    bldg.data.transform(G)
    return export_asset_bundle(
        asset_name,
        bldg,
        scale,
        extra_meta={"target_height_m": float(target_height_m), "front_axis": "+X"},
    )


def main():
    print(f"=== prep_b4_assets: IN={IN_DIR} OUT={OUT_ROOT} ===")
    results = [
        prep_patrol_airship(),
        prep_sea_gull(),
        prep_sailor_stand(),
        prep_sailor_rower(),
        prep_building("b4_town_tavern.glb", "TownTavernV4", target_height_m=11.0, rotz_deg=90.0, rough=0.82),
        prep_building("b4_town_chandlery.glb", "TownChandleryV4", target_height_m=11.5, rotz_deg=90.0, rough=0.80),
        prep_building("b4_town_foundry.glb", "TownFoundryV4", target_height_m=15.0, rotz_deg=0.0, rough=0.76),
    ]
    manifest_path = os.path.join(OUT_ROOT, "p28_b4_manifest.json")
    with open(manifest_path, "w") as f:
        json.dump(
            {
                "generator": "cluster/prep_b4_assets.py",
                "source_dir": IN_DIR,
                "assets": [r["base_name"] for r in results],
                "skintokens_rig_cmd": SKINTOKENS_CMD,
                "summaries": results,
            },
            f,
            indent=1,
        )
    print(f"=== ALL {len(results)} P28 B4 ASSETS READY -> {manifest_path} ===")


if __name__ == "__main__":
    main()
