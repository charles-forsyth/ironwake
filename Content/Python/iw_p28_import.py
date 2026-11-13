"""P28: headless import of pre-built UE-ready bundles:
1) $IRONWAKE_ASSETS/models/ue_ready/p28/* (PaddleSteamerV3+wheels, SalvageTenderV3+crane+bell,
   PatrolAirshipV4+3 props, SeaGullV4+2 wings, SailorCrewStand, SailorCrewRower, TownTavernV4, TownChandleryV4, TownFoundryV4)
2) $IRONWAKE_ASSETS/models/ue_ready/p28_signs/* (SignAnchorTavern, SignShipChandlery)
3) $IRONWAKE_ASSETS/models/ue_ready/ground_scans_4k/* -> Metal-SM6-safe rational micro-detail blend
   1 / (1 + (PixelDepth / 3500)^2) in M_IW_Hill and M_IW_Road (ZERO MaterialExpressionClamp nodes!).
"""
import unreal, os, json

proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
CONCEPT_READY = os.path.join(os.environ.get("IRONWAKE_ASSETS", "assets"), "models/ue_ready")
SRC_DIRS = [
    os.path.join(CONCEPT_READY, "p28"),
    os.path.join(CONCEPT_READY, "p28_signs"),
]
SCAN_DIR = os.path.join(CONCEPT_READY, "ground_scans_4k")
SCAN_DEST = "/Game/Ironwake/Materials/GroundScans"
TOWN_DEST = "/Game/Ironwake/Town"

MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
SUB = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
FLAT = unreal.load_asset("/Engine/EngineMaterials/FlatNormal")


def log(msg):
    unreal.log("IWP28M " + str(msg))


def ensure_ship_masters():
    """Ensure M_IW_BrigTex and M_IW_BrigPaint exist with matte Specular/Metallic parameters."""
    base_dir = "/Game/Ironwake/Ships/Materials"

    def get_or_make(asset_name, textured):
        path = base_dir + "/" + asset_name
        if EAL.does_asset_exist(path):
            return unreal.load_asset(path)
        m = AT.create_asset(asset_name, base_dir, unreal.Material, unreal.MaterialFactoryNew())
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
        m.set_editor_property("used_with_instanced_static_meshes", True)
        m.set_editor_property("two_sided", True)
        MEL.recompile_material(m)
        EAL.save_asset(path)
        return m

    m_tex = get_or_make("M_IW_BrigTex", True)
    m_paint = get_or_make("M_IW_BrigPaint", False)
    # Ensure used_with_instanced_static_meshes is True for HISM dockhands/clutter!
    for m, p in ((m_tex, base_dir + "/M_IW_BrigTex"), (m_paint, base_dir + "/M_IW_BrigPaint")):
        if not m.get_editor_property("used_with_instanced_static_meshes"):
            m.set_editor_property("used_with_instanced_static_meshes", True)
            MEL.recompile_material(m)
            EAL.save_asset(p)
    return m_tex, m_paint


def resolve_path(d, val):
    return val if os.path.isabs(val) else os.path.join(d, val)


def import_tex(fn, dest, name_override=None, kind="albedo", maxsize=2048):
    base = os.path.splitext(os.path.basename(fn))[0]
    name = name_override or (base if base.startswith("T_") else ("T_" + base))
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
    if not tex:
        raise RuntimeError("Failed to import texture: %s" % fn)
    if kind == "normal":
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("srgb", False)
    elif kind == "roughness":
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
        tex.set_editor_property("srgb", False)
    elif kind == "masks":
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        tex.set_editor_property("srgb", False)
    else:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
        tex.set_editor_property("srgb", True)
    tex.set_editor_property("max_texture_size", maxsize)
    EAL.save_asset(path)
    return tex


