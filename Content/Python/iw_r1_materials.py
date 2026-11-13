"""R1 materials: master M_IW_ShipPaint (vector colour) + M_IW_ShipTex (base texture * tint, ORM-ish metal/rough maps optional),
then one MaterialInstanceConstant per mesh slot from build/assets/r1/<Ship>/materials.json, assigned to every SM_<Ship>_* mesh.
Run headless: IWTEST_SCRIPT=iw_r1_materials UnrealEditor IronwakeTest.uproject -log
"""
import unreal, os, json

proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(proj, "build/assets/r1")
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
BASE = "/Game/Ironwake/Ships/Materials"
SHIPS = os.environ.get("IW_SHIPS").split(",") if os.environ.get("IW_SHIPS") else ["Devastation", "Kracken", "Goliath", "Aglae", "BrassAirship"]


def master(name, textured):
    path = "%s/%s" % (BASE, name)
    if EAL.does_asset_exist(path):
        return unreal.load_asset(path)
    m = AT.create_asset(name, BASE, unreal.Material, unreal.MaterialFactoryNew())
    col = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -600, -200)
    col.set_editor_property("parameter_name", "Color"); col.set_editor_property("default_value", unreal.LinearColor(0.1, 0.1, 0.11, 1))
    rough = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -600, 100)
    rough.set_editor_property("parameter_name", "Roughness"); rough.set_editor_property("default_value", 0.6)
    metal = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -600, 250)
    metal.set_editor_property("parameter_name", "Metallic"); metal.set_editor_property("default_value", 0.0)
    if textured:
        tex = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -900, -300)
        tex.set_editor_property("parameter_name", "BaseTex")
        tex.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
        mul = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, -250)
        MEL.connect_material_expressions(tex, "RGB", mul, "A")
        MEL.connect_material_expressions(col, "", mul, "B")
        MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
        # metal/rough map (glTF: G = roughness, B = metallic), blended with scalar via multiply
        mr = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -900, 200)
        mr.set_editor_property("parameter_name", "MRTex")
        mr.set_editor_property("texture", WHITE_MASK)
        mr.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        mr_r = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 100)
        MEL.connect_material_expressions(mr, "G", mr_r, "A"); MEL.connect_material_expressions(rough, "", mr_r, "B")
        mr_m = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 250)
        MEL.connect_material_expressions(mr, "B", mr_m, "A"); MEL.connect_material_expressions(metal, "", mr_m, "B")
        MEL.connect_material_property(mr_r, "", unreal.MaterialProperty.MP_ROUGHNESS)
        MEL.connect_material_property(mr_m, "", unreal.MaterialProperty.MP_METALLIC)
        nt = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -900, 500)
        nt.set_editor_property("parameter_name", "NormalTex")
        nt.set_editor_property("texture", unreal.load_asset("/Engine/EngineMaterials/FlatNormal"))
        nt.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        MEL.connect_material_property(nt, "RGB", unreal.MaterialProperty.MP_NORMAL)
    else:
        MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
        MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
        MEL.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    m.set_editor_property("used_with_nanite", True)
    m.set_editor_property("two_sided", True)   # SketchUp exports have single faces seen from both sides
    MEL.recompile_material(m)
    EAL.save_asset(path)
    return m


def import_tex(fn, dest, srgb=True, normal=False):
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
        tex.set_editor_property("max_texture_size", 2048)
        EAL.save_asset(path)
    return tex


# shared linear white mask (default for MRTex): imported with TC_Masks so the Masks sampler compiles
WHITE_MASK = None
M_PAINT = master("M_IW_ShipPaint", False)
WHITE_MASK = import_tex(os.path.join(proj, "build/assets/common/T_IW_WhiteMask.png"), BASE, False)
M_TEX = master("M_IW_ShipTex2", True)   # v2: sampler defaults fixed (v1 failed on Metal: default-texture sampler mismatch)
log = {}
for ship in SHIPS:
    jf = os.path.join(SRC, ship, "materials.json")
    if not os.path.exists(jf): continue
    mats = json.load(open(jf))
    dest = "/Game/Ironwake/Ships/%s" % ship
    mis = {}
    for name, e in mats.items():
        safe = "MI_%s_%s" % (ship, name.replace(".", "_"))
        path = "%s/%s" % (dest, safe)
        mi = unreal.load_asset(path) if EAL.does_asset_exist(path) else \
            AT.create_asset(safe, dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        textured = 'base' in e
        MEL.set_material_instance_parent(mi, M_TEX if textured else M_PAINT)
        c = e["color"]
        MEL.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(c[0], c[1], c[2], 1))
        if textured:
            MEL.set_material_instance_texture_parameter_value(mi, "BaseTex", import_tex(e['base'], dest, True))
            if 'mr' in e:
                MEL.set_material_instance_texture_parameter_value(mi, "MRTex", import_tex(e['mr'], dest, False))
                MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", 1.0)
                MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", 1.0)
            else:
                MEL.set_material_instance_texture_parameter_value(mi, "MRTex", WHITE_MASK)
                MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", e['rough'] / 1.0)
                MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", 0.0)
            if 'normal' in e:
                MEL.set_material_instance_texture_parameter_value(mi, "NormalTex", import_tex(e['normal'], dest, False, True))
            else:
                MEL.set_material_instance_texture_parameter_value(mi, "NormalTex", unreal.load_asset("/Engine/EngineMaterials/FlatNormal"))
        else:
            MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", e['rough'])
            MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", e['metal'])
        EAL.save_asset(path)
        mis[name] = mi
    # assign by slot name to every mesh of this ship
    for ap in EAL.list_assets(dest, recursive=False):
        a = unreal.load_asset(ap.split(".")[0])
        if not isinstance(a, unreal.StaticMesh): continue
        sm = a.get_editor_property("static_materials")
        n = 0
        for i, s in enumerate(sm):
            key = str(s.get_editor_property("material_slot_name"))
            mi = mis.get(key) or mis.get(key.replace("_", ".", 1)) or next((v for k, v in mis.items() if k.replace(".", "_") == key), None)
            if mi:
                a.set_material(i, mi); n += 1
            else:
                unreal.log_warning("IWR1MAT no MI for slot %s on %s" % (key, a.get_name()))
        EAL.save_asset(ap.split(".")[0])
        log[a.get_name()] = "%d/%d slots" % (n, len(sm))
for k, v in log.items(): unreal.log("IWR1MAT %s %s" % (k, v))
unreal.log("IWR1MAT DONE")
if not os.environ.get("IW_NOQUIT"): unreal.SystemLibrary.quit_editor()
