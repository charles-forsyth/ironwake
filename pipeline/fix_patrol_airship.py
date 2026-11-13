#!/usr/bin/env python3
"""Rebuild clean PatrolAirshipV4 (70 m hull) with riveted shaft housings and 3 procedural
4-blade laminated mahogany + brass propellers (PART_prop_port, PART_prop_stbd, PART_prop_stern),
eliminating comb-tooth mesh cuts on fused nacelle struts.

Usage:
  blender -b --factory-startup -P pipeline/fix_patrol_airship.py -- [in_glb] [out_dir]
"""
import json
import math
import os
import sys

import bmesh  # type: ignore
import bpy  # type: ignore
import numpy as np
from mathutils import Matrix, Vector  # type: ignore

ASSETS_ROOT = os.environ.get("IRONWAKE_ASSETS", os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
IN_GLB = os.path.abspath(argv[0]) if len(argv) >= 1 else os.path.join(ASSETS_ROOT, "models", "bodies4", "b4_patrol_airship_body.glb")
OUT_DIR = os.path.abspath(argv[1]) if len(argv) >= 2 else os.path.join(ASSETS_ROOT, "models", "ue_ready", "p28")


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


def build_propeller_mesh(name, radius_m, forward_spinner_sign=1.0, handedness=1.0, mat_wood=None, mat_brass=None):
    """Build a 4-blade Victorian laminated mahogany + polished brass airship propeller around local (0,0,0)."""
    bm_wood = bmesh.new()
    bm_brass = bmesh.new()
    r = radius_m
    s_fwd = forward_spinner_sign

    add_cyl(bm_brass, Vector((-0.14 * r * s_fwd, 0.0, 0.0)), Vector((0.08 * r * s_fwd, 0.0, 0.0)), 0.135 * r, 0.135 * r, 18)
    add_cyl(bm_brass, Vector((0.08 * r * s_fwd, 0.0, 0.0)), Vector((0.28 * r * s_fwd, 0.0, 0.0)), 0.135 * r, 0.024 * r, 18)
    add_cyl(bm_brass, Vector((-0.03 * r * s_fwd, 0.0, 0.0)), Vector((0.03 * r * s_fwd, 0.0, 0.0)), 0.162 * r, 0.162 * r, 18)

    for k in range(4):
        ang = k * (math.pi * 0.5)
        r_roll = Matrix.Rotation(ang, 3, "X")
        add_cyl(bm_brass, r_roll @ Vector((0.0, 0.11 * r, 0.0)), r_roll @ Vector((0.0, 0.25 * r, 0.0)), 0.052 * r, 0.044 * r, 12)
        add_cyl(bm_brass, r_roll @ Vector((0.0, 0.23 * r, 0.0)), r_roll @ Vector((0.0, 0.27 * r, 0.0)), 0.058 * r, 0.055 * r, 12)

        n_seg = 6
        for seg in range(n_seg):
            t = (seg + 0.5) / n_seg
            rad_y = r * (0.24 + 0.74 * t)
            seg_len = (0.74 * r / n_seg) * 1.10
            chord = r * (0.115 + 0.095 * math.sin(math.pi * min(1.0, t * 0.92 + 0.08)))
            thick = r * (0.030 * (1.0 - 0.55 * t) + 0.008)
            pitch_rad = math.radians((42.0 - 24.0 * t) * handedness)
            r_seg = r_roll @ Matrix.Rotation(pitch_rad, 3, "Y")
            ctr = r_roll @ Vector((0.0, rad_y, 0.0))
            add_box(bm_wood, ctr, thick, seg_len, chord, rot_mat=r_seg)
            if t >= 0.42:
                le_off = r_seg @ Vector((0.0, 0.0, 0.46 * chord * handedness))
                add_box(bm_brass, ctr + le_off, thick * 1.08, seg_len * 1.01, chord * 0.16, rot_mat=r_seg)

    o_wood = bm_to_obj(name, bm_wood, mat_wood)
    o_brass = bm_to_obj(name + "_brass", bm_brass, mat_brass)
    bpy.ops.object.select_all(action="DESELECT")
    o_wood.select_set(True)
    o_brass.select_set(True)
    bpy.context.view_layer.objects.active = o_wood
    bpy.ops.object.join()
    return o_wood


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=IN_GLB)
    meshes = [o for o in bpy.data.objects if o.type == "MESH"]
    for o in meshes:
        o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    if len(meshes) > 1:
        bpy.ops.object.join()
    body = bpy.context.view_layer.objects.active
    body.name = "PatrolAirshipV4"
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    body.data.transform(Matrix.Rotation(math.radians(90.0), 4, "Z"))
    vb = get_verts(body)
    mn, mx = vb.min(0), vb.max(0)
    scale = 70.0 / (mx[0] - mn[0])
    ctr_xy = 0.5 * (mn[:2] + mx[:2])
    body.data.transform(Matrix.Scale(scale, 4) @ Matrix.Translation((-ctr_xy[0], -ctr_xy[1], -mn[2])))

    va = get_verts(body)
    amn, amx = va.min(0), va.max(0)
    span_y, span_z = amx[1] - amn[1], amx[2] - amn[2]

    def measure_pod(sign_y):
        mask = (va[:, 1] * sign_y > 0.30 * span_y) & (va[:, 2] < amn[2] + 0.45 * span_z)
        pod = va[mask]
        pmin, pmax = pod.min(0), pod.max(0)
        nose = pod[pod[:, 0] > pmax[0] - 0.06 * (pmax[0] - pmin[0])]
        return float(pmax[0]), float(np.median(nose[:, 1])), float(np.median(nose[:, 2])), 0.5 * float(max(pmax[1] - pmin[1], pmax[2] - pmin[2]))

    px_port, py_port, pz_port, pr_port = measure_pod(+1.0)
    px_stbd, py_stbd, pz_stbd, pr_stbd = measure_pod(-1.0)
    horiz_fins = va[(va[:, 0] < amn[0] + 0.08 * 70.0) & (np.abs(va[:, 1]) > 0.06 * span_y)]
    stern_z = float(np.median(horiz_fins[:, 2]))
    stern_cone = va[(va[:, 0] < amn[0] + 0.04 * 70.0) & (np.abs(va[:, 1]) < 0.04 * span_y) & (np.abs(va[:, 2] - stern_z) < 0.05 * span_z)]
    stern_cone_min_x = float(stern_cone[:, 0].min()) if len(stern_cone) else float(amn[0] + 1.0)
    stern_hub_x = float(amn[0]) - 0.95

    mat_wood = make_pbr_mat("IW_PropWood", (0.26, 0.14, 0.07, 1.0), metal=0.0, rough=0.54)
    mat_brass = make_pbr_mat("IW_PropBrass", (0.72, 0.54, 0.22, 1.0), metal=0.88, rough=0.32)
    mat_iron = make_pbr_mat("IW_AirshipIron", (0.08, 0.085, 0.09, 1.0), metal=0.78, rough=0.48)

    piv_port = Vector((px_port + 0.55, py_port, pz_port))
    piv_stbd = Vector((px_stbd + 0.55, py_stbd, pz_stbd))
    piv_stern = Vector((stern_hub_x, 0.0, stern_z))

    bm_boss = bmesh.new()
    add_cyl(bm_boss, Vector((stern_cone_min_x + 0.80, 0.0, stern_z)), Vector((stern_hub_x + 0.35, 0.0, stern_z)), 0.82, 0.52, 18)
    add_cyl(bm_boss, Vector((px_port - 0.45, py_port, pz_port)), Vector((piv_port.x - 0.15, py_port, pz_port)), pr_port * 0.32, 0.56, 16)
    add_cyl(bm_boss, Vector((px_stbd - 0.45, py_stbd, pz_stbd)), Vector((piv_stbd.x - 0.15, py_stbd, pz_stbd)), pr_stbd * 0.32, 0.56, 16)
    o_boss = bm_to_obj("_airship_shaft_bosses", bm_boss, mat_iron)
    bpy.ops.object.select_all(action="DESELECT")
    body.select_set(True)
    o_boss.select_set(True)
    bpy.context.view_layer.objects.active = body
    bpy.ops.object.join()

    p_port = build_propeller_mesh("PART_prop_port", 4.65, 1.0, 1.0, mat_wood, mat_brass)
    p_stbd = build_propeller_mesh("PART_prop_stbd", 4.65, 1.0, -1.0, mat_wood, mat_brass)
    p_stern = build_propeller_mesh("PART_prop_stern", 3.85, -1.0, 1.0, mat_wood, mat_brass)
    p_port.location = piv_port
    p_stbd.location = piv_stbd
    p_stern.location = piv_stern

    os.makedirs(OUT_DIR, exist_ok=True)
    out_glb = os.path.join(OUT_DIR, "PatrolAirshipV4_rigged.glb")
    bpy.ops.export_scene.gltf(filepath=out_glb, export_format="GLB")
    meta = {
        "model": "PatrolAirshipV4",
        "length_m": 70.0,
        "parts": [
            {"name": "PART_prop_port", "axis": "X", "pivot_m": [round(x, 4) for x in piv_port]},
            {"name": "PART_prop_stbd", "axis": "X", "pivot_m": [round(x, 4) for x in piv_stbd]},
            {"name": "PART_prop_stern", "axis": "X", "pivot_m": [round(x, 4) for x in piv_stern]},
        ],
    }
    with open(os.path.join(OUT_DIR, "PatrolAirshipV4_parts.json"), "w") as f:
        json.dump(meta, f, indent=2)
    print("Wrote", out_glb)


if __name__ == "__main__":
    main()
