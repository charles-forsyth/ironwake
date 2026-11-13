"""P26: import TallBrig + 17 moving rig parts (build/assets/p26/<Name>/<Name>.fbx + materials.json) into /Game/Ironwake/World/<Name>/.
Also creates/updates /Game/Ironwake/Ships/Materials/M_IW_SailCanvas (Metal-SM6-safe WPO billow + luffing ripple, zero Clamp nodes).
Run headless: IWTEST_SCRIPT=iw_p26_import UnrealEditor IronwakeTest.uproject -log   (quits when done). Idempotent.
"""
import unreal, os, json

proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(proj, "build/assets/p26")
ONLY = [s for s in os.environ.get("IWP26_ONLY", "").split(",") if s]
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
M_TEX = unreal.load_asset("/Game/Ironwake/Ships/Materials/M_IW_ShipTex2")
M_PAINT = unreal.load_asset("/Game/Ironwake/Ships/Materials/M_IW_ShipPaint")
WHITE_MASK = unreal.load_asset("/Game/Ironwake/Ships/Materials/T_IW_WhiteMask")
FLAT = unreal.load_asset("/Engine/EngineMaterials/FlatNormal")


def build_brig_masters():
    """Matte marine timber/paint masters with explicit Specular=0.22, Metallic=0.0 so hulls & masts never look like chrome."""
    base_dir = "/Game/Ironwake/Ships/Materials"

    def make_one(asset_name, textured):
        path = base_dir + "/" + asset_name
        m = unreal.load_asset(path) if EAL.does_asset_exist(path) else \
            AT.create_asset(asset_name, base_dir, unreal.Material, unreal.MaterialFactoryNew())
        MEL.delete_all_material_expressions(m)
        col = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -700, -240)
        col.set_editor_property("parameter_name", "Color")
        col.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
        rough = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -700, 60)
        rough.set_editor_property("parameter_name", "Roughness")
        rough.set_editor_property("default_value", 0.74)
        metal = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -700, 200)
        metal.set_editor_property("parameter_name", "Metallic")
        metal.set_editor_property("default_value", 0.0)
        spec = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -700, 340)
        spec.set_editor_property("parameter_name", "Specular")
        spec.set_editor_property("default_value", 0.22)
        if textured:
            tex = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -1000, -320)
            tex.set_editor_property("parameter_name", "BaseTex")
            tex.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
            mul = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -380, -240)
            MEL.connect_material_expressions(tex, "RGB", mul, "A")
            MEL.connect_material_expressions(col, "", mul, "B")
            MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
            nt = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -1000, 480)
            nt.set_editor_property("parameter_name", "NormalTex")
            nt.set_editor_property("texture", FLAT)
            nt.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
            MEL.connect_material_property(nt, "RGB", unreal.MaterialProperty.MP_NORMAL)
        else:
            MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
        MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
        MEL.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
        MEL.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
        m.set_editor_property("used_with_nanite", True)
        m.set_editor_property("two_sided", True)
        MEL.recompile_material(m)
        EAL.save_asset(path)
        return m

    return make_one("M_IW_BrigTex", True), make_one("M_IW_BrigPaint", False)


