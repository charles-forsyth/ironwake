#!/usr/bin/env python3
"""Build matte-shaded, assembled, WebGL-optimized .glb models and manifest.json for Ironwake.

Assembles multi-part vessels, aircraft, harbour architecture, and crewed small craft from
source meshes, converts raw metallic Principled BSDF materials to painted-miniature matte
(Metallic=0.0, Roughness=0.72, preserving metallic sheen only on genuine small brass
hardware), aligns bow/front to -Y in Blender Z-up (+Z forward in glTF +Y-up so
<model-viewer camera-orbit="32deg 74deg auto"> frames a heroic 3/4 front-bow view),
grounds base at z=0, decimates dense meshes into the 15k-38k face budget, and compresses
embedded textures to JPEG quality 88 (< 4.5 MB per .glb).

Usage:
  env -u PYTHONPATH blender -b --factory-startup -P pipeline/prep_web_models.py -- \
    --models-dir ./models --out-dir ./docs/models
"""

import argparse
import json
import math
import os
import sys

import bmesh  # type: ignore
import bpy  # type: ignore
import numpy as np
from mathutils import Matrix, Vector  # type: ignore


BRASS_MATERIAL_TAGS = (
    "PropBrass",
    "TenderBrass",
    "SignBrass",
    "RowlockBrass",
    "HubBrass",
)


MODEL_SPECS = [
    {
        "id": "tall_brig_rigged",
        "title": "Tall Brig Curlew",
        "category": "ships",
        "qa_score": 9.5,
        "unreal_role": "Articulated 2-Mast Merchant Brig ( Hull + 17 Animated Rig & Sail Parts )",
        "description": (
            "42 m two-masted square-rigged brig assembled with topmasts, jibboom, standing "
            "shrouds and ratlines, six bracing yards, six roached square sails, gaff spanker, "
            "fore staysail, and outer jib."
        ),
    },
    {
        "id": "ironclad_devastation",
        "title": "Breastwork Turret Ironclad",
        "category": "ships",
        "qa_score": 8.8,
        "unreal_role": "Capital Escort & Coastal Bombardment Ironclad ( 125 m, 8,000 t )",
        "description": (
            "125 m breastwork turret ironclad with tumblehome armoured casemate, underwater "
            "ram bow, twin central funnels, and fore-and-aft heavy gun turrets."
        ),
    },
    {
        "id": "cargo_airship",
        "title": "Brass-Plated Cargo Dirigible",
        "category": "aircraft",
        "qa_score": 8.2,
        "unreal_role": "High-Altitude Heavy Freight Dirigible ( 4 Outrigger Screws, 310 m Cruise )",
        "description": (
            "110 m bluff-hulled steampunk cargo airship with ribbed canvas-and-brass envelope, "
            "underslung cargo gondola, external catwalks, cruciform tailplanes, and four "
            "outrigger propellers."
        ),
    },
    {
        "id": "patrol_airship",
        "title": "Crown Patrol Airship Aurora Watch",
        "category": "aircraft",
        "qa_score": 9.1,
        "unreal_role": "Harbour Overwatch & Reconnaissance Dirigible ( 3 Spinning Screws, 240 m Cruise )",
        "description": (
            "70 m Crown aerial patrol dirigible assembled from PatrolAirshipV4 with twin "
            "forward nacelle tractor screws and an aft cruciform tail pusher propeller in "
            "laminated mahogany and polished brass."
        ),
    },
    {
        "id": "paddle_steamer",
        "title": "Side-Wheel Steamer Duchess of Kestrel",
        "category": "ships",
        "qa_score": 9.2,
        "unreal_role": "Packet & Passenger Side-Wheel Steamer ( Dual Sponson Wheels + Churn FX )",
        "description": (
            "45 m side-wheel packet steamer assembled with recessed sponson wheel-wells and "
            "port/starboard multi-spoke iron and red-timber paddle wheels."
        ),
    },
    {
        "id": "salvage_tender",
        "title": "Salvage Tender Dredger's Hope",
        "category": "ships",
        "qa_score": 9.6,
        "unreal_role": "Wreck-Diving Support Vessel ( Slewing A-Frame Derrick + Pendulum Diving Bell )",
        "description": (
            "30 m iron salvage workboat equipped with an aft steam-winch turntable, twin-leg "
            "A-frame derrick boom slewed 22 degrees outboard, and a suspended brass-portholed "
            "diving bell."
        ),
    },
    {
        "id": "rowing_gig_crewed",
        "title": "Crewed Harbour Rowing Gig",
        "category": "craft_crew",
        "qa_score": 9.3,
        "unreal_role": "Inner-Harbour Ship-to-Shore Gig ( 6 Synchronised Oars + 4 Seated Crew )",
        "description": (
            "8 m clinker-built wooden harbour gig crewed by three 1870s oilskin oarsmen seated "
            "on the thwarts, a stern-sheets coxswain, and six spoon-bladed ash oars mounted in "
            "brass gunwale crutches."
        ),
    },
    {
        "id": "coastal_steamer",
        "title": "Tramp Collier SS Calloway",
        "category": "ships",
        "qa_score": 8.0,
        "unreal_role": "Coastal Coal & Ore Freighter ( 50 m Bulk Merchant Traffic )",
        "description": (
            "50 m iron-hulled coastal tramp collier with raised forecastle, central bridge "
            "deckhouse, tall black funnel, and fore-and-aft cargo derricks."
        ),
    },
    {
        "id": "pilot_cutter",
        "title": "Pilot Cutter Pathfinder",
        "category": "ships",
        "qa_score": 7.8,
        "unreal_role": "Fast Channel Pilot & Escort Cutter ( Bar Entry & Reef Guidance )",
        "description": (
            "18 m gaff-rigged pilot cutter with tan weather-cloth mainsail, staysail, jibboom "
            "headsail, and bold hull pilot markings built for rough bar crossings."
        ),
    },
    {
        "id": "harbour_tug",
        "title": "Steam Harbour Tug Hodman",
        "category": "ships",
        "qa_score": 9.0,
        "unreal_role": "High-Thrust Berthing & Towage Steam Tug ( 22 m Harbour Workhorse )",
        "description": (
            "22 m heavy steel harbour steam tug fitted with a braided rope bow pudding fender, "
            "forward timber wheelhouse, tall coal stack, and low aft towing deck."
        ),
    },
    {
        "id": "steam_pinnace",
        "title": "Brass Steam Pinnace",
        "category": "craft_crew",
        "qa_score": 8.0,
        "unreal_role": "Admiralty Officer & Quay Dispatch Launch ( 15 m Canopy Steam Boat )",
        "description": (
            "15 m Victorian naval steam launch featuring a plumb stem, exposed vertical brass "
            "boiler and raked funnel forward, and a stanchion-supported passenger canopy aft."
        ),
    },
    {
        "id": "town_tavern",
        "title": "The Anchor & Brass Tavern",
        "category": "architecture",
        "qa_score": 8.8,
        "unreal_role": "Quayside Landmark Public House ( TownTavernV4 + Mounted SignAnchorTavern )",
        "description": (
            "11 m half-timbered harbourfront tavern with stone ground-floor bay windows, "
            "jettied Tudor upper storey, dual brick chimneys, and the painted gold-and-maroon "
            "Anchor & Brass signboard mounted on its front crossbeam."
        ),
    },
    {
        "id": "town_foundry",
        "title": "Ironcrag Blast Foundry",
        "category": "architecture",
        "qa_score": 8.3,
        "unreal_role": "South-Wharf Heavy Industrial Smelting Works ( 15 m Brick Clerestory Hall )",
        "description": (
            "15 m red-brick Victorian blast foundry with arched casting-floor windows, vented "
            "clerestory roof monitor, and integrated square brick furnace stack."
        ),
    },
    {
        "id": "steam_crane",
        "title": "Rail-Mounted Steam Dock Crane",
        "category": "architecture",
        "qa_score": 8.0,
        "unreal_role": "Coaling Pier & Foundry Wharf Cargo Portal Crane ( 18.5 m Lattice Jib )",
        "description": (
            "18.5 m rail-straddling steel portal dock crane with a riveted boiler cab, vertical "
            "exhaust chimney, lattice jib boom, sheave block, and heavy cargo hook."
        ),
    },
    {
        "id": "coaling_station",
        "title": "Harbour Coaling Tipple",
        "category": "architecture",
        "qa_score": 8.0,
        "unreal_role": "Bunkering Wharf Coal Elevator & Chute Tower ( 24 m Industrial Tipple )",
        "description": (
            "24 m dockside coaling station combining an inclined bucket-conveyor ramp, overhead "
            "coal storage hoppers, counterweighted gravity chutes, and timber-and-iron trestles."
        ),
    },
    {
        "id": "windmill_stigsby",
        "title": "Stigsby Ridge Windmill",
        "category": "architecture",
        "qa_score": 8.4,
        "unreal_role": "Headland Daymark & Grain Mill ( Tower + 4 Rotating Lattice Sail Sweeps )",
        "description": (
            "22 m coastal stone-and-timber smock windmill crowned by an ogee cap and four "
            "articulated oak lattice sail sweeps overlooking the approaches."
        ),
    },
    {
        "id": "sailor_mariner",
        "title": "1870s Oilskin Mariner",
        "category": "craft_crew",
        "qa_score": 8.0,
        "unreal_role": "Quarterdeck Helmsman, Salvage Rigger & Quay Dockhand ( 1.78 m Crew Mesh )",
        "description": (
            "1.78 m Victorian merchant mariner dressed in dark weatherproof oilskins, double-"
            "breasted peacoat, sea boots, and peaked cap for shipboard helm and quayside duty."
        ),
    },
    {
        "id": "sea_gull",
        "title": "Harbour Herring Gull",
        "category": "craft_crew",
        "qa_score": 8.2,
        "unreal_role": "Ambient Harbour & Wake Flock Bird ( 1.30 m Span, Hinged L/R Wings )",
        "description": (
            "1.30 m wingspan coastal herring gull assembled from SeaGullV4 with hinged port and "
            "starboard primary wings posed in a slight dihedral harbour glide."
        ),
    },
]


