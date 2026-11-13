"""W4: M_IW_Spray, a soft spray/spume sprite material for Niagara (replaces the hard-dot M_IW_Particle on NS_IW_Spume and the
bow sheets). Unlit translucent: colour = particle colour, opacity = particle alpha x soft radial falloff x drifting foam
texture breakup x depth fade (no hard line where a sprite meets the sea). Creates the asset only if missing; re-running
rebuilds nothing (no graph deletes: they crashed the editor before)."""
import unreal, os
L = lambda m: unreal.log("IWW4S " + str(m))
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
DEST = "/Game/Ironwake/FX"; NAME = "M_IW_Spray"
def E(m, cls, x, y): return MEL.create_material_expression(m, cls, x, y)
def C(a, ao, b, bi): assert MEL.connect_material_expressions(a, ao, b, bi), (ao, bi)
if EAL.does_asset_exist(f"{DEST}/{NAME}"):
    L("exists")
else:
    m = AT.create_asset(NAME, DEST, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("used_with_niagara_sprites", True)
    m.set_editor_property("used_with_niagara_ribbons", True)
    pc = E(m, unreal.MaterialExpressionParticleColor, -900, -200)
    uv = E(m, unreal.MaterialExpressionTextureCoordinate, -1400, 200)
    # radial: saturate(1 - 2*|uv - 0.5|)^1.6 (soft edge, no visible quad)
    half = E(m, unreal.MaterialExpressionConstant2Vector, -1400, 350); half.set_editor_property("r", 0.5); half.set_editor_property("g", 0.5)
    sub = E(m, unreal.MaterialExpressionSubtract, -1200, 250); C(uv, "", sub, "A"); C(half, "", sub, "B")
    ln = E(m, unreal.MaterialExpressionLength, -1050, 250); C(sub, "", ln, "")
    two = E(m, unreal.MaterialExpressionConstant, -1050, 350); two.set_editor_property("r", 2.0)
    mu = E(m, unreal.MaterialExpressionMultiply, -900, 250); C(ln, "", mu, "A"); C(two, "", mu, "B")
    om = E(m, unreal.MaterialExpressionOneMinus, -780, 250); C(mu, "", om, "")
    sa = E(m, unreal.MaterialExpressionSaturate, -660, 250); C(om, "", sa, "")
    ex = E(m, unreal.MaterialExpressionConstant, -660, 350); ex.set_editor_property("r", 1.6)
    pw = E(m, unreal.MaterialExpressionPower, -540, 250); C(sa, "", pw, "Base"); C(ex, "", pw, "Exp")
    # breakup: foam texture, panned, 0.35 + 0.9*tex
    foam = unreal.load_asset("/Water/Textures/Foam/T_WaterFlow_03_Foam_Tiled")
    pn = E(m, unreal.MaterialExpressionPanner, -1200, 500); C(uv, "", pn, "Coordinate")
    pn.set_editor_property("speed_x", 0.08); pn.set_editor_property("speed_y", 0.03)
    tx = E(m, unreal.MaterialExpressionTextureSample, -1000, 500); tx.set_editor_property("texture", foam); C(pn, "", tx, "UVs")
    k9 = E(m, unreal.MaterialExpressionConstant, -1000, 650); k9.set_editor_property("r", 0.9)
    tm = E(m, unreal.MaterialExpressionMultiply, -820, 520); C(tx, "R", tm, "A"); C(k9, "", tm, "B")
    k35 = E(m, unreal.MaterialExpressionConstant, -820, 650); k35.set_editor_property("r", 0.35)
    ta = E(m, unreal.MaterialExpressionAdd, -680, 520); C(tm, "", ta, "A"); C(k35, "", ta, "B")
    o1 = E(m, unreal.MaterialExpressionMultiply, -400, 300); C(pw, "", o1, "A"); C(ta, "", o1, "B")
    o2 = E(m, unreal.MaterialExpressionMultiply, -280, 250); C(o1, "", o2, "A"); C(pc, "A", o2, "B")
    df = E(m, unreal.MaterialExpressionDepthFade, -150, 250); C(o2, "", df, "Opacity")
    dfd = E(m, unreal.MaterialExpressionConstant, -280, 400); dfd.set_editor_property("r", 60.0); C(dfd, "", df, "FadeDistance")
    sat2 = E(m, unreal.MaterialExpressionSaturate, 0, 250); C(df, "", sat2, "")
    MEL.connect_material_property(sat2, "", unreal.MaterialProperty.MP_OPACITY)
    MEL.connect_material_property(pc, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m)
    EAL.save_asset(f"{DEST}/{NAME}")
    L("created")
if os.environ.get("IWTEST_SCRIPT") == "w4_spray": unreal.SystemLibrary.quit_editor()
