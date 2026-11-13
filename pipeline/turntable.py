# Blender headless: 4K Cycles turntable stills + key-art for one model.
# blender -b -P turntable.py -- <in.glb|gltf> <outdir> <frames> <res_x> <res_y> <samples>
import bpy, sys, os, math
from mathutils import Vector
a = sys.argv[sys.argv.index("--") + 1:]
src, outdir = a[0], a[1]
frames, rx, ry, spp = int(a[2]), int(a[3]), int(a[4]), int(a[5])
name = os.path.splitext(os.path.basename(src))[0].replace("_game", "")
os.makedirs(outdir, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
meshes = [o for o in bpy.data.objects if o.type == "MESH"]
pts = []
for o in meshes:
    vs = o.data.vertices; step = max(1, len(vs) // 300)
    for i in range(0, len(vs), step):
        pts.append(o.matrix_world @ vs[i].co)
def pct(v, q): s = sorted(v); return s[int(q * (len(s) - 1))]
lo = Vector([pct([p[i] for p in pts], 0.01) for i in range(3)]); hi = Vector([pct([p[i] for p in pts], 0.99) for i in range(3)])
c = (lo + hi) / 2; size = (hi - lo).length
sc = bpy.context.scene
sc.render.engine = "CYCLES"; sc.cycles.device = "CPU"; sc.cycles.samples = spp; sc.cycles.use_denoising = True
sc.render.resolution_x, sc.render.resolution_y = rx, ry
sc.view_settings.view_transform = "AgX"; sc.view_settings.look = "AgX - Medium High Contrast"
w = bpy.data.worlds.new("w"); sc.world = w; w.use_nodes = True
nt = w.node_tree; sky = nt.nodes.new("ShaderNodeTexSky")
try:
    sky.sky_type = "NISHITA"; sky.sun_elevation = math.radians(18); sky.sun_rotation = math.radians(40); sky.air_density = 2.5; sky.dust_density = 4
except Exception:
    pass
nt.links.new(sky.outputs[0], nt.nodes["Background"].inputs[0]); nt.nodes["Background"].inputs[1].default_value = 0.22
sc.view_settings.exposure = -1.3
sun = bpy.data.objects.new("sun", bpy.data.lights.new("sun", "SUN")); sun.data.energy = 2.2; sun.data.angle = math.radians(2)
sun.rotation_euler = (math.radians(65), 0, math.radians(40)); sc.collection.objects.link(sun)
# dark wet ground plane as a sea stand-in
bpy.ops.mesh.primitive_plane_add(size=size * 30, location=(c.x, c.y, lo.z + (hi.z - lo.z) * 0.12))
pl = bpy.context.object; m = bpy.data.materials.new("sea"); m.use_nodes = True
b = m.node_tree.nodes["Principled BSDF"]; b.inputs["Base Color"].default_value = (0.012, 0.02, 0.025, 1); b.inputs["Roughness"].default_value = 0.18
nz = m.node_tree.nodes.new("ShaderNodeTexNoise"); nz.inputs["Scale"].default_value = 400.0 / max(size, 1)
bm = m.node_tree.nodes.new("ShaderNodeBump"); bm.inputs["Strength"].default_value = 0.25
m.node_tree.links.new(nz.outputs["Fac"], bm.inputs["Height"]); m.node_tree.links.new(bm.outputs["Normal"], b.inputs["Normal"])
vol = bpy.data.materials.new("haze"); vol.use_nodes = True; vn = vol.node_tree.nodes
vn.remove(vn["Principled BSDF"]); pv = vn.new("ShaderNodeVolumePrincipled"); pv.inputs["Density"].default_value = 0.6 / max(size * 10, 1)
vol.node_tree.links.new(pv.outputs[0], vn["Material Output"].inputs["Volume"])
bpy.ops.mesh.primitive_cube_add(size=size * 25, location=c); hz = bpy.context.object; hz.data.materials.append(vol)
pl.data.materials.append(m)
cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam")); sc.collection.objects.link(cam); sc.camera = cam
cam.data.lens = 70; cam.data.clip_start = size * 0.001; cam.data.clip_end = size * 50
cam.data.dof.use_dof = True; cam.data.dof.aperture_fstop = 11
def shoot(tag, az, el, dist):
    d = Vector((math.cos(math.radians(az)) * math.cos(math.radians(el)), math.sin(math.radians(az)) * math.cos(math.radians(el)), math.sin(math.radians(el))))
    cam.location = c + d * size * dist
    cam.rotation_euler = (c - cam.location).to_track_quat("-Z", "Y").to_euler()
    cam.data.dof.focus_distance = (c - cam.location).length
    sc.render.filepath = os.path.join(outdir, f"{name}_{tag}.png")
    bpy.ops.render.render(write_still=True)
for i in range(frames):
    shoot(f"turn{i:02d}", -90 + i * 360 / frames, 12, 1.6)
if os.environ.get("HERO_RES"):
    hx, hy = map(int, os.environ["HERO_RES"].split("x")); sc.render.resolution_x, sc.render.resolution_y = hx, hy
    sc.cycles.samples = int(os.environ.get("HERO_SPP", spp))
shoot("hero_low", -35, 5, 1.45)      # key art: low dramatic three-quarter
shoot("hero_high", 140, 28, 1.6)     # key art: high stern quarter
print("TT_DONE", name)