def parse_args():
    argv = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    p = argparse.ArgumentParser(description="Build WebGL .glb models and manifest.json.")
    p.add_argument(
        "--models-dir",
        default=os.environ.get("IW_MODELS_DIR", "models"),
        help="Read-only source directory containing raw/rigged GLBs and ue_ready bundles.",
    )
    p.add_argument(
        "--out-dir",
        default=os.environ.get("IW_OUT_DIR", os.path.join("docs", "models")),
        help="Destination directory for exported .glb files and manifest.json.",
    )
    p.add_argument(
        "--thumbs-dir",
        default=os.environ.get("IW_THUMBS_DIR", ""),
        help="Optional directory to render 3/4 <model-viewer> QA thumbnails.",
    )
    p.add_argument(
        "--only",
        default="",
        help="Optional comma-separated subset of model IDs to build.",
    )
    p.add_argument("--task-idx", type=int, default=0, help="Optional worker slice index (0-based).")
    p.add_argument("--num-tasks", type=int, default=1, help="Total number of worker slices.")
    p.add_argument(
        "--device",
        default=os.environ.get("IW_BLENDER_DEVICE", "OPTIX"),
        choices=["OPTIX", "CUDA", "METAL", "CPU"],
        help="Cycles device for optional QA thumbnail renders.",
    )
    return p.parse_args(argv)


