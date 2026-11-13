#!/usr/bin/env python3
"""Render one articulated-rig vignette from an Ironwake web GLB (headless Blender, Cycles).

Usage:
  blender -b --factory-startup -P render_rig_reel.py -- \
    --models DIR --out DIR --vignette brig|paddle|tender|airship \
    [--frames 120] [--rx 1280] [--ry 720] [--samples 16] [--device CPU|OPTIX] [--start 0] [--end -1]

Each vignette is 4 s at 30 fps: a slow 40 degree camera arc (gentle, no shake) while the
model's PART_* nodes animate about their pivots: yards brace, paddle wheels turn,
the crane slews with its bell swinging, propellers spin.
"""
import argparse
import math
import os
import sys

import bpy
from mathutils import Matrix, Vector

VIGNETTES = {
    "brig": "tall_brig_rigged.glb",
    "paddle": "paddle_steamer.glb",
    "tender": "salvage_tender.glb",
    "airship": "patrol_airship.glb",
}


def args():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser()
    p.add_argument("--models", required=True)
    p.add_argument("--out", required=True)
    p.add_argument("--vignette", required=True, choices=sorted(VIGNETTES))
    p.add_argument("--frames", type=int, default=120)
    p.add_argument("--rx", type=int, default=1280)
    p.add_argument("--ry", type=int, default=720)
    p.add_argument("--samples", type=int, default=16)
    p.add_argument("--device", default="CPU")
    p.add_argument("--start", type=int, default=0)
    p.add_argument("--end", type=int, default=-1)
    p.add_argument("--build-only", action="store_true")
    return p.parse_args(argv)


def world_bbox(objs):
    lo = Vector((1e9, 1e9, 1e9))
    hi = Vector((-1e9, -1e9, -1e9))
    for o in objs:
        for c in o.bound_box:
            w = o.matrix_world @ Vector(c)
            lo = Vector(map(min, lo, w))
            hi = Vector(map(max, hi, w))
    return lo, hi


def setup_render(a, sc):
    sc.render.engine = "CYCLES"
    sc.cycles.samples = a.samples
    sc.cycles.use_denoising = True
    sc.render.resolution_x = a.rx
    sc.render.resolution_y = a.ry
    sc.render.resolution_percentage = 100
    sc.render.fps = 30
    sc.view_settings.view_transform = "AgX"
    sc.view_settings.look = "AgX - Medium High Contrast"
    sc.cycles.max_bounces = 4
    sc.render.film_transparent = False
    sc.render.image_settings.file_format = "PNG"
    sc.cycles.device = "CPU"
    if a.device != "CPU":
        try:
            prefs = bpy.context.preferences.addons["cycles"].preferences
            prefs.compute_device_type = a.device
            prefs.get_devices()
            gpu = False
            for d in prefs.devices:
                d.use = d.type != "CPU"
                gpu = gpu or d.use
            sc.cycles.device = "GPU" if gpu else "CPU"
        except Exception:
            sc.cycles.device = "CPU"
    print("DEVICE", sc.cycles.device, flush=True)


def studio(sc, lo, hi):
    """Seamless dark-slate studio: constant world, a huge matte floor that fades into it."""
    w = bpy.data.worlds.new("Slate")
    sc.world = w
    w.use_nodes = True
    bg = w.node_tree.nodes["Background"]
    bg.inputs["Color"].default_value = (0.035, 0.05, 0.062, 1)
    bg.inputs["Strength"].default_value = 1.0
    sc.render.use_motion_blur = False

    # Cyclorama sweep: a floor that curves up into a wall all the way round, so no horizon line
    # ever shows, whatever the camera height. Built as a lathe: a bowl with a flat base.
    import bmesh
    R = (hi - lo).length * 3.0
    flat = R * 0.55
    bm = bmesh.new()
    prof = []
    for i in range(24):
        t = i / 23.0
        if t < 0.4:
            prof.append((flat * t / 0.4, 0.0))
        else:
            u = (t - 0.4) / 0.6
            ang = u * math.pi / 2
            prof.append((flat + (R - flat) * math.sin(ang), (R - flat) * (1 - math.cos(ang)) * 1.6))
    segs = 72
    rings = []
    for (r, z) in prof:
        ring = [bm.verts.new((r * math.cos(2 * math.pi * k / segs), r * math.sin(2 * math.pi * k / segs), lo.z - 0.02 + z)) for k in range(segs)]
        rings.append(ring)
    for a_, b_ in zip(rings[:-1], rings[1:]):
        for k in range(segs):
            bm.faces.new((a_[k], a_[(k + 1) % segs], b_[(k + 1) % segs], b_[k]))
    me = bpy.data.meshes.new("Cyc")
    bm.to_mesh(me)
    bm.free()
    for poly in me.polygons:
        poly.use_smooth = True
    floor = bpy.data.objects.new("Cyc", me)
    sc.collection.objects.link(floor)
    m = bpy.data.materials.new("SlateFloor")
    m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (0.035, 0.05, 0.062, 1)
    b.inputs["Roughness"].default_value = 0.55
    floor.data.materials.append(m)

    key = bpy.data.lights.new("Key", "SUN")
    key.energy = 3.6
    key.angle = math.radians(6)
    key.color = (1.0, 0.92, 0.8)
    ko = bpy.data.objects.new("Key", key)
    ko.rotation_euler = (math.radians(50), 0, math.radians(-35))
    sc.collection.objects.link(ko)
    fill = bpy.data.lights.new("Fill", "SUN")
    fill.energy = 0.9
    fill.color = (0.7, 0.82, 1.0)
    fo = bpy.data.objects.new("Fill", fill)
    fo.rotation_euler = (math.radians(65), 0, math.radians(120))
    sc.collection.objects.link(fo)
    rim = bpy.data.lights.new("Rim", "SUN")
    rim.energy = 2.2
    rim.color = (0.62, 0.8, 1.0)
    ro = bpy.data.objects.new("Rim", rim)
    ro.rotation_euler = (math.radians(75), 0, math.radians(165))
    sc.collection.objects.link(ro)


