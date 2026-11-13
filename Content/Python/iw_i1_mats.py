"""I1: island ground material (vertex-colour blend sand <-> grass/rock, Poly Haven CC0 textures), surf ribbon material
(translucent foam, no depth fade, vertex alpha = edge fade x surge from C++), and the surf_proc.wav loop.
Run headless: IWTEST_SCRIPT=iw_i1_mats UnrealEditor IronwakeTest.uproject -log  (idempotent: rebuilds the two materials)."""
import unreal, os

proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
GROUND = os.path.join(proj, "build/i1/ground")
DEST = "/Game/Ironwake/Islands"
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
FLAT = unreal.load_asset("/Engine/EngineMaterials/FlatNormal")


def L(m): unreal.log("IWI1M " + m)


def import_tex(fn, srgb=True, normal=False):
    name = "T_" + os.path.splitext(os.path.basename(fn))[0]
    path = "%s/%s" % (DEST, name)
    if not EAL.does_asset_exist(path):
        t = unreal.AssetImportTask()
        t.filename = fn; t.destination_path = DEST; t.destination_name = name
        t.automated = True; t.save = True; t.replace_existing = True
        AT.import_asset_tasks([t])
    tex = unreal.load_asset(path)
    tex.set_editor_property("srgb", srgb)
    if normal: tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    elif not srgb: tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    tex.set_editor_property("max_texture_size", 2048)
    EAL.save_asset(path)
    return tex


def fresh(name):
    path = "%s/%s" % (DEST, name)
    if EAL.does_asset_exist(path):
        m = unreal.load_asset(path)
        MEL.delete_all_material_expressions(m)
        return m
    return AT.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())


def E(m, cls, x, y): return MEL.create_material_expression(m, cls, x, y)


def scalar(m, name, v, x, y):
    s = E(m, unreal.MaterialExpressionScalarParameter, x, y)
    s.set_editor_property("parameter_name", name); s.set_editor_property("default_value", v); return s


def vec(m, name, c, x, y):
    s = E(m, unreal.MaterialExpressionVectorParameter, x, y)
    s.set_editor_property("parameter_name", name); s.set_editor_property("default_value", unreal.LinearColor(*c)); return s