# ------------------------------------------------------------------------------
# Geometry & Material Helpers
# ------------------------------------------------------------------------------
def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def make_mat(name, rgba, metal=0.0, rough=0.72):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = next(n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Base Color"].default_value = rgba
    bsdf.inputs["Metallic"].default_value = metal
    bsdf.inputs["Roughness"].default_value = rough
    return m


def bm_to_obj(name, bm, mat):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    if mat is not None:
        me.materials.append(mat)
    o = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(o)
    return o


def add_cyl(bm, p0, p1, r0, r1=None, segs=10):
    if r1 is None:
        r1 = r0
    d = p1 - p0
    if d.length < 1e-6:
        return
    mid = (p0 + p1) * 0.5
    g = bmesh.ops.create_cone(
        bm, cap_ends=True, segments=segs, radius1=r0, radius2=r1, depth=d.length
    )
    rot = Vector((0.0, 0.0, 1.0)).rotation_difference(d.normalized()).to_matrix().to_4x4()
    bmesh.ops.transform(bm, matrix=Matrix.Translation(mid) @ rot, verts=g["verts"])


def add_box(bm, center, sx, sy, sz, rot_mat=None):
    g = bmesh.ops.create_cube(bm, size=1.0)
    m = Matrix.Diagonal((sx, sy, sz, 1.0))
    if rot_mat is not None:
        m = rot_mat.to_4x4() @ m
    bmesh.ops.transform(bm, matrix=Matrix.Translation(center) @ m, verts=g["verts"])


def get_verts_np(obj):
    return np.array([tuple(v.co) for v in obj.data.vertices], dtype=np.float64)


def world_bbox(objs):
    mins = []
    maxs = []
    for o in objs:
        if o.type != "MESH" or len(o.data.vertices) == 0:
            continue
        v = get_verts_np(o)
        mw = np.array(o.matrix_world, dtype=np.float64)
        vw = (v @ mw[:3, :3].T) + mw[:3, 3]
        mins.append(vw.min(axis=0))
        maxs.append(vw.max(axis=0))
    lo = Vector(np.min(mins, axis=0))
    hi = Vector(np.max(maxs, axis=0))
    return lo, hi


def select_only(objs, active=None):
    target_set = set(objs)
    for o in bpy.context.scene.objects:
        o.select_set(o in target_set)
    bpy.context.view_layer.objects.active = active or (objs[0] if objs else None)


def join_objs(objs, name=None):
    if len(objs) == 1:
        if name:
            objs[0].name = name
        return objs[0]
    select_only(objs, active=objs[0])
    bpy.ops.object.join()
    res = bpy.context.view_layer.objects.active
    if name:
        res.name = name
    return res


def decimate_mesh(obj, target_faces):
    cur_faces = len(obj.data.polygons)
    if cur_faces <= target_faces or target_faces <= 0:
        return
    select_only([obj], active=obj)
    mod = obj.modifiers.new("WebDecimate", "DECIMATE")
    mod.decimate_type = "COLLAPSE"
    mod.ratio = max(0.02, min(1.0, float(target_faces) / float(cur_faces)))
    bpy.ops.object.modifier_apply(modifier=mod.name)


def import_glb_baked(glb_path):
    if not os.path.isfile(glb_path):
        raise FileNotFoundError(f"Missing GLB: {glb_path}")
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=glb_path)
    added = [o for o in bpy.data.objects if o not in before]
    meshes = []
    for o in added:
        if o.type == "MESH" and len(o.data.polygons) > 0:
            mw = o.matrix_world.copy()
            o.parent = None
            if o.data.users > 1:
                o.data = o.data.copy()
            o.data.transform(mw)
            o.matrix_world = Matrix.Identity(4)
            meshes.append(o)
    for o in added:
        if o not in meshes:
            bpy.data.objects.remove(o, do_unlink=True)
    return meshes


def import_fbx_bundle(fbx_path):
    if not os.path.isfile(fbx_path):
        raise FileNotFoundError(f"Missing FBX: {fbx_path}")
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=fbx_path)
    added = [o for o in bpy.data.objects if o not in before]
    bundle_dir = os.path.dirname(fbx_path)
    mat_json = os.path.join(bundle_dir, "materials.json")
    mdata = json.load(open(mat_json)) if os.path.isfile(mat_json) else {}

    meshes = []
    for o in added:
        if o.type == "MESH" and len(o.data.polygons) > 0:
            mw = o.matrix_world.copy()
            o.parent = None
            if o.data.users > 1:
                o.data = o.data.copy()
            o.data.transform(mw)
            o.matrix_world = Matrix.Identity(4)
            meshes.append(o)
    for o in added:
        if o not in meshes:
            bpy.data.objects.remove(o, do_unlink=True)

    for o in meshes:
        for slot in o.material_slots:
            m = slot.material
            if not m:
                continue
            m.use_nodes = True
            nt = m.node_tree
            bsdf = next((n for n in nt.nodes if n.type == "BSDF_PRINCIPLED"), None)
            if not bsdf:
                bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
            info = mdata.get(m.name)
            if not info:
                for k, v in mdata.items():
                    if m.name.startswith(k) or k in m.name:
                        info = v
                        break
            if not info and len(mdata) == 1:
                info = list(mdata.values())[0]

            for link in list(nt.links):
                if link.to_node == bsdf and link.to_socket.name in (
                    "Base Color",
                    "Metallic",
                    "Roughness",
                    "Normal",
                ):
                    nt.links.remove(link)

            if info:
                c = info.get("color", [1.0, 1.0, 1.0])
                bsdf.inputs["Base Color"].default_value = (c[0], c[1], c[2], 1.0)
                if "base" in info:
                    tp = os.path.join(bundle_dir, info["base"])
                    if os.path.isfile(tp):
                        tn = nt.nodes.new("ShaderNodeTexImage")
                        tn.image = bpy.data.images.load(tp, check_existing=True)
                        tn.image.colorspace_settings.name = "sRGB"
                        nt.links.new(tn.outputs["Color"], bsdf.inputs["Base Color"])
                if "normal" in info:
                    np_p = os.path.join(bundle_dir, info["normal"])
                    if os.path.isfile(np_p):
                        tn = nt.nodes.new("ShaderNodeTexImage")
                        tn.image = bpy.data.images.load(np_p, check_existing=True)
                        tn.image.colorspace_settings.name = "Non-Color"
                        nm_node = nt.nodes.new("ShaderNodeNormalMap")
                        nt.links.new(tn.outputs["Color"], nm_node.inputs["Color"])
                        nt.links.new(nm_node.outputs["Normal"], bsdf.inputs["Normal"])
    return meshes


def enforce_web_materials():
    """Force Metallic=0.0, Roughness=0.72 on all materials except small brass hardware."""
    for m in bpy.data.materials:
        if not m:
            continue
        m.use_nodes = True
        nt = m.node_tree
        bsdf = next((n for n in nt.nodes if n.type == "BSDF_PRINCIPLED"), None)
        if not bsdf:
            continue
        for link in list(nt.links):
            if link.to_node == bsdf and link.to_socket.name in ("Metallic", "Roughness"):
                nt.links.remove(link)
        is_brass = any(tag in m.name for tag in BRASS_MATERIAL_TAGS)
        if is_brass:
            bsdf.inputs["Metallic"].default_value = 0.70
            bsdf.inputs["Roughness"].default_value = 0.35
        else:
            bsdf.inputs["Metallic"].default_value = 0.0
            bsdf.inputs["Roughness"].default_value = 0.72