def key_rot(obj, frame, axis, angle_rad, base):
    """Rotate obj about its own origin by angle around a WORLD axis, on top of base matrix."""
    loc = base.to_translation()
    r = Matrix.Rotation(angle_rad, 4, Vector(axis))
    obj.matrix_world = Matrix.Translation(loc) @ r @ Matrix.Translation(-loc) @ base
    obj.keyframe_insert("location", frame=frame)
    obj.keyframe_insert("rotation_quaternion", frame=frame)


def main():
    a = args()
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    sc = bpy.context.scene
    bpy.ops.import_scene.gltf(filepath=os.path.join(a.models, VIGNETTES[a.vignette]))
    meshes = [o for o in sc.objects if o.type == "MESH"]
    for o in meshes:
        o.rotation_mode = "QUATERNION"
    parts = {o.name: o for o in meshes if o.name.startswith("PART_")}
    print("PARTS", sorted(parts), flush=True)
    bpy.context.view_layer.update()
    base = {n: o.matrix_world.copy() for n, o in parts.items()}
    lo, hi = world_bbox(meshes)
    ctr = (lo + hi) * 0.5
    span = (hi - lo).length
    N = a.frames
    sc.frame_start, sc.frame_end = 0, N - 1

    for f in range(N):
        t = f / float(N)                       # 0..1 over the 4 s vignette
        s = math.sin(2 * math.pi * t)          # one smooth cycle, loops cleanly
        if a.vignette == "brig":
            for n, o in parts.items():
                if "yard" in n or "sail_m" in n:
                    key_rot(o, f, (0, 0, 1), math.radians(22) * s, base[n])
                elif "spanker" in n:
                    key_rot(o, f, (0, 0, 1), math.radians(-18) * s, base[n])
                else:
                    key_rot(o, f, (0, 0, 1), math.radians(6) * s, base[n])
        elif a.vignette == "paddle":
            for n, o in parts.items():
                key_rot(o, f, (1, 0, 0), -2 * math.pi * t * 1.5, base[n])  # 1.5 turns
        elif a.vignette == "tender":
            slew = math.radians(28) * s
            crane = parts.get("PART_crane")
            bell = parts.get("PART_bell")
            if crane:
                key_rot(crane, f, (0, 0, 1), slew, base["PART_crane"])
            if bell and crane:
                # Bell rides the crane's slew about the crane pivot, plus a lagged pendulum swing.
                piv = base["PART_crane"].to_translation()
                r = Matrix.Rotation(slew, 4, "Z")
                carried = Matrix.Translation(piv) @ r @ Matrix.Translation(-piv) @ base["PART_bell"]
                swing = math.radians(7) * math.sin(2 * math.pi * t - 0.9)
                key_rot(bell, f, (1, 0, 0), swing, carried)
        elif a.vignette == "airship":
            for n, o in parts.items():
                key_rot(o, f, (0, 1, 0), 2 * math.pi * t * 6, base[n])  # 6 turns in 4 s

    for o in parts.values():
        if o.animation_data and o.animation_data.action:
            for fc in o.animation_data.action.fcurves if hasattr(o.animation_data.action, "fcurves") else []:
                for kp in fc.keyframe_points:
                    kp.interpolation = "LINEAR"

    studio(sc, lo, hi)
    cam_d = bpy.data.cameras.new("Cam")
    cam_d.lens = 40
    cam_d.clip_end = span * 50
    cam = bpy.data.objects.new("Cam", cam_d)
    sc.collection.objects.link(cam)
    sc.camera = cam
    # Per-vignette framing: the airship floats high (look at its true centre), the tender is short and deep.
    dist = span * {"brig": 1.18, "paddle": 1.1, "tender": 1.45, "airship": 1.25}[a.vignette]
    elev = math.radians({"airship": 10}.get(a.vignette, 18))
    for f in range(N):
        t = f / float(N - 1)
        ease = 0.5 - 0.5 * math.cos(math.pi * t)
        az = math.radians(200 + 32 * ease)    # slow 32 degree arc on the bow quarter, never head-on
        p = ctr + Vector((math.cos(az) * math.cos(elev), math.sin(az) * math.cos(elev), math.sin(elev))) * dist
        cam.location = p
        cam.rotation_euler = (ctr - p).to_track_quat("-Z", "Y").to_euler()
        cam.keyframe_insert("location", frame=f)
        cam.keyframe_insert("rotation_euler", frame=f)

    setup_render(a, sc)
    os.makedirs(a.out, exist_ok=True)
    print("BUILD_OK", a.vignette, "span", round(span, 1), flush=True)
    if a.build_only:
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(a.out, f"{a.vignette}.blend"))
        return
    end = N - 1 if a.end < 0 else min(a.end, N - 1)
    for f in range(a.start, end + 1):
        fp = os.path.join(a.out, f"{a.vignette}_{f:04d}.png")
        if os.path.exists(fp):
            continue
        sc.frame_set(f)
        sc.render.filepath = fp
        bpy.ops.render.render(write_still=True)
        print("FRAME_DONE", a.vignette, f, flush=True)
    print("RENDER_OK", a.vignette, flush=True)


main()