def texp(m, name, tex, x, y, kind=None):
    t = E(m, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    t.set_editor_property("parameter_name", name); t.set_editor_property("texture", tex)
    if kind: t.set_editor_property("sampler_type", kind)
    return t


def C(a, ao, b, bi): assert MEL.connect_material_expressions(a, ao, b, bi), (ao, bi)


# ------------------------------------------------------------------ ground
grass_d = import_tex(os.path.join(GROUND, "aerial_grass_rock_Diffuse.png"))
grass_n = import_tex(os.path.join(GROUND, "aerial_grass_rock_nor_gl.png"), False, True)
sand_d = import_tex(os.path.join(GROUND, "coast_sand_01_Diffuse.png"))
sand_n = import_tex(os.path.join(GROUND, "coast_sand_01_nor_gl.png"), False, True)
m = fresh("M_IW_IslandGround")
uv = E(m, unreal.MaterialExpressionTextureCoordinate, -1400, 0)
gt = scalar(m, "GrassTile", 1.0, -1400, -200); st = scalar(m, "SandTile", 2.2, -1400, 200)
uvg = E(m, unreal.MaterialExpressionMultiply, -1200, -150); C(uv, "", uvg, "A"); C(gt, "", uvg, "B")
uvs = E(m, unreal.MaterialExpressionMultiply, -1200, 150); C(uv, "", uvs, "A"); C(st, "", uvs, "B")
gd = texp(m, "GrassTex", grass_d, -1000, -400); C(uvg, "", gd, "UVs")
gn = texp(m, "GrassNrm", grass_n, -1000, -150, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL); C(uvg, "", gn, "UVs")
sd = texp(m, "SandTex", sand_d, -1000, 150); C(uvs, "", sd, "UVs")
sn = texp(m, "SandNrm", sand_n, -1000, 400, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL); C(uvs, "", sn, "UVs")
vc = E(m, unreal.MaterialExpressionVertexColor, -1000, 650)
gtint = vec(m, "GrassTint", (0.62, 0.66, 0.58, 1), -800, -500); stint = vec(m, "SandTint", (0.80, 0.76, 0.68, 1), -800, 250)
g2 = E(m, unreal.MaterialExpressionMultiply, -650, -400); C(gd, "RGB", g2, "A"); C(gtint, "", g2, "B")
s2 = E(m, unreal.MaterialExpressionMultiply, -650, 150); C(sd, "RGB", s2, "A"); C(stint, "", s2, "B")
lb = E(m, unreal.MaterialExpressionLinearInterpolate, -450, -100); C(g2, "", lb, "A"); C(s2, "", lb, "B"); C(vc, "R", lb, "Alpha")
# macro brightness variation from vertex G (0..1 -> 0.6..1.15), B = wet darkening near the waterline
lo = E(m, unreal.MaterialExpressionConstant, -650, 700); lo.set_editor_property("r", 0.6)
hi = E(m, unreal.MaterialExpressionConstant, -650, 780); hi.set_editor_property("r", 1.15)
lv = E(m, unreal.MaterialExpressionLinearInterpolate, -450, 700); C(lo, "", lv, "A"); C(hi, "", lv, "B"); C(vc, "G", lv, "Alpha")
wet = E(m, unreal.MaterialExpressionLinearInterpolate, -450, 880)
one = E(m, unreal.MaterialExpressionConstant, -650, 860); one.set_editor_property("r", 1.0)
wk = E(m, unreal.MaterialExpressionConstant, -650, 940); wk.set_editor_property("r", 0.55)
C(one, "", wet, "A"); C(wk, "", wet, "B"); C(vc, "B", wet, "Alpha")
b1 = E(m, unreal.MaterialExpressionMultiply, -250, 0); C(lb, "", b1, "A"); C(lv, "", b1, "B")
b2 = E(m, unreal.MaterialExpressionMultiply, -100, 0); C(b1, "", b2, "A"); C(wet, "", b2, "B")
MEL.connect_material_property(b2, "", unreal.MaterialProperty.MP_BASE_COLOR)
ln = E(m, unreal.MaterialExpressionLinearInterpolate, -450, 300); C(gn, "RGB", ln, "A"); C(sn, "RGB", ln, "B"); C(vc, "R", ln, "Alpha")
MEL.connect_material_property(ln, "", unreal.MaterialProperty.MP_NORMAL)
rg = E(m, unreal.MaterialExpressionLinearInterpolate, -250, 500)
r1 = E(m, unreal.MaterialExpressionConstant, -450, 500); r1.set_editor_property("r", 0.92)
r2 = E(m, unreal.MaterialExpressionConstant, -450, 560); r2.set_editor_property("r", 0.45)
C(r1, "", rg, "A"); C(r2, "", rg, "B"); C(vc, "B", rg, "Alpha")
MEL.connect_material_property(rg, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(m); EAL.save_asset("%s/M_IW_IslandGround" % DEST); L("ground OK")

# ------------------------------------------------------------------ surf (translucent foam ribbon)
foam = unreal.load_asset("/Water/Textures/Foam/T_WaterFlow_03_Foam_Tiled")
m = fresh("M_IW_Surf")
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
m.set_editor_property("two_sided", True)
m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE)
uv = E(m, unreal.MaterialExpressionTextureCoordinate, -1400, 0)
p1 = E(m, unreal.MaterialExpressionPanner, -1200, -150); C(uv, "", p1, "Coordinate")
p1.set_editor_property("speed_x", 0.015); p1.set_editor_property("speed_y", 0.035)
sc = scalar(m, "Tile2", 2.7, -1400, 250)
uv2 = E(m, unreal.MaterialExpressionMultiply, -1250, 200); C(uv, "", uv2, "A"); C(sc, "", uv2, "B")
p2 = E(m, unreal.MaterialExpressionPanner, -1100, 200); C(uv2, "", p2, "Coordinate")
p2.set_editor_property("speed_x", -0.03); p2.set_editor_property("speed_y", 0.02)
t1 = texp(m, "FoamTex", foam, -950, -150); C(p1, "", t1, "UVs")
t2 = texp(m, "FoamTex2", foam, -950, 200); C(p2, "", t2, "UVs")
# breakup: (t1 + t2) * 0.5 -> contrast
ad = E(m, unreal.MaterialExpressionAdd, -750, 0); C(t1, "R", ad, "A"); C(t2, "R", ad, "B")
vc = E(m, unreal.MaterialExpressionVertexColor, -750, 400)
th = scalar(m, "Thresh", 0.55, -750, 200)
sb = E(m, unreal.MaterialExpressionSubtract, -600, 0); C(ad, "", sb, "A"); C(th, "", sb, "B")
k = scalar(m, "Contrast", 2.2, -600, 150)
mk = E(m, unreal.MaterialExpressionMultiply, -450, 0); C(sb, "", mk, "A"); C(k, "", mk, "B")
# dense white near the rock (vertex R = 1 at the inner edge) blended with the broken-up foam further out
ln = E(m, unreal.MaterialExpressionLinearInterpolate, -300, 0); C(mk, "", ln, "A")
one = E(m, unreal.MaterialExpressionConstant, -450, 120); one.set_editor_property("r", 1.0)
C(one, "", ln, "B"); C(vc, "R", ln, "Alpha")
sat = E(m, unreal.MaterialExpressionSaturate, -150, 0); C(ln, "", sat, "")
op = scalar(m, "Opacity", 1.0, -150, 200)
o1 = E(m, unreal.MaterialExpressionMultiply, 0, 0); C(sat, "", o1, "A"); C(vc, "A", o1, "B")
o2 = E(m, unreal.MaterialExpressionMultiply, 150, 0); C(o1, "", o2, "A"); C(op, "", o2, "B")
MEL.connect_material_property(o2, "", unreal.MaterialProperty.MP_OPACITY)
col = vec(m, "FoamColor", (0.86, 0.89, 0.90, 1), 0, -250)
MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
gl = scalar(m, "Glow", 0.12, 0, -400)
em = E(m, unreal.MaterialExpressionMultiply, 150, -350); C(col, "", em, "A"); C(gl, "", em, "B")
MEL.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
rr = E(m, unreal.MaterialExpressionConstant, 150, -150); rr.set_editor_property("r", 0.8)
MEL.connect_material_property(rr, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(m); EAL.save_asset("%s/M_IW_Surf" % DEST); L("surf OK")

# ------------------------------------------------------------------ surf audio loop
wav = os.path.join(os.environ.get("IRONWAKE_ASSETS", "assets"), "audio/sfx_cc0/surf_proc.wav")
ap = "/Game/Ironwake/Audio/surf_proc"
if not EAL.does_asset_exist(ap):
    t = unreal.AssetImportTask()
    t.filename = wav; t.destination_path = "/Game/Ironwake/Audio"; t.destination_name = "surf_proc"
    t.automated = True; t.save = True; t.replace_existing = True
    AT.import_asset_tasks([t])
s = unreal.load_asset(ap)
if s:
    s.set_editor_property("looping", True); EAL.save_asset(ap); L("surf audio OK %s" % s.get_editor_property("duration"))
else:
    L("surf audio FAILED")
L("DONE")
unreal.SystemLibrary.quit_editor()