def clamp_image_sizes(max_dim=2048):
    for img in bpy.data.images:
        if img.size[0] > max_dim or img.size[1] > max_dim:
            scale = float(max_dim) / float(max(img.size[0], img.size[1]))
            w = max(1, int(round(img.size[0] * scale)))
            h = max(1, int(round(img.size[1] * scale)))
            img.scale(w, h)


def finalize_orientation_and_ground(yaw_deg):
    """Rotate scene by yaw_deg around Z so bow/front points along -Y (+Z in glTF),
    center XY at origin, and seat min_z cleanly at z = 0.0."""
    # .location edits made by the builders are not reflected in matrix_world until the
    # depsgraph is evaluated; without this every PART_* collapses to the origin.
    bpy.context.view_layer.update()
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    # Bake any non-uniform/uniform object scale into mesh data first so rigid world
    # transforms keep local PART_* origins cleanly on their hinges/axles.
    for o in meshes:
        sx, sy, sz = o.scale
        if abs(sx - 1.0) > 1e-5 or abs(sy - 1.0) > 1e-5 or abs(sz - 1.0) > 1e-5:
            if o.data.users > 1:
                o.data = o.data.copy()
            o.data.transform(Matrix.Diagonal((sx, sy, sz, 1.0)))
            o.scale = (1.0, 1.0, 1.0)

    r_yaw = Matrix.Rotation(math.radians(yaw_deg), 4, "Z")
    for o in meshes:
        if o.name.startswith("PART_"):
            o.matrix_world = r_yaw @ o.matrix_world
        else:
            if o.data.users > 1:
                o.data = o.data.copy()
            o.data.transform(r_yaw @ o.matrix_world)
            o.matrix_world = Matrix.Identity(4)

    bpy.context.view_layer.update()
    lo, hi = world_bbox(meshes)
    shift = Vector((-(lo.x + hi.x) * 0.5, -(lo.y + hi.y) * 0.5, -lo.z))
    t_mat = Matrix.Translation(shift)
    for o in meshes:
        if o.name.startswith("PART_"):
            o.matrix_world = t_mat @ o.matrix_world
        else:
            o.data.transform(t_mat)

    bpy.context.view_layer.update()
    return world_bbox(meshes)


# ------------------------------------------------------------------------------
# Per-Model Builders (each leaves scene ready for finalize_orientation_and_ground)
# ------------------------------------------------------------------------------
def add_brig_standing_shrouds(hull_obj):
    """Add dark-tarred hemp standing shrouds, ratlines, and topmast backstays to HullBody."""
    vh = get_verts_np(hull_obj)
    rope_mat = next(
        (m for m in bpy.data.materials if m and m.name == "IW_StayRope"),
        None,
    ) or make_mat("IW_StayRope", (0.13, 0.10, 0.07, 1.0), metal=0.0, rough=0.72)

    bm = bmesh.new()
    # Native b2_brig_hull_rigged coords: bow = -X, stern = +X, foremast ~ -0.148, mainmast ~ +0.339
    mast_stations = [
        (-0.148, 0.405, 0.645),
        (0.339, 0.465, 0.695),
    ]
    for xm, z_cap, z_tm in mast_stations:
        sl = vh[(np.abs(vh[:, 0] - xm) < 0.09) & (vh[:, 2] > -0.17) & (vh[:, 2] < 0.04)]
        y_rail = float(np.percentile(np.abs(sl[:, 1]), 96)) * 0.94 if len(sl) else 0.235
        z_rail = float(np.percentile(sl[:, 2], 84)) if len(sl) else -0.065

        for sgn in (-1.0, 1.0):
            low_pts = []
            top_pt = Vector((xm + 0.006, sgn * 0.024, z_cap))
            for k in range(4):
                p_chn = Vector((xm + 0.010 + k * 0.026, sgn * y_rail, z_rail))
                add_cyl(bm, p_chn, top_pt, 0.0024, 0.0019, segs=6)
                low_pts.append(p_chn)

            # Horizontal ratline rungs between forward-most and aft-most lower shroud
            for r_i in range(1, 9):
                t = r_i / 9.5
                p_fwd = low_pts[0].lerp(top_pt, t)
                p_aft = low_pts[-1].lerp(top_pt, t)
                add_cyl(bm, p_fwd, p_aft, 0.0011, 0.0011, segs=5)

            # Topmast standing backstays aft to channels
            tm_head = Vector((xm + 0.012, sgn * 0.018, z_tm))
            for k in range(2):
                p_bk = Vector((xm + 0.125 + k * 0.032, sgn * y_rail * 0.93, z_rail))
                add_cyl(bm, p_bk, tm_head, 0.0019, 0.0016, segs=6)

    shroud_obj = bm_to_obj("_standing_shrouds", bm, rope_mat)
    return join_objs([hull_obj, shroud_obj], name="HullBody")


def build_01_tall_brig(models_dir):
    meshes = import_glb_baked(os.path.join(models_dir, "parts", "b2_brig_hull_rigged.glb"))
    spec = json.load(open(os.path.join(models_dir, "parts", "b2_brig_hull_rigged.json")))
    piv_lookup = {p["name"]: Vector(p["pivot"]) for p in spec["parts"]}

    bodies = [o for o in meshes if not o.name.startswith("PART_")]
    parts = [o for o in meshes if o.name.startswith("PART_")]
    hull = join_objs(bodies, name="HullBody")

    decimate_mesh(hull, 22500)
    hull = add_brig_standing_shrouds(hull)

    # Restore exact part local pivots so PART_* nodes preserve articulation axes in the GLB
    for p in parts:
        piv = piv_lookup.get(p.name)
        if piv is not None:
            p.data.transform(Matrix.Translation(-piv))
            p.location = piv

    # Scale bare-hull length (~1.995 units before bowsprit overhang) to 42.0 m
    scale = 42.0 / 1.995
    s_mat = Matrix.Scale(scale, 4)
    hull.data.transform(s_mat)
    for p in parts:
        p.data.transform(s_mat)
        p.location = p.location * scale

    return 90.0  # Native bow is -X -> +90 deg around Z puts bow at -Y


def build_simple_glb(glb_path, target_span_m, span_axis, target_faces, yaw_deg):
    meshes = import_glb_baked(glb_path)
    body = join_objs(meshes, name="HullBody")
    decimate_mesh(body, target_faces)
    lo, hi = world_bbox([body])
    cur_span = (hi - lo)[span_axis]
    scale = float(target_span_m) / float(cur_span)
    body.data.transform(Matrix.Scale(scale, 4))
    return yaw_deg


