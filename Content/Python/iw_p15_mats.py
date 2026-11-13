"""P15: hillside ground master (4 vertex-weighted layers: grass / earth / scree / rock-triplanar, world-aligned, macro noise) and road master
(cobble <-> dirt by edge wear). Poly Haven CC0 1k textures from build/assets/p15. Headless, idempotent (creates new assets)."""
import unreal, os
proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(proj, "build/assets/p15"); DEST = "/Game/Ironwake/Town"
MEL = unreal.MaterialEditingLibrary; AT = unreal.AssetToolsHelpers.get_asset_tools(); EAL = unreal.EditorAssetLibrary
L = lambda m: unreal.log("IWP15M " + str(m))


def imp(fn, name, kind):
    path = "%s/%s" % (DEST, name)
    if not EAL.does_asset_exist(path):
        t = unreal.AssetImportTask(); t.filename = os.path.join(SRC, fn); t.destination_path = DEST; t.destination_name = name
        t.automated = True; t.save = True; t.replace_existing = True
        AT.import_asset_tasks([t])
    tex = unreal.load_asset(path)
    if tex is None: raise RuntimeError("import failed " + fn)
    if kind == "nor":
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP); tex.set_editor_property("srgb", False)
    tex.set_editor_property("max_texture_size", 1024)
    EAL.save_asset(path); return tex


def fresh(name):
    p = "%s/%s" % (DEST, name)
    if EAL.does_asset_exist(p): EAL.delete_asset(p)
    return AT.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())


def C(a, ao, b, bi): assert MEL.connect_material_expressions(a, ao, b, bi), (ao, bi)


TX = {}
for tag, a in (("Grass", "grass_ground"), ("Earth", "brown_mud_dry"), ("Scree", "gray_rocks"), ("Rock", "aerial_rocks_02"), ("Dirt", "dirt_aerial_03")):
    TX[tag] = (imp(a + "_diff.jpg", "T_hill_%s_d" % a, "diff"), imp(a + "_nor_gl.jpg", "T_hill_%s_n" % a, "nor"))
cob_d = unreal.load_asset(DEST + "/T_cobblestone_floor_02_d"); cob_n = unreal.load_asset(DEST + "/T_cobblestone_floor_02_n")

# ---------------------------------------------------------------- hill master
m = fresh("M_IW_Hill")
E = lambda c, x, y: MEL.create_material_expression(m, c, x, y)


def sc(name, v, x, y):
    s = E(unreal.MaterialExpressionScalarParameter, x, y); s.set_editor_property("parameter_name", name); s.set_editor_property("default_value", v); return s


def vc4(name, c, x, y):
    s = E(unreal.MaterialExpressionVectorParameter, x, y); s.set_editor_property("parameter_name", name); s.set_editor_property("default_value", unreal.LinearColor(*c)); return s


def tex(name, t, x, y, normal=False):
    e = E(unreal.MaterialExpressionTextureSampleParameter2D, x, y); e.set_editor_property("parameter_name", name); e.set_editor_property("texture", t)
    e.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if normal else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    e.set_editor_property("sampler_source", unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS); return e


def mask(src, ch, x, y):
    k = E(unreal.MaterialExpressionComponentMask, x, y)
    for c in "rgba": k.set_editor_property(c, c in ch)
    C(src, "", k, ""); return k


def mul(a, ao, b, bo, x, y):
    k = E(unreal.MaterialExpressionMultiply, x, y); C(a, ao, k, "A"); C(b, bo, k, "B"); return k


def add(a, b, x, y):
    k = E(unreal.MaterialExpressionAdd, x, y); C(a, "", k, "A"); C(b, "", k, "B"); return k


wp = E(unreal.MaterialExpressionWorldPosition, -2000, 0)
xy = mask(wp, "rg", -1800, -100); xz = None
wx = mask(wp, "r", -1800, 100); wy = mask(wp, "g", -1800, 180); wz = mask(wp, "b", -1800, 260)
app = lambda a, b, x, y: (lambda k: (C(a, "", k, "A"), C(b, "", k, "B"), k)[2])(E(unreal.MaterialExpressionAppendVector, x, y))
uvxz = app(wx, wz, -1650, 120); uvyz = app(wy, wz, -1650, 220)
vcol = E(unreal.MaterialExpressionVertexColor, -1700, 600)
layers = {}
y0 = -900
for tag, tile in (("Grass", 520.0), ("Earth", 380.0), ("Scree", 300.0)):
    t = sc(tag + "Tile", tile, -1500, y0)
    dv = E(unreal.MaterialExpressionDivide, -1350, y0); C(xy, "", dv, "A"); C(t, "", dv, "B")
    d = tex(tag + "Diff", TX[tag][0], -1150, y0); C(dv, "", d, "UVs")
    n = tex(tag + "Nor", TX[tag][1], -1150, y0 + 200, True); C(dv, "", n, "UVs")
    tn = vec = vc4(tag + "Tint", {"Grass": (0.46, 0.78, 0.26, 1), "Earth": (0.95, 0.85, 0.70, 1), "Scree": (1.0, 1.0, 1.0, 1)}[tag], -1150, y0 - 150)
    layers[tag] = (mul(d, "RGB", tn, "", -900, y0), n)
    y0 += 450