def build_sail_canvas_master():
    base_dir = "/Game/Ironwake/Ships/Materials"
    path = base_dir + "/M_IW_SailCanvas"
    m = unreal.load_asset(path) if EAL.does_asset_exist(path) else \
        AT.create_asset("M_IW_SailCanvas", base_dir, unreal.Material, unreal.MaterialFactoryNew())
    MEL.delete_all_material_expressions(m)

    def E(cls, x, y):
        return MEL.create_material_expression(m, cls, x, y)

    def C(a, ao, b, bi):
        MEL.connect_material_expressions(a, ao, b, bi)

    def const1(val, x, y):
        k = E(unreal.MaterialExpressionConstant, x, y)
        k.set_editor_property("r", float(val))
        return k

    def sc_param(nm, val, x, y):
        p = E(unreal.MaterialExpressionScalarParameter, x, y)
        p.set_editor_property("parameter_name", nm)
        p.set_editor_property("default_value", float(val))
        return p

    def vec_param(nm, rgba, x, y):
        p = E(unreal.MaterialExpressionVectorParameter, x, y)
        p.set_editor_property("parameter_name", nm)
        p.set_editor_property("default_value", unreal.LinearColor(*rgba))
        return p

    def mask(src, ch, x, y):
        k = E(unreal.MaterialExpressionComponentMask, x, y)
        for c in "rgba":
            k.set_editor_property(c, c in ch)
        C(src, "", k, "")
        return k

    def mul(a, b, x, y):
        k = E(unreal.MaterialExpressionMultiply, x, y)
        C(a, "", k, "A")
        C(b, "", k, "B")
        return k

    def add(a, b, x, y):
        k = E(unreal.MaterialExpressionAdd, x, y)
        C(a, "", k, "A")
        C(b, "", k, "B")
        return k

    # Base color, roughness, warm backlit subsurface translucency
    col = vec_param("Color", (0.56, 0.50, 0.40, 1.0), -900, -350)
    rough = sc_param("Roughness", 0.88, -900, -150)
    ss_k = const1(0.78, -900, -50)
    ss = mul(col, ss_k, -650, -100)
    MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(ss, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)

    # WPO: algebraic pivot falloff d^2 / (d^2 + 120000) so spar/stay pivots stay locked at 0 displacement (no Clamp node!)
    wp = E(unreal.MaterialExpressionWorldPosition, -1800, 300)
    op = E(unreal.MaterialExpressionObjectPositionWS, -1800, 480)
    delta = E(unreal.MaterialExpressionSubtract, -1550, 380)
    C(wp, "", delta, "A")
    C(op, "", delta, "B")
    dx = mask(delta, "r", -1350, 300)
    dy = mask(delta, "g", -1350, 380)
    dz = mask(delta, "b", -1350, 460)
    d2 = add(add(mul(dx, dx, -1150, 300), mul(dy, dy, -1150, 380), -980, 340), mul(dz, dz, -1150, 460), -820, 400)
    piv_w = E(unreal.MaterialExpressionDivide, -640, 400)
    C(d2, "", piv_w, "A")
    C(add(d2, const1(120000.0, -820, 520), -720, 480), "", piv_w, "B")

    # Wind belly breathing + travelling luffing ripple along world BillowDir
    tm = E(unreal.MaterialExpressionTime, -1800, 720)
    ph = sc_param("Phase", 0.0, -1800, 840)
    belly = sc_param("WindBelly", 0.0, -1400, 640)
    luff = sc_param("LuffAmp", 0.0, -1400, 960)
    bdir = vec_param("BillowDir", (1.0, 0.0, 0.0, 0.0), -700, 950)
    bdir3 = mask(bdir, "rgb", -500, 950)

    s1 = E(unreal.MaterialExpressionSine, -1350, 740)
    s1.set_editor_property("period", 1.0)
    C(add(mul(tm, const1(0.28, -1620, 720), -1500, 720), ph, -1420, 760), "", s1, "")
    breath = add(const1(0.90, -1200, 680), mul(s1, const1(0.10, -1200, 780), -1100, 760), -960, 720)
    belly_term = mul(belly, breath, -780, 680)

    wx = mask(wp, "r", -1600, 1060)
    wy = mask(wp, "g", -1600, 1140)
    wz = mask(wp, "b", -1600, 1220)
    sp_xy = mul(add(wx, wy, -1420, 1080), const1(0.0012, -1420, 1160), -1260, 1100)
    sp_z = mul(wz, const1(0.0019, -1420, 1240), -1260, 1220)
    sp = add(sp_xy, sp_z, -1100, 1160)
    tm_rip = mul(tm, const1(-1.35, -1420, 1320), -1260, 1320)
    arg2 = add(add(sp, tm_rip, -960, 1220), ph, -840, 1180)
    s2 = E(unreal.MaterialExpressionSine, -720, 1180)
    s2.set_editor_property("period", 1.0)
    C(arg2, "", s2, "")
    luff_term = mul(luff, s2, -580, 1080)

    mag = mul(add(belly_term, luff_term, -460, 820), piv_w, -320, 680)
    wpo = mul(bdir3, mag, -160, 820)
    MEL.connect_material_property(wpo, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)

    m.set_editor_property("two_sided", True)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    MEL.recompile_material(m)
    EAL.save_asset(path)
    unreal.log("IWP26 M_IW_SailCanvas built and saved: %s" % path)
    return m


def resolve_path(d, val):
    return val if os.path.isabs(val) else os.path.join(d, val)


def import_tex(fn, dest, srgb=True, normal=False, maxsize=2048):
    base = os.path.splitext(os.path.basename(fn))[0]
    name = base if base.startswith("T_") else ("T_" + base)
    path = "%s/%s" % (dest, name)
    t = unreal.AssetImportTask()
    t.filename = fn
    t.destination_path = dest
    t.destination_name = name
    t.automated = True
    t.save = True
    t.replace_existing = True
    AT.import_asset_tasks([t])
    tex = unreal.load_asset(path)
    if tex:
        tex.set_editor_property("srgb", srgb)
        if normal:
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        elif not srgb:
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        tex.set_editor_property("max_texture_size", maxsize)
        EAL.save_asset(path)
    return tex


M_BRIG_TEX, M_BRIG_PAINT = build_brig_masters()
M_SAIL = build_sail_canvas_master()
report = {}

names = sorted(n for n in os.listdir(SRC) if os.path.isdir(os.path.join(SRC, n)))
if ONLY:
    names = [n for n in names if n in ONLY]