def build_03_cargo_airship(models_dir):
    meshes = import_glb_baked(os.path.join(models_dir, "parts", "body_cargo_airship_parts.glb"))
    spec = json.load(open(os.path.join(models_dir, "parts", "body_cargo_airship_parts.json")))
    piv_lookup = {"PART_" + p["name"]: (Vector(p["pivot"]), float(p["radius"])) for p in spec["parts"]}

    hull = join_objs([o for o in meshes if not o.name.startswith("PART_")], name="HullBody")
    parts = [o for o in meshes if o.name.startswith("PART_")]
    decimate_mesh(hull, 25500)

    wood_mat = make_mat("IW_CargoPropWood", (0.28, 0.16, 0.08, 1.0), metal=0.0, rough=0.72)
    brass_mat = make_mat("IW_CargoHubBrass", (0.72, 0.54, 0.22, 1.0), metal=0.70, rough=0.35)

    lo_h, hi_h = world_bbox([hull])
    scale = 110.0 / float(hi_h.y - lo_h.y)
    s_mat = Matrix.Scale(scale, 4)
    hull.data.transform(s_mat)

    for p in parts:
        p.data.materials.clear()
        p.data.materials.append(wood_mat)
        piv_info = piv_lookup.get(p.name)
        if piv_info is not None:
            piv, rad = piv_info
            p.data.transform(Matrix.Translation(-piv))
            # Add gleaming brass spinner cap along local Y shaft before scaling
            bm_hub = bmesh.new()
            add_cyl(bm_hub, Vector((0.0, -rad * 0.16, 0.0)), Vector((0.0, rad * 0.26, 0.0)), rad * 0.14, rad * 0.04, 14)
            o_hub = bm_to_obj("_hub_brass", bm_hub, brass_mat)
            p = join_objs([p, o_hub], name=p.name)
            p.data.transform(s_mat)
            p.location = piv * scale
        else:
            p.data.transform(s_mat)

    return 180.0  # Native nose is +Y -> 180 deg around Z puts nose at -Y


def build_04_patrol_airship(models_dir):
    p28 = os.path.join(models_dir, "ue_ready", "p28")
    body = join_objs(import_fbx_bundle(os.path.join(p28, "PatrolAirshipV4", "PatrolAirshipV4.fbx")), "HullBody")
    decimate_mesh(body, 24000)

    parts_meta = json.load(open(os.path.join(p28, "PatrolAirshipV4", "parts.json")))["parts"]
    for part_key, info in parts_meta.items():
        asset_nm = info["asset"]
        p_obj = join_objs(import_fbx_bundle(os.path.join(p28, asset_nm, f"{asset_nm}.fbx")), part_key)
        p_obj.location = Vector(info["pivot_m"])

    return -90.0  # Native bow is +X -> -90 deg around Z puts bow at -Y


def build_05_paddle_steamer(models_dir):
    p28 = os.path.join(models_dir, "ue_ready", "p28")
    body = join_objs(import_fbx_bundle(os.path.join(p28, "PaddleSteamerV3", "PaddleSteamerV3.fbx")), "HullBody")
    decimate_mesh(body, 23500)

    w_port = join_objs(
        import_fbx_bundle(os.path.join(p28, "PaddleSteamerV3_wheel_port", "PaddleSteamerV3_wheel_port.fbx")),
        "PART_wheel_port",
    )
    w_stbd = join_objs(
        import_fbx_bundle(os.path.join(p28, "PaddleSteamerV3_wheel_stbd", "PaddleSteamerV3_wheel_stbd.fbx")),
        "PART_wheel_stbd",
    )

    # Verified P28 sponson fit (0.86 wheel scale tucked cleanly inside guard rails, then 0.90 vessel scale -> 45 m)
    ship_scale = 45.0 / 50.0
    body.data.transform(Matrix.Scale(ship_scale, 4))
    for w_obj, piv in (
        (w_port, Vector((-1.3212, 6.34, 4.52))),
        (w_stbd, Vector((-1.3212, -6.22, 4.52))),
    ):
        w_obj.data.transform(Matrix.Scale(0.86 * ship_scale, 4))
        w_obj.location = piv * ship_scale

    return -90.0  # Native bow is +X -> -90 deg puts bow at -Y


def build_06_salvage_tender(models_dir):
    p28 = os.path.join(models_dir, "ue_ready", "p28")
    body = join_objs(import_fbx_bundle(os.path.join(p28, "SalvageTenderV3", "SalvageTenderV3.fbx")), "HullBody")
    decimate_mesh(body, 24500)

    crane = join_objs(
        import_fbx_bundle(os.path.join(p28, "SalvageTenderV3_crane", "SalvageTenderV3_crane.fbx")),
        "PART_crane",
    )
    bell = join_objs(
        import_fbx_bundle(os.path.join(p28, "SalvageTenderV3_bell", "SalvageTenderV3_bell.fbx")),
        "PART_bell",
    )

    ship_scale = 30.0 / 38.0
    body.data.transform(Matrix.Scale(ship_scale, 4))
    crane.data.transform(Matrix.Scale(ship_scale, 4))
    bell.data.transform(Matrix.Scale(0.76 * ship_scale, 4))

    # Slew A-frame crane 22 degrees outboard so crane & suspended diving bell silhouette clearly
    crane_piv = Vector((-4.65, 0.0522, 9.5654)) * ship_scale
    slew_rad = math.radians(-22.0)
    crane.location = crane_piv
    crane.rotation_euler = (0.0, 0.0, slew_rad)

    bell_off = Matrix.Rotation(slew_rad, 3, "Z") @ (Vector((-16.7654, 0.0, 8.2494)) * ship_scale)
    bell.location = crane_piv + bell_off
    bell.rotation_euler = (0.0, 0.0, slew_rad)

    return -90.0  # Native bow is +X -> -90 deg puts bow at -Y


