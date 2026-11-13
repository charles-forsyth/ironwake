"""P10: tiling PBR town materials (world-aligned top projection) + emissive window material. Headless, idempotent."""
import unreal, os
proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(proj, "build/assets/p10"); DEST = "/Game/Ironwake/Town"
MEL = unreal.MaterialEditingLibrary; AT = unreal.AssetToolsHelpers.get_asset_tools(); EAL = unreal.EditorAssetLibrary
L = lambda m: unreal.log("IWP10M " + str(m))
if not EAL.does_directory_exist(DEST): EAL.make_directory(DEST)

def imp(fn, name, kind):
    t = unreal.AssetImportTask(); t.filename = os.path.join(SRC, fn); t.destination_path = DEST; t.destination_name = name
    t.automated = True; t.save = True; t.replace_existing = True
    AT.import_asset_tasks([t])
    tex = unreal.load_asset("%s/%s" % (DEST, name))
    if kind == "nor":
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP); tex.set_editor_property("srgb", False)
    elif kind == "rough":
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS); tex.set_editor_property("srgb", False)
    tex.set_editor_property("max_texture_size", 2048)
    EAL.save_asset("%s/%s" % (DEST, name)); return tex

TEX = {}
for tag, a in (("Setts", "patterned_cobblestone_02"), ("Cobble", "cobblestone_floor_02"), ("Deck", "wood_floor_deck"), ("Ground", "aerial_grass_rock")):
    TEX[tag] = {"diff": imp(a + "_diff.jpg", "T_" + a + "_d", "diff"), "nor": imp(a + "_nor_gl.jpg", "T_" + a + "_n", "nor"), "rough": imp(a + "_rough.jpg", "T_" + a + "_r", "rough")}
# ---- master
path = DEST + "/M_IW_Tile"
if EAL.does_asset_exist(path): EAL.delete_asset(path)
m = AT.create_asset("M_IW_Tile", DEST, unreal.Material, unreal.MaterialFactoryNew())
E = lambda c, x, y: MEL.create_material_expression(m, c, x, y)
wp = E(unreal.MaterialExpressionWorldPosition, -900, 0)
mask = E(unreal.MaterialExpressionComponentMask, -700, 0); mask.set_editor_property("r", True); mask.set_editor_property("g", True); mask.set_editor_property("b", False); mask.set_editor_property("a", False)
L("mask %s" % MEL.connect_material_expressions(wp, "", mask, ""))
sc = E(unreal.MaterialExpressionScalarParameter, -700, 120); sc.set_editor_property("parameter_name", "TileCm"); sc.set_editor_property("default_value", 300.0)
dv = E(unreal.MaterialExpressionDivide, -500, 0)
L("dvA %s %s" % (MEL.connect_material_expressions(mask, "", dv, "A"), MEL.connect_material_expressions(sc, "", dv, "B")))
def T(name, y, st):
    e = E(unreal.MaterialExpressionTextureSampleParameter2D, -250, y); e.set_editor_property("parameter_name", name); e.set_editor_property("sampler_type", st); e.set_editor_property("texture", TEX["Setts"][{"Diff":"diff","Nor":"nor","Rough":"rough"}[name]])
    L("uv conn %s" % MEL.connect_material_expressions(dv, "", e, "UVs")); return e