# rock: triplanar-ish blend of the two vertical projections by the vertex normal
rt = sc("RockTile", 650.0, -1500, y0)
dxz = E(unreal.MaterialExpressionDivide, -1350, y0); C(uvxz, "", dxz, "A"); C(rt, "", dxz, "B")
dyz = E(unreal.MaterialExpressionDivide, -1350, y0 + 100); C(uvyz, "", dyz, "A"); C(rt, "", dyz, "B")
r1 = tex("RockDiff", TX["Rock"][0], -1150, y0); C(dxz, "", r1, "UVs")
r2 = E(unreal.MaterialExpressionTextureSampleParameter2D, -1150, y0 + 250); r2.set_editor_property("parameter_name", "RockDiff"); r2.set_editor_property("texture", TX["Rock"][0]); r2.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR); r2.set_editor_property("sampler_source", unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS); C(dyz, "", r2, "UVs")
vn = E(unreal.MaterialExpressionVertexNormalWS, -1500, y0 + 400)
ab = E(unreal.MaterialExpressionAbs, -1350, y0 + 400); C(vn, "", ab, "")
nx = mask(ab, "r", -1200, y0 + 380); ny = mask(ab, "g", -1200, y0 + 450)
den = add(nx, ny, -1050, y0 + 400)
eps = E(unreal.MaterialExpressionConstant, -1050, y0 + 500); eps.set_editor_property("r", 0.001)
den2 = add(den, eps, -900, y0 + 420)
tb = E(unreal.MaterialExpressionDivide, -750, y0 + 420); C(ny, "", tb, "A"); C(den2, "", tb, "B")
rl = E(unreal.MaterialExpressionLinearInterpolate, -600, y0 + 100); C(r2, "RGB", rl, "A"); C(r1, "RGB", rl, "B"); C(tb, "", rl, "Alpha")
rtint = vc4("RockTint", (0.95, 0.93, 0.90, 1), -600, y0 - 80)
layers["Rock"] = (mul(rl, "", rtint, "", -450, y0 + 100), None)
# blend: sum(weight_k * colour_k)
def vpin(pin, y):
    k = E(unreal.MaterialExpressionComponentMask, -700, y)
    for c in "rgba": k.set_editor_property(c, False)
    k.set_editor_property("r", True); C(vcol, pin, k, ""); return k
cr, cg, cb, ca = (vpin(pin, 700 + 70 * i) for i, pin in enumerate(("R", "G", "B", "A")))
acc = None
for w, k in zip((cr, cg, cb, ca), ("Grass", "Earth", "Scree", "Rock")):
    t = mul(layers[k][0], "", w, "", -300, 300 + 70 * len(k))
    acc = t if acc is None else add(acc, t, -150, 300)
# macro brightness (two incommensurate noise scales on world position)
def macro(name, tile, x, y, swap):
    t = sc(name, tile, x - 200, y)
    d = E(unreal.MaterialExpressionDivide, x - 100, y); C(uvyz if swap else xy, "", d, "A"); C(t, "", d, "B")
    e = tex(name + "T", TX["Earth"][0], x, y); C(d, "", e, "UVs")
    g = mask(e, "g", x + 120, y); return g
n1 = macro("Macro1Cm", 9000.0, -900, 1100, False)
n2 = macro("Macro2Cm", 23000.0, -900, 1250, True)
ns = add(n1, n2, -520, 1170)
k1 = E(unreal.MaterialExpressionConstant, -520, 1280); k1.set_editor_property("r", 0.42)
nm = mul(ns, "", k1, "", -380, 1200)
k2 = E(unreal.MaterialExpressionConstant, -380, 1300); k2.set_editor_property("r", 0.72)
br = add(nm, k2, -240, 1240)
cf = mul(acc, "", br, "", 0, 600)
C(cf, "", E(unreal.MaterialExpressionSaturate, 0, 0), "") if False else None
MEL.connect_material_property(cf, "", unreal.MaterialProperty.MP_BASE_COLOR)
# normal: weighted blend of grass/earth/scree normals (rock uses the flat normal: its albedo carries the detail)
nacc = None
for w, k in zip((cr, cg, cb), ("Grass", "Earth", "Scree")):
    t = mul(layers[k][1], "RGB", w, "", -300, 1500); nacc = t if nacc is None else add(nacc, t, -150, 1500)