def build_07_rowing_gig_crewed(models_dir):
    meshes = import_glb_baked(os.path.join(models_dir, "parts", "body_rowing_gig_parts.glb"))
    gig_spec = json.load(open(os.path.join(models_dir, "parts", "body_rowing_gig_parts.json")))
    hull = join_objs([o for o in meshes if not o.name.startswith("PART_")], name="HullBody")
    oars = [o for o in meshes if o.name.startswith("PART_")]

    decimate_mesh(hull, 15500)

    # Rotate +90 deg Z and scale hull length to 8.0 m (bow at +X, stern at -X, keel at z=0)
    rz90 = Matrix.Rotation(math.radians(90.0), 4, "Z")
    for o in [hull] + oars:
        o.data.transform(rz90)

    lo_h, hi_h = world_bbox([hull])
    sc_gig = 8.0 / max(hi_h.x - lo_h.x, hi_h.y - lo_h.y)
    g_mat = Matrix.Scale(sc_gig, 4) @ Matrix.Translation(
        (-(lo_h.x + hi_h.x) * 0.5, -(lo_h.y + hi_h.y) * 0.5, -lo_h.z)
    )
    for o in [hull] + oars:
        o.data.transform(g_mat)

    oar_mat = make_mat("IW_AshOar", (0.38, 0.25, 0.13, 1.0), metal=0.0, rough=0.72)
    brass_mat = make_mat("IW_RowlockBrass", (0.72, 0.54, 0.22, 1.0), metal=0.70, rough=0.35)

    # Position 6 gunwale oars on gleaming brass U-crutch rowlocks
    oar_pivs = {
        "PART_oar_0_port": Vector((0.42, -1.54, 1.98)),
        "PART_oar_0_stbd": Vector((0.42, 1.54, 1.98)),
        "PART_oar_1_port": Vector((-0.76, -1.52, 1.98)),
        "PART_oar_1_stbd": Vector((-0.76, 1.52, 1.98)),
        "PART_oar_2_port": Vector((-1.84, -1.44, 1.98)),
        "PART_oar_2_stbd": Vector((-1.84, 1.44, 1.98)),
    }
    bm_crutch = bmesh.new()
    for o in oars:
        o.data.materials.clear()
        o.data.materials.append(oar_mat)
        pinfo = next(p for p in gig_spec["parts"] if "PART_" + p["name"] == o.name)
        old_piv = g_mat @ (rz90 @ Vector(pinfo["pivot"]))
        new_piv = oar_pivs[o.name]
        o.data.transform(Matrix.Translation(-old_piv))
        o.location = new_piv

        add_cyl(bm_crutch, new_piv + Vector((0.0, 0.0, -0.14)), new_piv + Vector((0.0, 0.0, -0.01)), 0.026, 0.026, 10)
        add_box(bm_crutch, new_piv + Vector((0.05, 0.0, 0.0)), 0.028, 0.055, 0.09)
        add_box(bm_crutch, new_piv + Vector((-0.05, 0.0, 0.0)), 0.028, 0.055, 0.09)

    crutch_obj = bm_to_obj("_brass_rowlocks", bm_crutch, brass_mat)
    hull = join_objs([hull, crutch_obj], name="HullBody")

    # Load SailorCrewRower once, decimate cleanly to ~3.8k faces, and place 3 oarsmen + 1 coxswain
    rower_fbx = os.path.join(models_dir, "ue_ready", "p28", "SailorCrewRower", "SailorCrewRower.fbx")
    rower_base = join_objs(import_fbx_bundle(rower_fbx), "Crew_Coxswain")
    decimate_mesh(rower_base, 3800)

    crew_objs = []
    for idx, tx in enumerate((1.04, -0.16, -1.26)):
        cp = rower_base.copy()
        cp.data = rower_base.data.copy()
        cp.name = f"Crew_Oarsman_{idx}"
        bpy.context.scene.collection.objects.link(cp)
        cp.data.transform(
            Matrix.Translation(Vector((tx, -0.05 if idx == 1 else 0.05, 1.07)))
            @ Matrix.Rotation(math.radians(180.0), 4, "Z")
            @ Matrix.Scale(1.08, 4)
        )
        crew_objs.append(cp)

    # Stern coxswain facing forward (+X) on stern sheets
    rower_base.data.transform(Matrix.Translation(Vector((-2.52, 0.0, 1.08))) @ Matrix.Scale(1.06, 4))
    crew_objs.append(rower_base)
    join_objs([hull] + crew_objs, name="HullBody")

    return -90.0  # Assemble with bow at +X -> -90 deg puts bow at -Y


def build_12_town_tavern(models_dir):
    p28 = os.path.join(models_dir, "ue_ready", "p28")
    signs = os.path.join(models_dir, "ue_ready", "p28_signs")
    tav = join_objs(import_fbx_bundle(os.path.join(p28, "TownTavernV4", "TownTavernV4.fbx")), "TownTavern")
    decimate_mesh(tav, 26000)

    sign_obj = join_objs(
        import_fbx_bundle(os.path.join(signs, "SignAnchorTavern", "SignAnchorTavern.fbx")),
        "SignAnchorTavern",
    )
    sign_obj.data.transform(Matrix.Translation(Vector((4.42, 0.0, 3.48))))

    # Wrought-iron wall tie brackets behind signboard into front crossbeam
    iron_mat = make_mat("IW_SignBracketIron", (0.09, 0.09, 0.10, 1.0), metal=0.0, rough=0.72)
    bm_br = bmesh.new()
    for y_off in (-1.35, 1.35):
        for z_off in (3.62, 4.22):
            add_box(bm_br, Vector((4.34, y_off, z_off)), 0.22, 0.06, 0.06)
    br_obj = bm_to_obj("_sign_brackets", bm_br, iron_mat)

    join_objs([tav, sign_obj, br_obj], name="TownTavern")
    return -90.0  # Native front is +X -> -90 deg puts front facade & sign at -Y


def build_13_town_foundry(models_dir):
    p28 = os.path.join(models_dir, "ue_ready", "p28")
    fnd = join_objs(import_fbx_bundle(os.path.join(p28, "TownFoundryV4", "TownFoundryV4.fbx")), "TownFoundry")
    decimate_mesh(fnd, 26500)
    return -90.0


def build_16_windmill(models_dir):
    meshes = import_glb_baked(os.path.join(models_dir, "parts", "body_windmill_parts.glb"))
    spec = json.load(open(os.path.join(models_dir, "parts", "body_windmill_parts.json")))
    tower = join_objs([o for o in meshes if not o.name.startswith("PART_")], name="WindmillTower")
    sails = [o for o in meshes if o.name.startswith("PART_")][0]

    decimate_mesh(tower, 25000)
    oak_mat = make_mat("IW_WindmillSweepOak", (0.30, 0.19, 0.10, 1.0), metal=0.0, rough=0.72)
    sails.data.materials.clear()
    sails.data.materials.append(oak_mat)

    piv = Vector(spec["parts"][0]["pivot"])
    sails.data.transform(Matrix.Translation(-piv))
    sails.location = piv

    lo_all, hi_all = world_bbox([tower, sails])
    scale = 22.0 / float(hi_all.z - lo_all.z)
    s_mat = Matrix.Scale(scale, 4)
    tower.data.transform(s_mat)
    sails.data.transform(s_mat)
    sails.location = piv * scale

    return 0.0  # Sails already face -Y


