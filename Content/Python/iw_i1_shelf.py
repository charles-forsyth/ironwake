"""I1: M_IW_Shelf (translucent shallow-water tint drawn just under the sea surface around islands: turquoise/sand, vertex
alpha = fade) and M_IW_Surf2 (surf with brighter emissive + vertex-colour-driven tint). New assets only (no graph deletion).
IWTEST_SCRIPT=iw_i1_shelf"""
import unreal
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
DEST = "/Game/Ironwake/Islands"


def E(m, cls, x, y): return MEL.create_material_expression(m, cls, x, y)
def C(a, ao, b, bi): assert MEL.connect_material_expressions(a, ao, b, bi), (ao, bi)


def make(name):
    p = "%s/%s" % (DEST, name)
    if EAL.does_asset_exist(p):
        unreal.log("IWI1H exists %s" % p); return None
    return AT.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())


m = make("M_IW_Shelf")
if m:
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    vc = E(m, unreal.MaterialExpressionVertexColor, -600, 0)
    col = E(m, unreal.MaterialExpressionVectorParameter, -600, -250)
    col.set_editor_property("parameter_name", "ShelfColor"); col.set_editor_property("default_value", unreal.LinearColor(0.10, 0.22, 0.20, 1))
    mul = E(m, unreal.MaterialExpressionMultiply, -300, -200); C(col, "", mul, "A"); C(vc, "", mul, "B")
    MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    op = E(m, unreal.MaterialExpressionScalarParameter, -600, 250)
    op.set_editor_property("parameter_name", "Opacity"); op.set_editor_property("default_value", 0.55)
    om = E(m, unreal.MaterialExpressionMultiply, -300, 150); C(vc, "A", om, "A"); C(op, "", om, "B")
    MEL.connect_material_property(om, "", unreal.MaterialProperty.MP_OPACITY)
    MEL.recompile_material(m); EAL.save_asset("%s/M_IW_Shelf" % DEST); unreal.log("IWI1H shelf OK")

# surf v2: unlit-ish bright foam (emissive carries it in the grey overcast), opacity = foam mask * vertex A
foam = unreal.load_asset("/Water/Textures/Foam/T_WaterFlow_03_Foam_Tiled")
m = make("M_IW_Surf2")
if m:
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    uv = E(m, unreal.MaterialExpressionTextureCoordinate, -1400, 0)
    p1 = E(m, unreal.MaterialExpressionPanner, -1200, -150); C(uv, "", p1, "Coordinate")
    p1.set_editor_property("speed_x", 0.02); p1.set_editor_property("speed_y", 0.05)
    k2 = E(m, unreal.MaterialExpressionConstant, -1400, 250); k2.set_editor_property("r", 2.7)
    uv2 = E(m, unreal.MaterialExpressionMultiply, -1250, 200); C(uv, "", uv2, "A"); C(k2, "", uv2, "B")
    p2 = E(m, unreal.MaterialExpressionPanner, -1100, 200); C(uv2, "", p2, "Coordinate")
    p2.set_editor_property("speed_x", -0.04); p2.set_editor_property("speed_y", 0.03)
    t1 = E(m, unreal.MaterialExpressionTextureSample, -950, -150); t1.set_editor_property("texture", foam); C(p1, "", t1, "UVs")
    t2 = E(m, unreal.MaterialExpressionTextureSample, -950, 200); t2.set_editor_property("texture", foam); C(p2, "", t2, "UVs")
    ad = E(m, unreal.MaterialExpressionAdd, -750, 0); C(t1, "R", ad, "A"); C(t2, "R", ad, "B")
    vc = E(m, unreal.MaterialExpressionVertexColor, -750, 400)
    # mask = saturate((foam + R*1.2 - 0.6) * 2.5)  (R = density: 1 at the rock -> solid white)
    dr = E(m, unreal.MaterialExpressionMultiply, -600, 300); kd = E(m, unreal.MaterialExpressionConstant, -750, 300); kd.set_editor_property("r", 1.2)
    C(vc, "R", dr, "A"); C(kd, "", dr, "B")
    a2 = E(m, unreal.MaterialExpressionAdd, -450, 100); C(ad, "", a2, "A"); C(dr, "", a2, "B")
    th = E(m, unreal.MaterialExpressionScalarParameter, -600, -100); th.set_editor_property("parameter_name", "Thresh"); th.set_editor_property("default_value", 0.6)
    sb = E(m, unreal.MaterialExpressionSubtract, -300, 100); C(a2, "", sb, "A"); C(th, "", sb, "B")
    kc = E(m, unreal.MaterialExpressionScalarParameter, -450, 250); kc.set_editor_property("parameter_name", "Contrast"); kc.set_editor_property("default_value", 2.5)
    mk = E(m, unreal.MaterialExpressionMultiply, -150, 100); C(sb, "", mk, "A"); C(kc, "", mk, "B")
    sat = E(m, unreal.MaterialExpressionSaturate, 0, 100); C(mk, "", sat, "")
    o1 = E(m, unreal.MaterialExpressionMultiply, 150, 100); C(sat, "", o1, "A"); C(vc, "A", o1, "B")
    MEL.connect_material_property(o1, "", unreal.MaterialProperty.MP_OPACITY)
    col = E(m, unreal.MaterialExpressionVectorParameter, 0, -250); col.set_editor_property("parameter_name", "FoamColor")
    col.set_editor_property("default_value", unreal.LinearColor(0.55, 0.58, 0.60, 1))
    MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m); EAL.save_asset("%s/M_IW_Surf2" % DEST); unreal.log("IWI1H surf2 OK")
unreal.log("IWI1H DONE")
unreal.SystemLibrary.quit_editor()
