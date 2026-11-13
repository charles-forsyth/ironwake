"""W1 assets: M_IW_BowFoam (lit translucent foam, vertex-alpha, world-aligned foam texture, depth fade), water loop sounds, NS_IW_BowSpray (dup of splash)."""
import unreal, os
L = lambda m: unreal.log("IWW1 " + str(m))
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
P = "/Game/Ironwake/FX/M_IW_BowFoam"
if EAL.does_asset_exist(P):
    EAL.delete_asset(P)
m = AT.create_asset("M_IW_BowFoam", "/Game/Ironwake/FX", unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
m.set_editor_property("two_sided", True)
try: m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_VOLUMETRIC_NON_DIRECTIONAL)
except Exception as e: L(f"tlm {e}")
tex = unreal.load_asset("/Water/Textures/Foam/T_WaterFlow_03_Foam_Tiled")
L(f"foam tex {tex}")
def X(cls, x, y): return MEL.create_material_expression(m, cls, x, y)
wp = X(unreal.MaterialExpressionWorldPosition, -1400, 0)
msk = X(unreal.MaterialExpressionComponentMask, -1250, 0); msk.set_editor_property("r", True); msk.set_editor_property("g", True); msk.set_editor_property("b", False); msk.set_editor_property("a", False)
MEL.connect_material_expressions(wp, "", msk, "")
def uvscale(div, x, y, sx, sy):
    d = X(unreal.MaterialExpressionDivide, x, y); d.set_editor_property("const_b", div)
    MEL.connect_material_expressions(msk, "", d, "A")
    pn = X(unreal.MaterialExpressionPanner, x + 150, y); pn.set_editor_property("speed_x", sx); pn.set_editor_property("speed_y", sy)
    MEL.connect_material_expressions(d, "", pn, "Coordinate")
    t = X(unreal.MaterialExpressionTextureSample, x + 320, y); t.set_editor_property("texture", tex)
    MEL.connect_material_expressions(pn, "", t, "UVs")
    return t
t1 = uvscale(900.0, -1100, -150, 0.01, 0.004)
t2 = uvscale(2300.0, -1100, 150, -0.006, 0.008)
mul12 = X(unreal.MaterialExpressionMultiply, -600, 0)
MEL.connect_material_expressions(t1, "R", mul12, "A"); MEL.connect_material_expressions(t2, "R", mul12, "B")
add12 = X(unreal.MaterialExpressionAdd, -480, 0)
MEL.connect_material_expressions(t1, "R", add12, "A"); MEL.connect_material_expressions(mul12, "", add12, "B")
vc = X(unreal.MaterialExpressionVertexColor, -600, 250)
# op = VC.A * (0.3 + 1.4 * foam)
k = X(unreal.MaterialExpressionMultiply, -350, 0); k.set_editor_property("const_b", 1.4)
MEL.connect_material_expressions(add12, "", k, "A")
k2 = X(unreal.MaterialExpressionAdd, -250, 0); k2.set_editor_property("const_b", 0.3)
MEL.connect_material_expressions(k, "", k2, "A")
k3 = X(unreal.MaterialExpressionMultiply, -150, 100)
MEL.connect_material_expressions(k2, "", k3, "A"); MEL.connect_material_expressions(vc, "A", k3, "B")
op = X(unreal.MaterialExpressionScalarParameter, -150, 300); op.set_editor_property("parameter_name", "Opacity"); op.set_editor_property("default_value", 1.0)
k4 = X(unreal.MaterialExpressionMultiply, 0, 100)
MEL.connect_material_expressions(k3, "", k4, "A"); MEL.connect_material_expressions(op, "", k4, "B")
sat = X(unreal.MaterialExpressionSaturate, 120, 100)
MEL.connect_material_expressions(k4, "", sat, "")
df = X(unreal.MaterialExpressionDepthFade, 260, 100); df.set_editor_property("fade_distance_default", 120.0)
MEL.connect_material_expressions(sat, "", df, "Opacity")
MEL.connect_material_property(df, "", unreal.MaterialProperty.MP_OPACITY)
# Base colour: slightly blue-grey foam, brighter where the foam texture is dense.
c0 = X(unreal.MaterialExpressionConstant3Vector, -250, -300); c0.set_editor_property("constant", unreal.LinearColor(0.55, 0.6, 0.63, 1))
c1 = X(unreal.MaterialExpressionConstant3Vector, -250, -200); c1.set_editor_property("constant", unreal.LinearColor(0.92, 0.94, 0.95, 1))
lp = X(unreal.MaterialExpressionLinearInterpolate, 0, -250)
MEL.connect_material_expressions(c0, "", lp, "A"); MEL.connect_material_expressions(c1, "", lp, "B"); MEL.connect_material_expressions(t1, "R", lp, "Alpha")
MEL.connect_material_property(lp, "", unreal.MaterialProperty.MP_BASE_COLOR)
r = X(unreal.MaterialExpressionConstant, 0, -100); r.set_editor_property("r", 0.85)
MEL.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(m)
EAL.save_asset(P)
L("material saved")

# Sounds: import the three water loops, looping.
src = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Audio")
tasks = []
for n in ["bow_wash_loop", "hull_wake_loop", "slow_lap_loop"]:
    t = unreal.AssetImportTask(); t.filename = os.path.join(src, n + ".wav"); t.destination_path = "/Game/Ironwake/Audio"; t.destination_name = n
    t.automated = True; t.save = True; t.replace_existing = True; tasks.append(t)
AT.import_asset_tasks(tasks)
for n in ["bow_wash_loop", "hull_wake_loop", "slow_lap_loop"]:
    s = unreal.load_asset("/Game/Ironwake/Audio/" + n)
    if s:
        s.set_editor_property("looping", True)
        EAL.save_asset("/Game/Ironwake/Audio/" + n)
    L(f"sound {n} {s} dur={s.get_editor_property('duration') if s else None}")

if not EAL.does_asset_exist("/Game/Ironwake/FX/NS_IW_BowSpray"):
    EAL.duplicate_asset("/Game/Ironwake/FX/NS_IW_Splash", "/Game/Ironwake/FX/NS_IW_BowSpray")
    EAL.save_asset("/Game/Ironwake/FX/NS_IW_BowSpray")
L("assets done")