def build_17_sailor(models_dir):
    p28 = os.path.join(models_dir, "ue_ready", "p28")
    sailor = join_objs(import_fbx_bundle(os.path.join(p28, "SailorCrewStand", "SailorCrewStand.fbx")), "SailorMariner")
    decimate_mesh(sailor, 22000)
    return -90.0  # Native face/toes point +X -> -90 deg puts face at -Y


def build_18_sea_gull(models_dir):
    p28 = os.path.join(models_dir, "ue_ready", "p28")
    body = join_objs(import_fbx_bundle(os.path.join(p28, "SeaGullV4", "SeaGullV4.fbx")), "SeaGullBody")
    wing_l = join_objs(import_fbx_bundle(os.path.join(p28, "SeaGullV4_wing_L", "SeaGullV4_wing_L.fbx")), "PART_wing_L")
    wing_r = join_objs(import_fbx_bundle(os.path.join(p28, "SeaGullV4_wing_R", "SeaGullV4_wing_R.fbx")), "PART_wing_R")

    # Proportionally decimate so total gull lands right around ~19k faces (inside 15k-38k budget)
    total_f = sum(len(o.data.polygons) for o in (body, wing_l, wing_r))
    if total_f > 19500:
        ratio = 19500.0 / float(total_f)
        for o in (body, wing_l, wing_r):
            decimate_mesh(o, max(1200, int(len(o.data.polygons) * ratio)))

    parts_meta = json.load(open(os.path.join(p28, "SeaGullV4", "parts.json")))["parts"]
    wing_l.location = Vector(parts_meta["PART_wing_L"]["pivot_m"])
    wing_l.rotation_euler = (math.radians(7.0), 0.0, 0.0)
    wing_r.location = Vector(parts_meta["PART_wing_R"]["pivot_m"])
    wing_r.rotation_euler = (math.radians(-7.0), 0.0, 0.0)

    return -90.0  # Native beak points +X -> -90 deg puts beak at -Y


def dispatch_build(model_id, models_dir):
    if model_id == "tall_brig_rigged":
        return build_01_tall_brig(models_dir)
    if model_id == "ironclad_devastation":
        return build_simple_glb(os.path.join(models_dir, "ironclad.glb"), 125.0, 0, 27500, -90.0)
    if model_id == "cargo_airship":
        return build_03_cargo_airship(models_dir)
    if model_id == "patrol_airship":
        return build_04_patrol_airship(models_dir)
    if model_id == "paddle_steamer":
        return build_05_paddle_steamer(models_dir)
    if model_id == "salvage_tender":
        return build_06_salvage_tender(models_dir)
    if model_id == "rowing_gig_crewed":
        return build_07_rowing_gig_crewed(models_dir)
    if model_id == "coastal_steamer":
        return build_simple_glb(os.path.join(models_dir, "fleet3", "fleet_coastal_steamer.glb"), 50.0, 0, 26000, 90.0)
    if model_id == "pilot_cutter":
        return build_simple_glb(os.path.join(models_dir, "fleet3", "fleet_pilot_cutter.glb"), 18.0, 0, 26000, 90.0)
    if model_id == "harbour_tug":
        return build_simple_glb(os.path.join(models_dir, "fleet3", "fleet_harbour_tug.glb"), 22.0, 0, 26000, -90.0)
    if model_id == "steam_pinnace":
        return build_simple_glb(os.path.join(models_dir, "harbour_gen", "steam_pinnace.glb"), 15.0, 0, 25000, 90.0)
    if model_id == "town_tavern":
        return build_12_town_tavern(models_dir)
    if model_id == "town_foundry":
        return build_13_town_foundry(models_dir)
    if model_id == "steam_crane":
        return build_simple_glb(os.path.join(models_dir, "harbour_gen", "steam_crane.glb"), 18.5, 2, 25000, -90.0)
    if model_id == "coaling_station":
        return build_simple_glb(os.path.join(models_dir, "harbour_gen", "coaling_station.glb"), 24.0, 0, 26000, -90.0)
    if model_id == "windmill_stigsby":
        return build_16_windmill(models_dir)
    if model_id == "sailor_mariner":
        return build_17_sailor(models_dir)
    if model_id == "sea_gull":
        return build_18_sea_gull(models_dir)
    raise ValueError(f"Unknown model id: {model_id}")


# ------------------------------------------------------------------------------
# Optional <model-viewer camera-orbit="32deg 74deg auto"> QA Thumbnail Renderer
# ------------------------------------------------------------------------------
def render_mv_thumbnail(thumb_path, lo, hi, device_pref="OPTIX"):
    sc = bpy.context.scene
    sc.render.engine = "CYCLES"
    sc.render.resolution_x = 560
    sc.render.resolution_y = 420
    sc.render.resolution_percentage = 100
    sc.cycles.samples = 28
    sc.cycles.use_denoising = True
    sc.view_settings.view_transform = "AgX"
    sc.view_settings.exposure = 0.20

    if device_pref != "CPU":
        try:
            prefs = bpy.context.preferences.addons["cycles"].preferences
            prefs.compute_device_type = device_pref
            prefs.get_devices()
            has_gpu = False
            for d in prefs.devices:
                d.use = d.type != "CPU"
                if d.use:
                    has_gpu = True
            sc.cycles.device = "GPU" if has_gpu else "CPU"
        except Exception:
            sc.cycles.device = "CPU"
    else:
        sc.cycles.device = "CPU"

    w = bpy.data.worlds.new("ThumbWorld")
    w.use_nodes = True
    sc.world = w
    bg = w.node_tree.nodes["Background"]
    bg.inputs["Color"].default_value = (0.14, 0.17, 0.22, 1.0)
    bg.inputs["Strength"].default_value = 1.15

    ctr = (lo + hi) * 0.5
    diag = (hi - lo).length
    # Exact <model-viewer camera-orbit="32deg 74deg auto"> direction in Blender Z-up
    phi = math.radians(74.0)
    theta = math.radians(32.0)
    cam_dir = Vector(
        (
            math.sin(phi) * math.sin(theta),
            -math.sin(phi) * math.cos(theta),
            math.cos(phi),
        )
    ).normalized()

    cam_data = bpy.data.cameras.new("ThumbCam")
    cam_data.lens = 45.0
    cam_data.clip_start = max(0.005, diag * 0.005)
    cam_data.clip_end = max(100.0, diag * 20.0)
    cam = bpy.data.objects.new("ThumbCam", cam_data)
    sc.collection.objects.link(cam)
    sc.camera = cam
    cam.location = ctr + cam_dir * (diag * 1.55)
    cam.rotation_euler = (ctr - cam.location).to_track_quat("-Z", "Y").to_euler()

    key_data = bpy.data.lights.new("ThumbKey", "SUN")
    key_data.energy = 3.2
    key_data.color = (1.0, 0.95, 0.88)
    key_data.angle = math.radians(5.0)
    key = bpy.data.objects.new("ThumbKey", key_data)
    key.parent = cam
    key.rotation_euler = (math.radians(22.0), math.radians(-26.0), 0.0)
    sc.collection.objects.link(key)

    fill_data = bpy.data.lights.new("ThumbFill", "SUN")
    fill_data.energy = 1.1
    fill_data.color = (0.70, 0.82, 1.0)
    fill = bpy.data.objects.new("ThumbFill", fill_data)
    fill.parent = cam
    fill.rotation_euler = (math.radians(-12.0), math.radians(38.0), 0.0)
    sc.collection.objects.link(fill)

    sc.render.image_settings.file_format = "PNG"
    sc.render.filepath = thumb_path
    bpy.ops.render.render(write_still=True)