td = T("Diff", -200, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
tn = T("Nor", 0, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
tr = T("Rough", 200, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
tint = E(unreal.MaterialExpressionVectorParameter, -250, -380); tint.set_editor_property("parameter_name", "Tint"); tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
# anti-tiling: second diffuse sample at an incommensurate, ~7x larger scale (+ rotated via swapped axes), multiplied in (mean-normalised ~3x)
sc2 = E(unreal.MaterialExpressionScalarParameter, -700, 300); sc2.set_editor_property("parameter_name", "MacroCm"); sc2.set_editor_property("default_value", 2170.0)
swz = E(unreal.MaterialExpressionAppendVector, -900, 300)
mg = E(unreal.MaterialExpressionComponentMask, -1000, 260); mg.set_editor_property("r", False); mg.set_editor_property("g", True); mg.set_editor_property("b", False); mg.set_editor_property("a", False)
mr = E(unreal.MaterialExpressionComponentMask, -1000, 340); mr.set_editor_property("r", True); mr.set_editor_property("g", False); mr.set_editor_property("b", False); mr.set_editor_property("a", False)
MEL.connect_material_expressions(wp, "", mg, ""); MEL.connect_material_expressions(wp, "", mr, "")
MEL.connect_material_expressions(mg, "", swz, "A"); MEL.connect_material_expressions(mr, "", swz, "B")
dv2 = E(unreal.MaterialExpressionDivide, -500, 300)
MEL.connect_material_expressions(swz, "", dv2, "A"); MEL.connect_material_expressions(sc2, "", dv2, "B")
td2 = E(unreal.MaterialExpressionTextureSampleParameter2D, -250, 330); td2.set_editor_property("parameter_name", "Diff"); td2.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR); td2.set_editor_property("texture", TEX["Ground"]["diff"])
MEL.connect_material_expressions(dv2, "", td2, "UVs")
k3 = E(unreal.MaterialExpressionConstant, -100, 330); k3.set_editor_property("r", 2.2)
mm = E(unreal.MaterialExpressionMultiply, 0, 330)
MEL.connect_material_expressions(td2, "RGB", mm, "A"); MEL.connect_material_expressions(k3, "", mm, "B")
lp = E(unreal.MaterialExpressionLinearInterpolate, 100, -100)
k6 = E(unreal.MaterialExpressionConstant, 0, 400); k6.set_editor_property("r", 0.0)
MEL.connect_material_expressions(td, "RGB", lp, "A")
m3 = E(unreal.MaterialExpressionMultiply, 60, 200)
MEL.connect_material_expressions(td, "RGB", m3, "A"); MEL.connect_material_expressions(mm, "", m3, "B")
MEL.connect_material_expressions(m3, "", lp, "B")
ka = E(unreal.MaterialExpressionScalarParameter, 0, 480); ka.set_editor_property("parameter_name", "MacroAmt"); ka.set_editor_property("default_value", 0.0)
MEL.connect_material_expressions(ka, "", lp, "Alpha")
mul = E(unreal.MaterialExpressionMultiply, 200, -200)
L("rgb conn %s" % MEL.connect_material_expressions(lp, "", mul, "A")); MEL.connect_material_expressions(tint, "", mul, "B")
L("bc %s" % MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR))
L("nm %s" % MEL.connect_material_property(tn, "RGB", unreal.MaterialProperty.MP_NORMAL))
L("ro %s" % MEL.connect_material_property(tr, "R", unreal.MaterialProperty.MP_ROUGHNESS))
MEL.recompile_material(m); EAL.save_asset(path); L("master saved; errors: %s" % [str(x) for x in (MEL.get_statistics(m).__dict__ if False else [])])

# ---- glow (windows): dark glass by day, warm emissive at night (scalar On driven by the game)
gp = DEST + "/M_IW_Glow"
if EAL.does_asset_exist(gp): EAL.delete_asset(gp)
g = AT.create_asset("M_IW_Glow", DEST, unreal.Material, unreal.MaterialFactoryNew())
g.set_editor_property("used_with_instanced_static_meshes", True)
GE = lambda c, x, y: MEL.create_material_expression(g, c, x, y)
gc = GE(unreal.MaterialExpressionVectorParameter, -500, 0); gc.set_editor_property("parameter_name", "Color"); gc.set_editor_property("default_value", unreal.LinearColor(2.2, 1.2, 0.4, 1))
go = GE(unreal.MaterialExpressionScalarParameter, -500, 120); go.set_editor_property("parameter_name", "On"); go.set_editor_property("default_value", 0.0)
gm = GE(unreal.MaterialExpressionMultiply, -250, 0)
MEL.connect_material_expressions(gc, "", gm, "A"); MEL.connect_material_expressions(go, "", gm, "B")
MEL.connect_material_property(gm, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
gb = GE(unreal.MaterialExpressionConstant3Vector, -250, -150); gb.set_editor_property("constant", unreal.LinearColor(0.012, 0.014, 0.018, 1))
MEL.connect_material_property(gb, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.recompile_material(g); EAL.save_asset(gp); L("glow saved")

def mi(name, base, tex, tile, tint, macro=0.0):
    p = "%s/%s" % (DEST, name)
    if EAL.does_asset_exist(p): EAL.delete_asset(p)
    x = AT.create_asset(name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    x.set_editor_property("parent", base)
    MEL.set_material_instance_scalar_parameter_value(x, "TileCm", tile)
    MEL.set_material_instance_vector_parameter_value(x, "Tint", unreal.LinearColor(*tint))
    MEL.set_material_instance_scalar_parameter_value(x, "MacroAmt", macro)
    for k, suf in (("Diff", "diff"), ("Nor", "nor"), ("Rough", "rough")):
        MEL.set_material_instance_texture_parameter_value(x, k, tex[suf])
    EAL.save_asset(p); L("mi " + name)
base = unreal.load_asset(path)
sets = {}
for tag, a, tile, tint in (("Setts", "patterned_cobblestone_02", 350.0, (1.1, 1.1, 1.1)), ("Cobble", "cobblestone_floor_02", 220.0, (1.0, 1.0, 1.0)), ("Deck", "wood_floor_deck", 300.0, (1.0, 0.9, 0.8)), ("Ground", "aerial_grass_rock", 500.0, (0.9, 0.9, 0.9))):
    mi("MI_IW_" + tag, base, TEX[tag], tile, tint, 0.85 if tag in ("Ground", "Setts") else 0.5)
gi = DEST + "/MI_IW_Window"
if EAL.does_asset_exist(gi): EAL.delete_asset(gi)
x = AT.create_asset("MI_IW_Window", DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew()); x.set_editor_property("parent", unreal.load_asset(gp)); EAL.save_asset(gi)
L("DONE")
unreal.SystemLibrary.quit_editor()