def import_p28_meshes(m_tex, m_paint):
    only = [s for s in os.environ.get("IWP28_ONLY", "").split(",") if s]
    report = {}
    bundles = []
    for sdir in SRC_DIRS:
        if not os.path.isdir(sdir):
            continue
        for n in sorted(os.listdir(sdir)):
            if only and n not in only:
                continue
            d = os.path.join(sdir, n)
            fbx = os.path.join(d, n + ".fbx")
            if os.path.isdir(d) and os.path.exists(fbx):
                bundles.append((n, d, fbx))

    for name, d, fbx in bundles:
        is_part_or_small = (
            "_wheel_" in name
            or "_crane" in name
            or "_bell" in name
            or "_prop_" in name
            or "SeaGullV4" in name
            or "SailorCrew" in name
            or name.startswith("Sign")
        )
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
        o.static_mesh_import_data.build_nanite = not is_part_or_small
        o.static_mesh_import_data.set_editor_property("import_uniform_scale", 1.0)
        t.options = o
        AT.import_asset_tasks([t])

        path = "%s/SM_%s" % (dest, name)
        m = unreal.load_asset(path)
        if not m:
            unreal.log_error("IWP28M import failed %s" % path)
            continue

        ns = SUB.get_nanite_settings(m)
        ns.enabled = not is_part_or_small
        SUB.set_nanite_settings(m, ns, True)

        if "SailorCrew" in name or "SeaGullV4" in name:
            opts = unreal.StaticMeshReductionOptions(
                auto_compute_lod_screen_size=False,
                reduction_settings=[
                    unreal.StaticMeshReductionSettings(percent_triangles=0.22, screen_size=1.0),
                    unreal.StaticMeshReductionSettings(percent_triangles=0.06, screen_size=0.20),
                    unreal.StaticMeshReductionSettings(percent_triangles=0.015, screen_size=0.07),
                ],
            )
            SUB.set_lods(m, opts)

        n_prims = 0
        try:
            SUB.remove_collisions(m)
            if not is_part_or_small:
                shape = unreal.ScriptCollisionShapeType.BOX if name.startswith("Town") else unreal.ScriptCollisionShapeType.NDOP26
                SUB.add_simple_collisions(m, shape)
            n_prims = SUB.get_simple_collision_count(m) + SUB.get_convex_collision_count(m)
        except Exception as ex:
            unreal.log_warning("IWP28M collision %s: %s" % (name, ex))

        mats = json.load(open(os.path.join(d, "materials.json")))
        mis = {}
        for mn, e in mats.items():
            safe = "MI_%s_%s" % (name, mn.replace(".", "_").replace(" ", "_").replace("#", ""))
            mp = "%s/%s" % (dest, safe)
            mi = unreal.load_asset(mp) if EAL.does_asset_exist(mp) else \
                AT.create_asset(safe, dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
            c = e["color"]
            if "base" in e:
                MEL.set_material_instance_parent(mi, m_tex)
                MEL.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(c[0], c[1], c[2], 1.0))
                MEL.set_material_instance_texture_parameter_value(
                    mi, "BaseTex", import_tex(resolve_path(d, e["base"]), dest, None, "albedo", 2048)
                )
                is_sign_face = mn.startswith("IW_Sign") and mn.endswith("_Face")
                # Force Metallic=0 on Hunyuan Material_0 so painted hulls/buildings/crew never render dark-metallic
                rough_val = float(e.get("rough", 0.48)) if is_sign_face else max(float(e.get("rough", 0.78)), 0.72)
                metal_val = float(e.get("metal", 0.05)) if is_sign_face else 0.0
                spec_val = 0.30 if is_sign_face else 0.24
                MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", rough_val)
                MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", metal_val)
                MEL.set_material_instance_scalar_parameter_value(mi, "Specular", spec_val)
                nt = import_tex(resolve_path(d, e["normal"]), dest, None, "normal", 2048) if "normal" in e else FLAT
                MEL.set_material_instance_texture_parameter_value(mi, "NormalTex", nt)
            else:
                MEL.set_material_instance_parent(mi, m_paint)
                MEL.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(c[0], c[1], c[2], 1.0))
                rough_val = float(e.get("rough", 0.65))
                metal_val = min(float(e.get("metal", 0.0)), 0.88)
                spec_val = 0.35 if metal_val > 0.3 else 0.22
                MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", rough_val)
                MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", metal_val)
                MEL.set_material_instance_scalar_parameter_value(mi, "Specular", spec_val)
            EAL.save_asset(mp)
            mis[mn] = mi

        sm = m.get_editor_property("static_materials")
        n_assigned = 0
        for i, s in enumerate(sm):
            key = str(s.get_editor_property("material_slot_name"))
            mi = mis.get(key) or next(
                (v for k, v in mis.items() if k.replace(".", "_").replace(" ", "_") == key or key.startswith(k.replace(".", "_"))),
                None,
            )
            if mi is None and len(mis) == 1:
                mi = list(mis.values())[0]
            if mi:
                m.set_material(i, mi)
                n_assigned += 1
            else:
                unreal.log_warning("IWP28M no MI for slot %s on %s (have %s)" % (key, name, list(mis)))
        EAL.save_asset(path)
        b = m.get_bounding_box()
        report[name] = {
            "tris": m.get_num_triangles(0),
            "nanite": bool(SUB.get_nanite_settings(m).enabled),
            "collision_prims": int(n_prims),
            "slots": "%d/%d" % (n_assigned, len(sm)),
            "min": [round(b.min.x, 1), round(b.min.y, 1), round(b.min.z, 1)],
            "max": [round(b.max.x, 1), round(b.max.y, 1), round(b.max.z, 1)],
        }
        log("%s %s" % (name, report[name]))
        EAL.save_directory(dest)
    return report