for name in names:
    d = os.path.join(SRC, name)
    fbx = os.path.join(d, name + ".fbx")
    if not os.path.exists(fbx):
        continue
    ispart = (name != "TallBrig")
    dest = "/Game/Ironwake/World/%s" % name
    t = unreal.AssetImportTask()
    t.filename = fbx
    t.destination_path = dest
    t.destination_name = "SM_" + name
    t.automated = True
    t.save = True
    t.replace_existing = True
    o = unreal.FbxImportUI()
    o.import_mesh = True
    o.import_as_skeletal = False
    o.import_materials = False
    o.import_textures = False
    o.static_mesh_import_data.combine_meshes = True
    o.static_mesh_import_data.remove_degenerates = False
    o.static_mesh_import_data.generate_lightmap_u_vs = False
    o.static_mesh_import_data.auto_generate_collision = False
    o.static_mesh_import_data.build_nanite = not ispart
    o.static_mesh_import_data.set_editor_property("import_uniform_scale", 1.0)
    t.options = o
    AT.import_asset_tasks([t])

    path = "%s/SM_%s" % (dest, name)
    m = unreal.load_asset(path)
    if not m:
        unreal.log_error("IWP26 import failed %s" % path)
        continue

    ns = sub.get_nanite_settings(m)
    ns.enabled = not ispart
    sub.set_nanite_settings(m, ns, True)

    n_prims = 0
    try:
        sub.remove_collisions(m)
        if not ispart:
            sub.add_simple_collisions(m, unreal.ScriptCollisionShapeType.NDOP26)
        n_prims = sub.get_simple_collision_count(m) + sub.get_convex_collision_count(m)
    except Exception as ex:
        unreal.log_warning("IWP26 collision %s: %s" % (name, ex))

    mats = json.load(open(os.path.join(d, "materials.json")))
    mis = {}
    for mn, e in mats.items():
        safe = "MI_%s_%s" % (name, mn.replace(".", "_").replace(" ", "_").replace("#", ""))
        mp = "%s/%s" % (dest, safe)
        mi = unreal.load_asset(mp) if EAL.does_asset_exist(mp) else \
            AT.create_asset(safe, dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        c = e["color"]
        if mn == "IW_SailCanvas":
            MEL.set_material_instance_parent(mi, M_SAIL)
            MEL.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(c[0] * 0.74, c[1] * 0.74, c[2] * 0.75, 1.0))
            MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", e.get("rough", 0.88))
        elif "base" in e:
            MEL.set_material_instance_parent(mi, M_BRIG_TEX)
            MEL.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(c[0], c[1], c[2], 1.0))
            MEL.set_material_instance_texture_parameter_value(mi, "BaseTex", import_tex(resolve_path(d, e["base"]), dest, True, False, 2048))
            MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", 0.74)
            MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", 0.0)
            MEL.set_material_instance_scalar_parameter_value(mi, "Specular", 0.25)
            nt = import_tex(resolve_path(d, e["normal"]), dest, False, True, 2048) if "normal" in e else FLAT
            MEL.set_material_instance_texture_parameter_value(mi, "NormalTex", nt)
        else:
            MEL.set_material_instance_parent(mi, M_BRIG_PAINT)
            sc = (0.19, 0.12, 0.062) if mn == "IW_SparWood" else c
            MEL.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(sc[0], sc[1], sc[2], 1.0))
            MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", 0.82 if mn == "IW_SparWood" else (0.88 if mn == "IW_StayRope" else 0.58))
            MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", 0.0)
            MEL.set_material_instance_scalar_parameter_value(mi, "Specular", 0.22 if mn != "IW_StayRope" else 0.15)
        EAL.save_asset(mp)
        mis[mn] = mi

    sm = m.get_editor_property("static_materials")
    n = 0
    for i, s in enumerate(sm):
        key = str(s.get_editor_property("material_slot_name"))
        mi = mis.get(key) or next((v for k, v in mis.items() if k.replace(".", "_").replace(" ", "_") == key or key.startswith(k.replace(".", "_"))), None)
        if mi is None and len(mis) == 1:
            mi = list(mis.values())[0]
        if mi:
            m.set_material(i, mi)
            n += 1
        else:
            unreal.log_warning("IWP26 no MI for slot %s on %s (have %s)" % (key, name, list(mis)))
    EAL.save_asset(path)
    b = m.get_bounding_box()
    report[name] = {
        "tris": m.get_num_triangles(0),
        "nanite": bool(sub.get_nanite_settings(m).enabled),
        "collision_prims": int(n_prims),
        "slots": "%d/%d" % (n, len(sm)),
        "min": [round(b.min.x, 1), round(b.min.y, 1), round(b.min.z, 1)],
        "max": [round(b.max.x, 1), round(b.max.y, 1), round(b.max.z, 1)],
    }
    unreal.log("IWP26 %s %s" % (name, report[name]))
    EAL.save_directory(dest)

with open(os.path.join(proj, "Saved/iw_p26_import.json"), "w") as f:
    json.dump(report, f, indent=1)
unreal.log("IWP26 DONE %d meshes" % len(report))
unreal.SystemLibrary.quit_editor()