def main():
    args = parse_args()
    models_dir = os.path.abspath(args.models_dir)
    out_dir = os.path.abspath(args.out_dir)
    os.makedirs(out_dir, exist_ok=True)
    if args.thumbs_dir:
        os.makedirs(os.path.abspath(args.thumbs_dir), exist_ok=True)

    only_ids = {s.strip() for s in args.only.split(",") if s.strip()}
    selected = [
        (idx, spec)
        for idx, spec in enumerate(MODEL_SPECS)
        if (not only_ids or spec["id"] in only_ids) and (idx % args.num_tasks == args.task_idx)
    ]

    built_entries = []
    for idx, spec in selected:
        mid = spec["id"]
        reset_scene()
        yaw_deg = dispatch_build(mid, models_dir)
        enforce_web_materials()
        clamp_image_sizes(max_dim=2048)
        lo, hi = finalize_orientation_and_ground(yaw_deg)

        out_glb = os.path.join(out_dir, f"{mid}.glb")
        bpy.ops.export_scene.gltf(
            filepath=out_glb,
            export_format="GLB",
            export_yup=True,
            export_image_format="JPEG",
            export_jpeg_quality=88,
        )

        meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
        faces = sum(len(o.data.polygons) for o in meshes)
        tris = sum(len(p.vertices) - 2 for o in meshes for p in o.data.polygons)
        size_bytes = os.path.getsize(out_glb)
        dims = hi - lo  # Blender Z-up after -Y alignment: Y=length/depth, X=beam/width, Z=height
        len_m = round(float(dims.y), 2)
        beam_m = round(float(dims.x), 2)
        height_m = round(float(dims.z), 2)

        assert 15000 <= faces <= 38000, f"{mid}: face count {faces} outside 15k-38k target!"
        assert size_bytes < int(4.5 * 1024 * 1024), f"{mid}: size {size_bytes} exceeds 4.5 MB!"

        if args.thumbs_dir:
            thumb_p = os.path.join(os.path.abspath(args.thumbs_dir), f"{idx + 1:02d}_{mid}.png")
            render_mv_thumbnail(thumb_p, lo, hi, device_pref=args.device)

        entry = {
            "order": idx + 1,
            "id": mid,
            "filename": f"{mid}.glb",
            "title": spec["title"],
            "category": spec["category"],
            "description": spec["description"],
            "dimensions_m": {
                "length": len_m,
                "beam": beam_m,
                "height": height_m,
                "formatted": f"{len_m:.1f} x {beam_m:.1f} x {height_m:.1f} m",
            },
            "faces": faces,
            "triangles": tris,
            "size_bytes": size_bytes,
            "size_kb": round(size_bytes / 1024.0, 1),
            "unreal_role": spec["unreal_role"],
            "qa_score": spec["qa_score"],
        }
        built_entries.append(entry)
        print(
            f"WEB_GLB_OK {idx + 1:02d}/{len(MODEL_SPECS)} {mid:24s} "
            f"{entry['size_kb']:7.1f} KB  faces={faces:5d}  tris={tris:5d}  "
            f"dims={entry['dimensions_m']['formatted']}",
            flush=True,
        )

    # Write per-slice partial manifest if sliced, and full manifest.json when complete
    if args.num_tasks > 1 or only_ids:
        part_path = os.path.join(out_dir, f".manifest_part_{args.task_idx}.json")
        with open(part_path, "w", encoding="utf-8") as f:
            json.dump(built_entries, f, indent=2)

    manifest_path = os.path.join(out_dir, "manifest.json")
    existing_by_id = {}
    if os.path.isfile(manifest_path):
        try:
            prev = json.load(open(manifest_path, encoding="utf-8"))
            for item in prev.get("models", []):
                existing_by_id[item["id"]] = item
        except Exception:
            pass
    for fn in sorted(os.listdir(out_dir)):
        if fn.startswith(".manifest_part_") and fn.endswith(".json"):
            try:
                for item in json.load(open(os.path.join(out_dir, fn), encoding="utf-8")):
                    existing_by_id[item["id"]] = item
            except Exception:
                pass
    for item in built_entries:
        existing_by_id[item["id"]] = item

    ordered_models = [existing_by_id[s["id"]] for s in MODEL_SPECS if s["id"] in existing_by_id]
    manifest = {
        "project": "Ironwake",
        "count": len(ordered_models),
        "camera_orbit_default": "32deg 74deg auto",
        "gltf_axes": {"forward": "+Z", "up": "+Y"},
        "categories": ["ships", "aircraft", "architecture", "craft_crew"],
        "models": ordered_models,
    }
    with open(manifest_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)
    print(f"MANIFEST_OK count={len(ordered_models)} path={manifest_path}", flush=True)


if __name__ == "__main__":
    main()
