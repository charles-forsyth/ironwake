"""E1: import world assets (build/assets/p21/<Name>/<Name>.fbx + materials.json) into /Game/Ironwake/World/<Name>/.
Run headless: IWTEST_SCRIPT=iw_p21_import UnrealEditor IronwakeTest.uproject -log   (quits when done). Idempotent.
Every mesh gets Nanite; materials are MIs of the R1 masters M_IW_ShipTex2 / M_IW_ShipPaint.
"""
import unreal, os, json

proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(proj, "build/assets/p21")
ONLY = [s for s in os.environ.get("IWP21_ONLY", "").split(",") if s]
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
M_TEX = unreal.load_asset("/Game/Ironwake/Ships/Materials/M_IW_ShipTex2")
M_PAINT = unreal.load_asset("/Game/Ironwake/Ships/Materials/M_IW_ShipPaint")
WHITE_MASK = unreal.load_asset("/Game/Ironwake/Ships/Materials/T_IW_WhiteMask")
FLAT = unreal.load_asset("/Engine/EngineMaterials/FlatNormal")
report = {}


def import_tex(fn, dest, srgb=True, normal=False, maxsize=2048):
    name = "T_" + os.path.splitext(os.path.basename(fn))[0][2:]
    path = "%s/%s" % (dest, name)
    t = unreal.AssetImportTask()
    t.filename = fn; t.destination_path = dest; t.destination_name = name
    t.automated = True; t.save = True; t.replace_existing = True
    AT.import_asset_tasks([t])
    tex = unreal.load_asset(path)
    if tex:
        tex.set_editor_property("srgb", srgb)
        if normal: tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        elif not srgb: tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        tex.set_editor_property("max_texture_size", maxsize)
        EAL.save_asset(path)
    return tex


names = sorted(n for n in os.listdir(SRC) if os.path.isdir(os.path.join(SRC, n)))
if ONLY: names = [n for n in names if n in ONLY]
for name in names:
    d = os.path.join(SRC, name)
    fbx = os.path.join(d, name + ".fbx")
    if not os.path.exists(fbx): continue
    dest = "/Game/Ironwake/World/%s" % name
    t = unreal.AssetImportTask()
    t.filename = fbx; t.destination_path = dest; t.destination_name = "SM_" + name
    t.automated = True; t.save = True; t.replace_existing = True
    o = unreal.FbxImportUI()
    o.import_mesh = True; o.import_as_skeletal = False
    o.import_materials = False; o.import_textures = False
    o.static_mesh_import_data.combine_meshes = True
    o.static_mesh_import_data.remove_degenerates = False
    o.static_mesh_import_data.generate_lightmap_u_vs = False
    o.static_mesh_import_data.auto_generate_collision = False
    o.static_mesh_import_data.build_nanite = True
    o.static_mesh_import_data.set_editor_property("import_uniform_scale", 1.0)
    t.options = o
    AT.import_asset_tasks([t])
    path = "%s/SM_%s" % (dest, name)
    m = unreal.load_asset(path)
    if not m:
        unreal.log_error("IWP21 import failed %s" % path); continue
    ispart = ("_prop_" in name or "_sails" in name or "_oar_" in name)
    ns = sub.get_nanite_settings(m); ns.enabled = not ispart; sub.set_nanite_settings(m, ns, True)
    try:
        sub.remove_collisions(m)
        if not ispart: sub.add_simple_collisions(m, unreal.ScriptCollisionShapeType.BOX if name.startswith("Town") else unreal.ScriptCollisionShapeType.NDOP26)
    except Exception as ex: unreal.log_warning("IWP21 collision %s" % ex)
    # materials
    mats = json.load(open(os.path.join(d, "materials.json")))
    texmax = 4096 if name == "BrassAirshipRemesh" else 2048
    mis = {}
    for mn, e in mats.items():
        safe = "MI_%s_%s" % (name, mn.replace(".", "_").replace(" ", "_").replace("#", ""))
        mp = "%s/%s" % (dest, safe)
        mi = unreal.load_asset(mp) if EAL.does_asset_exist(mp) else \
            AT.create_asset(safe, dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        textured = 'base' in e
        MEL.set_material_instance_parent(mi, M_TEX if textured else M_PAINT)
        c = e["color"]
        MEL.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(c[0], c[1], c[2], 1))
        if textured:
            MEL.set_material_instance_texture_parameter_value(mi, "BaseTex", import_tex(e['base'], dest, True, False, texmax))
            if 'mr' in e:
                MEL.set_material_instance_texture_parameter_value(mi, "MRTex", import_tex(e['mr'], dest, False, False, texmax))
                MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", 1.0)
                MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", e.get('metal_override', 1.0))
            else:
                MEL.set_material_instance_texture_parameter_value(mi, "MRTex", WHITE_MASK)
                MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", e['rough'])
                MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", min(e['metal'], 0.6))
            nt = import_tex(e['normal'], dest, False, True, texmax) if 'normal' in e else FLAT
            MEL.set_material_instance_texture_parameter_value(mi, "NormalTex", nt)
        else:
            MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", e['rough'])
            MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", min(e['metal'], 0.6))
        EAL.save_asset(mp)
        mis[mn] = mi
    sm = m.get_editor_property("static_materials")
    n = 0
    for i, s in enumerate(sm):
        key = str(s.get_editor_property("material_slot_name"))
        mi = mis.get(key) or next((v for k, v in mis.items() if k.replace(".", "_").replace(" ", "_") == key or key.startswith(k.replace(".", "_"))), None)
        if mi is None and len(mis) == 1: mi = list(mis.values())[0]
        if mi: m.set_material(i, mi); n += 1
        else: unreal.log_warning("IWP21 no MI for slot %s on %s (have %s)" % (key, name, list(mis)))
    EAL.save_asset(path)
    b = m.get_bounding_box()
    report[name] = {"tris": m.get_num_triangles(0), "nanite": bool(sub.get_nanite_settings(m).enabled), "slots": "%d/%d" % (n, len(sm)),
                    "min": [round(b.min.x), round(b.min.y), round(b.min.z)], "max": [round(b.max.x), round(b.max.y), round(b.max.z)]}
    unreal.log("IWP21 %s %s" % (name, report[name]))
    EAL.save_directory(dest)

with open(os.path.join(proj, "Saved/iw_p21_import.json"), "w") as f: json.dump(report, f, indent=1)
unreal.log("IWP21 DONE %d meshes" % len(report))
unreal.SystemLibrary.quit_editor()