def upgrade_ground_materials_4k():
    """Import 4K Megascans ForestFloor & MossyFloor maps and rebuild M_IW_Hill & M_IW_Road in-place
    with Metal-safe rational falloff w = 1 / (1 + (PixelDepth / 3500)^2). Zero MaterialExpressionClamp nodes!
    """
    t_ff_alb = import_tex(os.path.join(SCAN_DIR, "T_Ground_ForestFloor_4K_Albedo.png"), SCAN_DEST, "T_Ground_ForestFloor_4K_Albedo", "albedo", 4096)
    t_ff_nrm = import_tex(os.path.join(SCAN_DIR, "T_Ground_ForestFloor_4K_Normal.png"), SCAN_DEST, "T_Ground_ForestFloor_4K_Normal", "normal", 4096)
    t_ff_rgh = import_tex(os.path.join(SCAN_DIR, "T_Ground_ForestFloor_4K_Roughness.png"), SCAN_DEST, "T_Ground_ForestFloor_4K_Roughness", "roughness", 4096)

    t_mf_alb = import_tex(os.path.join(SCAN_DIR, "T_Ground_MossyFloor_4K_Albedo.png"), SCAN_DEST, "T_Ground_MossyFloor_4K_Albedo", "albedo", 4096)
    t_mf_nrm = import_tex(os.path.join(SCAN_DIR, "T_Ground_MossyFloor_4K_Normal.png"), SCAN_DEST, "T_Ground_MossyFloor_4K_Normal", "normal", 4096)
    t_mf_rgh = import_tex(os.path.join(SCAN_DIR, "T_Ground_MossyFloor_4K_Roughness.png"), SCAN_DEST, "T_Ground_MossyFloor_4K_Roughness", "roughness", 4096)

    TX = {}
    for tag, a in (("Grass", "grass_ground"), ("Earth", "brown_mud_dry"), ("Scree", "gray_rocks"), ("Rock", "aerial_rocks_02"), ("Dirt", "dirt_aerial_03")):
        d_tex = unreal.load_asset("%s/T_hill_%s_d" % (TOWN_DEST, a))
        n_tex = unreal.load_asset("%s/T_hill_%s_n" % (TOWN_DEST, a))
        if not d_tex or not n_tex:
            raise RuntimeError("Missing existing P15 texture for %s" % tag)
        TX[tag] = (d_tex, n_tex)
    cob_d = unreal.load_asset(TOWN_DEST + "/T_cobblestone_floor_02_d")
    cob_n = unreal.load_asset(TOWN_DEST + "/T_cobblestone_floor_02_n")

    def C(a, ao, b, bi):
        assert MEL.connect_material_expressions(a, ao, b, bi), (a, ao, b, bi)

    # ---------------- 1. Rebuild M_IW_Hill in-place ----------------
    hill_path = TOWN_DEST + "/M_IW_Hill"
    m = unreal.load_asset(hill_path) if EAL.does_asset_exist(hill_path) else \
        AT.create_asset("M_IW_Hill", TOWN_DEST, unreal.Material, unreal.MaterialFactoryNew())
    MEL.delete_all_material_expressions(m)
    E = lambda cls, x, y: MEL.create_material_expression(m, cls, x, y)

    def c1(v, x, y):
        k = E(unreal.MaterialExpressionConstant, x, y)
        k.set_editor_property("r", float(v))
        return k

    def sc(nm, v, x, y):
        s = E(unreal.MaterialExpressionScalarParameter, x, y)
        s.set_editor_property("parameter_name", nm)
        s.set_editor_property("default_value", float(v))
        return s

    def vc4(nm, c, x, y):
        s = E(unreal.MaterialExpressionVectorParameter, x, y)
        s.set_editor_property("parameter_name", nm)
        s.set_editor_property("default_value", unreal.LinearColor(*c))
        return s

    def tex(nm, t, x, y, stype=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR):
        e = E(unreal.MaterialExpressionTextureSampleParameter2D, x, y)
        e.set_editor_property("parameter_name", nm)
        e.set_editor_property("texture", t)
        e.set_editor_property("sampler_type", stype)
        e.set_editor_property("sampler_source", unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)
        return e

    def mask(src, ch, x, y):
        k = E(unreal.MaterialExpressionComponentMask, x, y)
        for ch_i in "rgba":
            k.set_editor_property(ch_i, ch_i in ch)
        C(src, "", k, "")
        return k

    def mul(a, ao, b, bo, x, y):
        k = E(unreal.MaterialExpressionMultiply, x, y)
        C(a, ao, k, "A")
        C(b, bo, k, "B")
        return k

    def add(a, ao, b, bo, x, y):
        k = E(unreal.MaterialExpressionAdd, x, y)
        C(a, ao, k, "A")
        C(b, bo, k, "B")
        return k

    def div(a, ao, b, bo, x, y):
        k = E(unreal.MaterialExpressionDivide, x, y)
        C(a, ao, k, "A")
        C(b, bo, k, "B")
        return k

    def lerp(a, ao, b, bo, al, alo, x, y):
        k = E(unreal.MaterialExpressionLinearInterpolate, x, y)
        C(a, ao, k, "A")
        C(b, bo, k, "B")
        C(al, alo, k, "Alpha")
        return k

    wp = E(unreal.MaterialExpressionWorldPosition, -2400, 0)
    xy = mask(wp, "rg", -2180, -100)
    wx = mask(wp, "r", -2180, 100)
    wy = mask(wp, "g", -2180, 180)
    wz = mask(wp, "b", -2180, 260)

    def app(a, b, x, y):
        k = E(unreal.MaterialExpressionAppendVector, x, y)
        C(a, "", k, "A")
        C(b, "", k, "B")
        return k

    uvxz = app(wx, wz, -2000, 120)
    uvyz = app(wy, wz, -2000, 220)
    vcol = E(unreal.MaterialExpressionVertexColor, -2000, 600)

    layers = {}
    y0 = -900
    for tag, tile in (("Grass", 520.0), ("Earth", 380.0), ("Scree", 300.0)):
        t = sc(tag + "Tile", tile, -1750, y0)
        dv = div(xy, "", t, "", -1580, y0)
        d = tex(tag + "Diff", TX[tag][0], -1380, y0)
        C(dv, "", d, "UVs")
        n = tex(tag + "Nor", TX[tag][1], -1380, y0 + 200, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        C(dv, "", n, "UVs")
        tn = vc4(tag + "Tint", {"Grass": (0.46, 0.78, 0.26, 1), "Earth": (0.95, 0.85, 0.70, 1), "Scree": (1.0, 1.0, 1.0, 1)}[tag], -1380, y0 - 150)
        layers[tag] = (mul(d, "RGB", tn, "", -1120, y0), n)
        y0 += 450

    rt = sc("RockTile", 650.0, -1750, y0)
    dxz = div(uvxz, "", rt, "", -1580, y0)
    dyz = div(uvyz, "", rt, "", -1580, y0 + 100)
    r1 = tex("RockDiff", TX["Rock"][0], -1380, y0)
    C(dxz, "", r1, "UVs")
    r2 = tex("RockDiff", TX["Rock"][0], -1380, y0 + 250)
    C(dyz, "", r2, "UVs")
    vn = E(unreal.MaterialExpressionVertexNormalWS, -1750, y0 + 400)
    ab = E(unreal.MaterialExpressionAbs, -1580, y0 + 400)
    C(vn, "", ab, "")
    nx = mask(ab, "r", -1420, y0 + 360)
    ny = mask(ab, "g", -1420, y0 + 430)
    nz = mask(ab, "b", -1420, y0 + 500)
    den2 = add(add(nx, "", ny, "", -1260, y0 + 400), "", c1(0.001, -1260, y0 + 500), "", -1120, y0 + 420)
    tb = div(ny, "", den2, "", -980, y0 + 420)
    rl = lerp(r2, "RGB", r1, "RGB", tb, "", -820, y0 + 100)
    rn1 = tex("RockNor", TX["Rock"][1], -1380, y0 + 560, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL); C(dxz, "", rn1, "UVs")
    rn2 = tex("RockNor", TX["Rock"][1], -1380, y0 + 740, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL); C(dyz, "", rn2, "UVs")
    rnl = lerp(rn2, "RGB", rn1, "RGB", tb, "", -820, y0 + 620)
    rtint = vc4("RockTint", (0.82, 0.80, 0.76, 1), -820, y0 - 80)
    layers["Rock"] = (mul(rl, "", rtint, "", -650, y0 + 100), rnl)

    # Full 3-axis triplanar weights (w_yz, w_xz, w_xy) from abs(VertexNormalWS)
    sum3 = add(add(add(nx, "", ny, "", -1260, y0 + 880), "", nz, "", -1120, y0 + 880), "", c1(0.001, -1120, y0 + 960), "", -980, y0 + 900)
    tw_yz = div(nx, "", sum3, "", -820, y0 + 840)
    tw_xz = div(ny, "", sum3, "", -820, y0 + 910)
    tw_xy = div(nz, "", sum3, "", -820, y0 + 980)

    def vpin(pin, y):
        k = E(unreal.MaterialExpressionComponentMask, -900, y)
        for ch_i in "rgba":
            k.set_editor_property(ch_i, False)
        k.set_editor_property("r", True)
        C(vcol, pin, k, "")
        return k

    cr, cg, cb, ca = (vpin(pin, 700 + 70 * i) for i, pin in enumerate(("R", "G", "B", "A")))
    acc = None
    for w_v, k in zip((cr, cg, cb, ca), ("Grass", "Earth", "Scree", "Rock")):
        term = mul(layers[k][0], "", w_v, "", -500, 300 + 70 * len(k))
        acc = term if acc is None else add(acc, "", term, "", -350, 300)

    def macro(nm, tile, x, y, swap):
        t = sc(nm, tile, x - 200, y)
        d = div(uvyz if swap else xy, "", t, "", x - 100, y)
        e = tex(nm + "T", TX["Earth"][0], x, y)
        C(d, "", e, "UVs")
        return mask(e, "g", x + 120, y)

    n1 = macro("Macro1Cm", 9000.0, -1100, 1100, False)
    n2 = macro("Macro2Cm", 23000.0, -1100, 1250, True)
    br = add(mul(add(n1, "", n2, "", -720, 1170), "", c1(0.42, -720, 1280), "", -580, 1200), "", c1(0.72, -580, 1300), "", -440, 1240)
    cf_macro = mul(acc, "", br, "", -240, 500)

    nacc = None
    for w_v, k in zip((cr, cg, cb, ca), ("Grass", "Earth", "Scree", "Rock")):
        term = mul(layers[k][1], "RGB" if k != "Rock" else "", w_v, "", -500, 1500)
        nacc = term if nacc is None else add(nacc, "", term, "", -350, 1500)

    # ---- P28 World-Space Triplanar 4K Megascans Micro-Detail within ~45 m: w = 1 / (1 + (PixelDepth / 3500)^2) ----
    # Strictly algebraic (PixelDepth, Divide, Multiply, Add) -> ZERO MaterialExpressionClamp!
    pdepth = E(unreal.MaterialExpressionPixelDepth, -1500, 1900)
    d0 = sc("MicroFadeCm", 3500.0, -1500, 2020)
    dnrm = div(pdepth, "", d0, "", -1320, 1940)
    dsq = mul(dnrm, "", dnrm, "", -1160, 1940)
    w_micro = div(c1(1.0, -1160, 1840), "", add(dsq, "", c1(1.0, -1160, 2040), "", -1000, 1960), "", -840, 1920)

    def triplanar_sample(param_nm, tex_asset, tile_sc, stype, out_pin, y_base):
        u_xy = div(xy, "", tile_sc, "", -1420, y_base)
        u_xz = div(uvxz, "", tile_sc, "", -1420, y_base + 120)
        u_yz = div(uvyz, "", tile_sc, "", -1420, y_base + 240)
        s_xy = tex(param_nm, tex_asset, -1220, y_base, stype); C(u_xy, "", s_xy, "UVs")
        s_xz = tex(param_nm, tex_asset, -1220, y_base + 140, stype); C(u_xz, "", s_xz, "UVs")
        s_yz = tex(param_nm, tex_asset, -1220, y_base + 280, stype); C(u_yz, "", s_yz, "UVs")
        return add(
            add(mul(s_xy, out_pin, tw_xy, "", -980, y_base), "", mul(s_xz, out_pin, tw_xz, "", -980, y_base + 120), "", -840, y_base + 60),
            "",
            mul(s_yz, out_pin, tw_yz, "", -980, y_base + 240),
            "", -700, y_base + 120
        )

    ff_tile = sc("ForestMicroTileCm", 195.0, -1620, 2200)
    mf_tile = sc("MossMicroTileCm", 185.0, -1620, 3300)
    ff_a = triplanar_sample("ForestMicroAlb", t_ff_alb, ff_tile, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, "RGB", 2140)
    ff_n = triplanar_sample("ForestMicroNor", t_ff_nrm, ff_tile, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, "RGB", 2520)
    ff_r = triplanar_sample("ForestMicroRgh", t_ff_rgh, ff_tile, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE, "R", 2900)
    mf_a = triplanar_sample("MossMicroAlb", t_mf_alb, mf_tile, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, "RGB", 3280)
    mf_n = triplanar_sample("MossMicroNor", t_mf_nrm, mf_tile, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, "RGB", 3660)

    scree_or_rock = E(unreal.MaterialExpressionSaturate, -640, 2100)
    C(add(cb, "", ca, "", -760, 2100), "", scree_or_rock, "")
    det_alb = lerp(ff_a, "", mf_a, "", scree_or_rock, "", -540, 2240)
    det_nrm = lerp(ff_n, "", mf_n, "", scree_or_rock, "", -540, 2520)
    det_rgh = ff_r

    # Preserve macro tint from 400m while giving rich triplanar 4K grain underfoot & on cliff banks (< 45m)
    det_gain = add(mul(det_alb, "", c1(2.25, -540, 2380), "", -400, 2280), "", c1(0.45, -400, 2400), "", -280, 2300)
    cf_detailed = mul(cf_macro, "", det_gain, "", -180, 2180)
    w_col = mul(w_micro, "", sc("MicroColorStrength", 0.78, -840, 2060), "", -640, 2020)
    base_final = lerp(cf_macro, "", cf_detailed, "", w_col, "", -60, 560)
    MEL.connect_material_property(base_final, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Crisp 4K triplanar normal blend within 45 m
    nrm_near = add(mul(nacc, "", c1(0.40, -500, 2480), "", -340, 2460), "", mul(det_nrm, "", c1(0.95, -500, 2600), "", -340, 2580), "", -180, 2520)
    nrm_final = lerp(nacc, "", nrm_near, "", w_micro, "", -40, 1500)
    MEL.connect_material_property(nrm_final, "", unreal.MaterialProperty.MP_NORMAL)

    rgh_final = lerp(c1(0.92, -300, 2820), "", det_rgh, "", w_micro, "", -40, 820)
    MEL.connect_material_property(rgh_final, "", unreal.MaterialProperty.MP_ROUGHNESS)

    MEL.recompile_material(m)
    EAL.save_asset(hill_path)
    log("M_IW_Hill upgraded with 3-axis Triplanar 4K rational micro-detail (0 Clamp nodes)")

    # ---------------- 2. Rebuild M_IW_Road in-place (streets & verges) ----------------
    road_path = TOWN_DEST + "/M_IW_Road"
    mr = unreal.load_asset(road_path) if EAL.does_asset_exist(road_path) else \
        AT.create_asset("M_IW_Road", TOWN_DEST, unreal.Material, unreal.MaterialFactoryNew())
    MEL.delete_all_material_expressions(mr)
    Er = lambda cls, x, y: MEL.create_material_expression(mr, cls, x, y)

    def rc1(v, x, y):
        k = Er(unreal.MaterialExpressionConstant, x, y)
        k.set_editor_property("r", float(v))
        return k

    def rsc(nm, v, x, y):
        s = Er(unreal.MaterialExpressionScalarParameter, x, y)
        s.set_editor_property("parameter_name", nm)
        s.set_editor_property("default_value", float(v))
        return s

    def rvc4(nm, c, x, y):
        s = Er(unreal.MaterialExpressionVectorParameter, x, y)
        s.set_editor_property("parameter_name", nm)
        s.set_editor_property("default_value", unreal.LinearColor(*c))
        return s

    def rtex(nm, t, x, y, stype=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR):
        e = Er(unreal.MaterialExpressionTextureSampleParameter2D, x, y)
        e.set_editor_property("parameter_name", nm)
        e.set_editor_property("texture", t)
        e.set_editor_property("sampler_type", stype)
        e.set_editor_property("sampler_source", unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)
        return e

    def rmask(src, ch, x, y):
        k = Er(unreal.MaterialExpressionComponentMask, x, y)
        for ch_i in "rgba":
            k.set_editor_property(ch_i, ch_i in ch)
        C(src, "", k, "")
        return k

    def rmul(a, ao, b, bo, x, y):
        k = Er(unreal.MaterialExpressionMultiply, x, y)
        C(a, ao, k, "A")
        C(b, bo, k, "B")
        return k

    def radd(a, ao, b, bo, x, y):
        k = Er(unreal.MaterialExpressionAdd, x, y)
        C(a, ao, k, "A")
        C(b, bo, k, "B")
        return k

    def rdiv(a, ao, b, bo, x, y):
        k = Er(unreal.MaterialExpressionDivide, x, y)
        C(a, ao, k, "A")
        C(b, bo, k, "B")
        return k

    def rlerp(a, ao, b, bo, al, alo, x, y):
        k = Er(unreal.MaterialExpressionLinearInterpolate, x, y)
        C(a, ao, k, "A")
        C(b, bo, k, "B")
        C(al, alo, k, "Alpha")
        return k

    rwp = Er(unreal.MaterialExpressionWorldPosition, -1800, 0)
    rxy = rmask(rwp, "rg", -1600, 0)
    dv1 = rdiv(rxy, "", rsc("CobTile", 210.0, -1600, 150), "", -1400, 0)
    dv2 = rdiv(rxy, "", rsc("DirtTile", 330.0, -1600, 300), "", -1400, 200)
    dv_micro = rdiv(rxy, "", rsc("VergeMicroTileCm", 145.0, -1600, 520), "", -1400, 500)

    cd = rtex("CobDiff", cob_d, -1150, -200); C(dv1, "", cd, "UVs")
    cn = rtex("CobNor", cob_n, -1150, 0, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL); C(dv1, "", cn, "UVs")
    dd = rtex("DirtDiff", TX["Dirt"][0], -1150, 220); C(dv2, "", dd, "UVs")
    dn = rtex("DirtNor", TX["Dirt"][1], -1150, 400, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL); C(dv2, "", dn, "UVs")

    v_alb = rtex("VergeMicroAlb", t_ff_alb, -1150, 620); C(dv_micro, "", v_alb, "UVs")
    v_nrm = rtex("VergeMicroNor", t_ff_nrm, -1150, 800, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL); C(dv_micro, "", v_nrm, "UVs")
    v_rgh = rtex("VergeMicroRgh", t_ff_rgh, -1150, 980, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE); C(dv_micro, "", v_rgh, "UVs")

    # Rational PixelDepth falloff: w = 1 / (1 + (PixelDepth / 3500)^2)
    rpdepth = Er(unreal.MaterialExpressionPixelDepth, -1600, 1180)
    rdn = rdiv(rpdepth, "", rsc("MicroFadeCm", 3500.0, -1600, 1280), "", -1400, 1200)
    rdsq = rmul(rdn, "", rdn, "", -1240, 1200)
    rw = rdiv(rc1(1.0, -1240, 1100), "", radd(rdsq, "", rc1(1.0, -1240, 1300), "", -1080, 1220), "", -920, 1180)

    rvcol = Er(unreal.MaterialExpressionVertexColor, -1150, 1420)
    rwe = rmask(rvcol, "r", -950, 1420)
    rsm = radd(rwe, "", rsc("DirtAmt", 0.0, -950, 1520), "", -780, 1460)
    rsat = Er(unreal.MaterialExpressionSaturate, -620, 1460); C(rsm, "", rsat, "")

    # Enrich dirt/verge side with 4K ForestFloor micro-grain within 45 m
    dirt_near = rmul(dd, "RGB", radd(rmul(v_alb, "RGB", rc1(2.2, -920, 640), "", -760, 620), "", rc1(0.48, -760, 720), "", -600, 640), "", -440, 260)
    dd_final = rlerp(dd, "RGB", dirt_near, "", rmul(rw, "", rc1(0.80, -920, 760), "", -760, 760), "", -280, 240)
    dn_final = rlerp(dn, "RGB", v_nrm, "RGB", rmul(rw, "", rc1(0.85, -920, 860), "", -760, 860), "", -280, 440)

    lb = rlerp(cd, "RGB", dd_final, "", rsat, "", -120, 0)
    rtint = rvc4("Tint", (1.0, 1.0, 1.0, 1), -120, -220)
    bc = rmul(lb, "", rtint, "", 60, 0)
    MEL.connect_material_property(bc, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Also add subtle 4K surface normal relief across cobbles + verges
    cob_n_near = radd(rmul(cn, "RGB", rc1(0.82, -480, -40), "", -320, -20), "", rmul(v_nrm, "RGB", rc1(0.30, -480, 40), "", -320, 40), "", -180, 10)
    cn_final = rlerp(cn, "RGB", cob_n_near, "", rw, "", -60, 120)
    ln = rlerp(cn_final, "", dn_final, "", rsat, "", 80, 280)
    MEL.connect_material_property(ln, "", unreal.MaterialProperty.MP_NORMAL)

    rgh_base = rc1(0.88, -280, 980)
    rgh_out = rlerp(rgh_base, "", v_rgh, "R", rmul(rw, "", rc1(0.75, -500, 1060), "", -340, 1060), "", 80, 560)
    MEL.connect_material_property(rgh_out, "", unreal.MaterialProperty.MP_ROUGHNESS)

    MEL.recompile_material(mr)
    EAL.save_asset(road_path)
    log("M_IW_Road upgraded with 4K rational verge micro-detail (0 Clamp nodes)")

    # ---------------- 3. Rebuild M_IW_Tile in-place (3-axis Triplanar for quay apron, retaining walls, plinths, kerbs + 4K rational micro-detail) ----------------
    tile_path = TOWN_DEST + "/M_IW_Tile"
    mt = unreal.load_asset(tile_path) if EAL.does_asset_exist(tile_path) else \
        AT.create_asset("M_IW_Tile", TOWN_DEST, unreal.Material, unreal.MaterialFactoryNew())
    MEL.delete_all_material_expressions(mt)
    Et = lambda cls, x, y: MEL.create_material_expression(mt, cls, x, y)

    def tc1(v, x, y):
        k = Et(unreal.MaterialExpressionConstant, x, y); k.set_editor_property("r", float(v)); return k
    def tsc(nm, v, x, y):
        s = Et(unreal.MaterialExpressionScalarParameter, x, y); s.set_editor_property("parameter_name", nm); s.set_editor_property("default_value", float(v)); return s
    def tvc4(nm, c, x, y):
        s = Et(unreal.MaterialExpressionVectorParameter, x, y); s.set_editor_property("parameter_name", nm); s.set_editor_property("default_value", unreal.LinearColor(*c)); return s
    def ttex(nm, t, x, y, stype=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR):
        e = Et(unreal.MaterialExpressionTextureSampleParameter2D, x, y)
        e.set_editor_property("parameter_name", nm); e.set_editor_property("texture", t)
        e.set_editor_property("sampler_type", stype)
        e.set_editor_property("sampler_source", unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)
        return e
    def tmask(src, ch, x, y):
        k = Et(unreal.MaterialExpressionComponentMask, x, y)
        for ch_i in "rgba": k.set_editor_property(ch_i, ch_i in ch)
        C(src, "", k, ""); return k
    def tmul(a, ao, b, bo, x, y):
        k = Et(unreal.MaterialExpressionMultiply, x, y); C(a, ao, k, "A"); C(b, bo, k, "B"); return k
    def tadd(a, ao, b, bo, x, y):
        k = Et(unreal.MaterialExpressionAdd, x, y); C(a, ao, k, "A"); C(b, bo, k, "B"); return k
    def tdiv(a, ao, b, bo, x, y):
        k = Et(unreal.MaterialExpressionDivide, x, y); C(a, ao, k, "A"); C(b, bo, k, "B"); return k
    def tlerp(a, ao, b, bo, al, alo, x, y):
        k = Et(unreal.MaterialExpressionLinearInterpolate, x, y); C(a, ao, k, "A"); C(b, bo, k, "B"); C(al, alo, k, "Alpha"); return k
    def tapp(a, b, x, y):
        k = Et(unreal.MaterialExpressionAppendVector, x, y); C(a, "", k, "A"); C(b, "", k, "B"); return k

    twp = Et(unreal.MaterialExpressionWorldPosition, -2200, 0)
    txy = tmask(twp, "rg", -2000, -100)
    twx = tmask(twp, "r", -2000, 80); twy = tmask(twp, "g", -2000, 150); twz = tmask(twp, "b", -2000, 220)
    txz = tapp(twx, twz, -1840, 100)
    tyz = tapp(twy, twz, -1840, 200)

    tvn = Et(unreal.MaterialExpressionVertexNormalWS, -2000, 400)
    tab = Et(unreal.MaterialExpressionAbs, -1840, 400); C(tvn, "", tab, "")
    tnx = tmask(tab, "r", -1680, 360); tny = tmask(tab, "g", -1680, 430); tnz = tmask(tab, "b", -1680, 500)
    tsum = tadd(tadd(tadd(tnx, "", tny, "", -1520, 400), "", tnz, "", -1380, 400), "", tc1(0.001, -1380, 480), "", -1240, 420)
    wyz = tdiv(tnx, "", tsum, "", -1080, 360)
    wxz = tdiv(tny, "", tsum, "", -1080, 430)
    wxy = tdiv(tnz, "", tsum, "", -1080, 500)

    setts_d = unreal.load_asset(TOWN_DEST + "/T_patterned_cobblestone_02_d") or cob_d
    setts_n = unreal.load_asset(TOWN_DEST + "/T_patterned_cobblestone_02_n") or cob_n
    setts_r = unreal.load_asset(TOWN_DEST + "/T_patterned_cobblestone_02_r") or t_ff_rgh
    ground_d = unreal.load_asset(TOWN_DEST + "/T_aerial_grass_rock_d") or cob_d

    tile_cm = tsc("TileCm", 210.0, -1720, -120)

    def t_tri(nm, t_asset, sc_node, stype, out_pin, y0_t):
        uxy = tdiv(txy, "", sc_node, "", -1500, y0_t)
        uxz = tdiv(txz, "", sc_node, "", -1500, y0_t + 110)
        uyz = tdiv(tyz, "", sc_node, "", -1500, y0_t + 220)
        s1 = ttex(nm, t_asset, -1300, y0_t, stype); C(uxy, "", s1, "UVs")
        s2 = ttex(nm, t_asset, -1300, y0_t + 130, stype); C(uxz, "", s2, "UVs")
        s3 = ttex(nm, t_asset, -1300, y0_t + 260, stype); C(uyz, "", s3, "UVs")
        return tadd(
            tadd(tmul(s1, out_pin, wxy, "", -1060, y0_t), "", tmul(s2, out_pin, wxz, "", -1060, y0_t + 110), "", -900, y0_t + 55),
            "",
            tmul(s3, out_pin, wyz, "", -1060, y0_t + 220),
            "", -760, y0_t + 110
        )

    td = t_tri("Diff", setts_d, tile_cm, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, "RGB", -360)
    tn = t_tri("Nor", setts_n, tile_cm, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, "RGB", 60)
    tr = t_tri("Rough", setts_r, tile_cm, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, "R", 480)

    # Anti-tiling macro modulation
    macro_cm = tsc("MacroCm", 2170.0, -1500, 920)
    swz = tapp(twy, twx, -1680, 920)
    dv_mac = tdiv(swz, "", macro_cm, "", -1320, 920)
    td_mac = ttex("MacroTex", ground_d, -1120, 920); C(dv_mac, "", td_mac, "UVs")
    mac_mod = tmul(td, "", tmul(td_mac, "RGB", tc1(2.2, -920, 960), "", -760, 920), "", -600, 880)
    td_macro = tlerp(td, "", mac_mod, "", tsc("MacroAmt", 0.0, -760, 1040), "", -440, -180)

    # 4K Megascans triplanar micro-pebble/mortar grain & normal within 45 m (rational w = 1 / (1 + (PixelDepth/3500)^2), 0 Clamp nodes)
    tpdepth = Et(unreal.MaterialExpressionPixelDepth, -1500, 1180)
    tdn = tdiv(tpdepth, "", tsc("MicroFadeCm", 3500.0, -1500, 1280), "", -1320, 1200)
    tdsq = tmul(tdn, "", tdn, "", -1160, 1200)
    tw_near = tdiv(tc1(1.0, -1160, 1100), "", tadd(tdsq, "", tc1(1.0, -1160, 1300), "", -1000, 1220), "", -840, 1180)

    micro_cm = tsc("MicroTileCm", 190.0, -1720, 1460)
    tm_alb = t_tri("MicroAlb", t_ff_alb, micro_cm, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, "RGB", 1420)
    tm_nrm = t_tri("MicroNor", t_ff_nrm, micro_cm, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, "RGB", 1840)

    det_mod = tadd(tmul(tm_alb, "", tc1(1.65, -580, 1460), "", -440, 1440), "", tc1(0.56, -440, 1540), "", -300, 1460)
    td_near = tmul(td_macro, "", det_mod, "", -260, -80)
    td_final = tlerp(td_macro, "", td_near, "", tmul(tw_near, "", tc1(0.58, -600, 1240), "", -440, 1240), "", -120, -120)
    ttint = tvc4("Tint", (1.0, 1.0, 1.0, 1), -120, -280)
    MEL.connect_material_property(tmul(td_final, "", ttint, "", 60, -140), "", unreal.MaterialProperty.MP_BASE_COLOR)

    tn_near = tadd(tmul(tn, "", tc1(0.75, -520, 120), "", -360, 120), "", tmul(tm_nrm, "", tc1(0.55, -520, 220), "", -360, 220), "", -200, 160)
    MEL.connect_material_property(tlerp(tn, "", tn_near, "", tw_near, "", 40, 120), "", unreal.MaterialProperty.MP_NORMAL)

    rgh_min = tsc("RoughnessFloor", 0.84, -440, 560)
    rgh_blend = tlerp(tr, "", rgh_min, "", tc1(0.65, -440, 660), "", -160, 520)
    MEL.connect_material_property(rgh_blend, "", unreal.MaterialProperty.MP_ROUGHNESS)

    MEL.recompile_material(mt)
    EAL.save_asset(tile_path)

    # Create MI_IW_DarkStone for weathered dark-grey limestone kerbs, parapets & coping (0.36, 0.35, 0.33, Roughness ~0.86)
    ds_path = TOWN_DEST + "/MI_IW_DarkStone"
    ds = unreal.load_asset(ds_path) if EAL.does_asset_exist(ds_path) else \
        AT.create_asset("MI_IW_DarkStone", TOWN_DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(ds, mt)
    MEL.set_material_instance_scalar_parameter_value(ds, "TileCm", 195.0)
    MEL.set_material_instance_scalar_parameter_value(ds, "RoughnessFloor", 0.86)
    MEL.set_material_instance_vector_parameter_value(ds, "Tint", unreal.LinearColor(0.36, 0.35, 0.33, 1.0))
    MEL.set_material_instance_texture_parameter_value(ds, "Diff", setts_d)
    MEL.set_material_instance_texture_parameter_value(ds, "Nor", setts_n)
    MEL.set_material_instance_texture_parameter_value(ds, "Rough", setts_r)
    EAL.save_asset(ds_path)
    log("M_IW_Tile upgraded to 3-axis Triplanar + 4K rational micro-detail + MI_IW_DarkStone saved")

    # ---------------- 4. Re-tint any blue Barrel01 material to weathered 1870s oak cask ----------------
    for asset_p in EAL.list_assets("/Game/Ironwake/World", recursive=True):
        ap = asset_p.split(".")[0]
        if "Barrel01" in ap:
            obj = unreal.load_asset(ap)
            if isinstance(obj, unreal.MaterialInstanceConstant):
                MEL.set_material_instance_vector_parameter_value(obj, "Color", unreal.LinearColor(0.28, 0.17, 0.09, 1.0))
                MEL.set_material_instance_scalar_parameter_value(obj, "Roughness", 0.86)
                MEL.set_material_instance_scalar_parameter_value(obj, "Metallic", 0.02)
                EAL.save_asset(ap)
                log("Retinted %s -> weathered brown oak" % ap)



m_tex, m_paint = ensure_ship_masters()
mesh_report = import_p28_meshes(m_tex, m_paint)
if not os.environ.get("IWP28_SKIP_MATS"):
    upgrade_ground_materials_4k()

out_json = os.path.join(proj, "Saved/iw_p28_import.json")
if os.environ.get("IWP28_ONLY") and os.path.exists(out_json):
    prev = json.load(open(out_json))
    prev.update(mesh_report)
    mesh_report = prev
with open(out_json, "w") as f:
    json.dump(mesh_report, f, indent=1)
log("DONE %d meshes imported + 4K ground materials upgraded -> %s" % (len(mesh_report), out_json))
unreal.SystemLibrary.quit_editor()
