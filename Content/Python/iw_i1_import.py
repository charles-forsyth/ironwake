"""I1: island assets (copy of iw_e1_import; build/assets/i1/ + leaf alpha maps -> M_IW_Foliage). E1 original: import world assets (build/assets/e1/<Name>/<Name>.fbx + materials.json) into /Game/Ironwake/World/<Name>/.
Run headless: IWTEST_SCRIPT=iw_e1_import UnrealEditor IronwakeTest.uproject -log   (quits when done). Idempotent.
Every mesh gets Nanite; materials are MIs of the R1 masters M_IW_ShipTex2 / M_IW_ShipPaint.
"""
import unreal, os, json

proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(proj, "build/assets/i1")
ALPHA = os.path.join(proj, "build/i1/alpha")
ONLY = [s for s in os.environ.get("IWI1_ONLY", "").split(",") if s]
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
M_TEX = unreal.load_asset("/Game/Ironwake/Ships/Materials/M_IW_ShipTex2")
M_PAINT = unreal.load_asset("/Game/Ironwake/Ships/Materials/M_IW_ShipPaint")
WHITE_MASK = unreal.load_asset("/Game/Ironwake/Ships/Materials/T_IW_WhiteMask")
FLAT = unreal.load_asset("/Engine/EngineMaterials/FlatNormal")
FOL_PATH = "/Game/Ironwake/Islands/M_IW_Foliage"


def foliage_master():
    if EAL.does_asset_exist(FOL_PATH):
        return unreal.load_asset(FOL_PATH)
    m = AT.create_asset("M_IW_Foliage", "/Game/Ironwake/Islands", unreal.Material, unreal.MaterialFactoryNew())
    col = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -600, -200)
    col.set_editor_property("parameter_name", "Color"); col.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    tex = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -900, -300)
    tex.set_editor_property("parameter_name", "BaseTex"); tex.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
    mul = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, -250)
    MEL.connect_material_expressions(tex, "RGB", mul, "A"); MEL.connect_material_expressions(col, "", mul, "B")
    MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
    al = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -900, 0)
    al.set_editor_property("parameter_name", "AlphaTex"); al.set_editor_property("texture", WHITE_MASK)
    al.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    MEL.connect_material_property(al, "R", unreal.MaterialProperty.MP_OPACITY_MASK)
    r = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -300, 100)
    r.set_editor_property("parameter_name", "Roughness"); r.set_editor_property("default_value", 0.8)
    MEL.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    nt = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -900, 400)
    nt.set_editor_property("parameter_name", "NormalTex"); nt.set_editor_property("texture", FLAT)
    nt.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    MEL.connect_material_property(nt, "RGB", unreal.MaterialProperty.MP_NORMAL)
    ss = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 250)
    k = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -500, 300); k.set_editor_property("r", 0.6)
    MEL.connect_material_expressions(mul, "", ss, "A"); MEL.connect_material_expressions(k, "", ss, "B")
    MEL.connect_material_property(ss, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property("opacity_mask_clip_value", 0.4)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    m.set_editor_property("used_with_nanite", True)
    MEL.recompile_material(m)
    EAL.save_asset(FOL_PATH)
    return m


M_FOL = foliage_master()


def alpha_for(mat_name):
    if not os.path.isdir(ALPHA): return None
    key = mat_name.lower()
    for fn in sorted(os.listdir(ALPHA)):
        a, part = fn.rsplit(".", 1)[0].split("__")
        part = part.lower().replace("_alpha", "").replace("alpha", "")
        if key.startswith(a) and (part == "" or part in key): return os.path.join(ALPHA, fn)
    return None
report = {}


def import_tex(fn, dest, srgb=True, normal=False, maxsize=2048):
    b = os.path.splitext(os.path.basename(fn))[0]
    name = "T_" + (b[2:] if b.startswith("T_") else b.replace("__", "_"))
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
        unreal.log_error("IWI1 import failed %s" % path); continue
    ns = sub.get_nanite_settings(m); ns.enabled = True; sub.set_nanite_settings(m, ns, True)
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
        af = alpha_for(mn) if e.get('alpha') else None
        MEL.set_material_instance_parent(mi, M_FOL if af else (M_TEX if textured else M_PAINT))
        if af:
            MEL.set_material_instance_texture_parameter_value(mi, "AlphaTex", import_tex(af, dest, False, False, 2048))
            unreal.log("IWI1 foliage MI %s alpha %s" % (safe, os.path.basename(af)))
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
        else: unreal.log_warning("IWI1 no MI for slot %s on %s (have %s)" % (key, name, list(mis)))
    EAL.save_asset(path)
    b = m.get_bounding_box()
    report[name] = {"tris": m.get_num_triangles(0), "nanite": bool(sub.get_nanite_settings(m).enabled), "slots": "%d/%d" % (n, len(sm)),
                    "min": [round(b.min.x), round(b.min.y), round(b.min.z)], "max": [round(b.max.x), round(b.max.y), round(b.max.z)]}
    unreal.log("IWI1 %s %s" % (name, report[name]))
    EAL.save_directory(dest)

with open(os.path.join(proj, "Saved/iw_i1_import.json"), "w") as f: json.dump(report, f, indent=1)
unreal.log("IWI1 DONE %d meshes" % len(report))
unreal.SystemLibrary.quit_editor()