MEL.connect_material_property(nacc, "", unreal.MaterialProperty.MP_NORMAL)
ro = E(unreal.MaterialExpressionConstant, 0, 800); ro.set_editor_property("r", 0.93)
MEL.connect_material_property(ro, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(m); EAL.save_asset(DEST + "/M_IW_Hill"); L("hill master saved")
try:
    st = MEL.get_statistics(m); L("hill stats: samplers=%s vs=%s ps=%s" % (st.num_texture_sample_expressions if hasattr(st, "num_texture_sample_expressions") else "?", getattr(st, "num_vertex_shader_instructions", "?"), getattr(st, "num_pixel_shader_instructions", "?")))
except Exception as ex: L("stats err %s" % ex)

# ---------------------------------------------------------------- road master
m = fresh("M_IW_Road")
E = lambda c, x, y: MEL.create_material_expression(m, c, x, y)
wp = E(unreal.MaterialExpressionWorldPosition, -1400, 0)
xy = mask(wp, "rg", -1250, 0)
t1 = sc("CobTile", 210.0, -1250, 150); dv = E(unreal.MaterialExpressionDivide, -1100, 0); C(xy, "", dv, "A"); C(t1, "", dv, "B")
t2 = sc("DirtTile", 330.0, -1250, 300); dv2 = E(unreal.MaterialExpressionDivide, -1100, 200); C(xy, "", dv2, "A"); C(t2, "", dv2, "B")
cd = tex("CobDiff", cob_d, -900, -200); C(dv, "", cd, "UVs"); cn = tex("CobNor", cob_n, -900, 0, True); C(dv, "", cn, "UVs")
dd = tex("DirtDiff", TX["Dirt"][0], -900, 250); C(dv2, "", dd, "UVs"); dn = tex("DirtNor", TX["Dirt"][1], -900, 450, True); C(dv2, "", dn, "UVs")
vcol = E(unreal.MaterialExpressionVertexColor, -900, 700)
we = mask(vcol, "r", -700, 700)
pd = sc("DirtAmt", 0.0, -700, 800)
sm = add(we, pd, -520, 740)
sat = E(unreal.MaterialExpressionSaturate, -380, 740); C(sm, "", sat, "")
lb = E(unreal.MaterialExpressionLinearInterpolate, -200, 0); C(cd, "RGB", lb, "A"); C(dd, "RGB", lb, "B"); C(sat, "", lb, "Alpha")
tint = vc4("Tint", (1.0, 1.0, 1.0, 1), -200, -250)
bc = mul(lb, "", tint, "", 0, 0)
MEL.connect_material_property(bc, "", unreal.MaterialProperty.MP_BASE_COLOR)
ln = E(unreal.MaterialExpressionLinearInterpolate, -200, 300); C(cn, "RGB", ln, "A"); C(dn, "RGB", ln, "B"); C(sat, "", ln, "Alpha")
MEL.connect_material_property(ln, "", unreal.MaterialProperty.MP_NORMAL)
ro = E(unreal.MaterialExpressionConstant, 0, 500); ro.set_editor_property("r", 0.88)
MEL.connect_material_property(ro, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(m); EAL.save_asset(DEST + "/M_IW_Road"); L("road master saved")


def mi(name, parent, scal, vec):
    p = "%s/%s" % (DEST, name)
    if EAL.does_asset_exist(p): EAL.delete_asset(p)
    x = AT.create_asset(name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    x.set_editor_property("parent", parent)
    for k, v in scal.items(): MEL.set_material_instance_scalar_parameter_value(x, k, v)
    for k, v in vec.items(): MEL.set_material_instance_vector_parameter_value(x, k, unreal.LinearColor(*v))
    EAL.save_asset(p); L("mi " + name)


mi("MI_IW_Hill", unreal.load_asset(DEST + "/M_IW_Hill"), {}, {})
mi("MI_IW_Road", unreal.load_asset(DEST + "/M_IW_Road"), {"DirtAmt": 0.0}, {"Tint": (1.05, 1.0, 0.95, 1)})
mi("MI_IW_Path", unreal.load_asset(DEST + "/M_IW_Road"), {"DirtAmt": 1.0}, {"Tint": (1.0, 0.95, 0.88, 1)})
L("DONE")
unreal.SystemLibrary.quit_editor()
