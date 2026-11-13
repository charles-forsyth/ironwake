"""R1: import library ship FBX parts into /Game/Ironwake/Ships/<Name>/ (headless editor Python).
Run: IWTEST_SCRIPT=iw_r1_import UnrealEditor IronwakeTest.uproject -log   (quits when done)
Idempotent: replace_existing. Hull meshes get Nanite; moving parts stay classic (they're small).
"""
import unreal, os, json

proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(proj, "build/assets/r1")
SHIPS = os.environ.get("IW_SHIPS").split(",") if os.environ.get("IW_SHIPS") else ["Devastation", "Kracken", "Goliath", "Aglae", "BrassAirship"]
sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
report = {}

for ship in SHIPS:
    d = os.path.join(SRC, ship)
    if not os.path.isdir(d):
        unreal.log_warning("IWR1 missing %s" % d); continue
    dest = "/Game/Ironwake/Ships/%s" % ship
    tasks = []
    for fn in sorted(os.listdir(d)):
        if not fn.endswith(".fbx"): continue
        part = fn[len(ship) + 1:-4]
        t = unreal.AssetImportTask()
        t.filename = os.path.join(d, fn)
        t.destination_path = dest
        t.destination_name = "SM_%s_%s" % (ship, part)
        t.automated = True; t.save = True; t.replace_existing = True
        o = unreal.FbxImportUI()
        o.import_mesh = True; o.import_as_skeletal = False
        o.import_materials = True; o.import_textures = True
        o.static_mesh_import_data.combine_meshes = True
        o.static_mesh_import_data.remove_degenerates = False
        o.static_mesh_import_data.generate_lightmap_u_vs = False
        o.static_mesh_import_data.auto_generate_collision = False
        o.static_mesh_import_data.build_nanite = (part == "Hull")
        o.static_mesh_import_data.set_editor_property("import_uniform_scale", 1.0)
        t.options = o
        tasks.append((part, t))
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t for _, t in tasks])
    for part, t in tasks:
        path = "%s/%s" % (dest, t.destination_name)
        m = unreal.load_asset(path)
        if not m:
            unreal.log_error("IWR1 import failed %s" % path); continue
        if part == "Hull":
            ns = sub.get_nanite_settings(m)
            ns.enabled = True
            ns.set_editor_property("fallback_relative_error", 1.0) if hasattr(ns, "fallback_relative_error") else None
            sub.set_nanite_settings(m, ns, True)
        b = m.get_bounding_box()
        report["%s_%s" % (ship, part)] = {
            "tris": m.get_num_triangles(0), "nanite": bool(sub.get_nanite_settings(m).enabled),
            "min": [round(b.min.x), round(b.min.y), round(b.min.z)], "max": [round(b.max.x), round(b.max.y), round(b.max.z)],
            "mats": m.get_num_sections(0)}
        unreal.EditorAssetLibrary.save_asset(path)
    unreal.EditorAssetLibrary.save_directory(dest)

out = os.path.join(proj, "Saved/iw_r1_import.json")
with open(out, "w") as f: json.dump(report, f, indent=1)
for k, v in report.items(): unreal.log("IWR1 %s %s" % (k, v))
unreal.log("IWR1 DONE %d meshes" % len(report))
if not os.environ.get("IW_NOQUIT"): unreal.SystemLibrary.quit_editor()
