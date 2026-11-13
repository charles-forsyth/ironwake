"""P12: plaster/roof master with per-colour tint and subtle world-space noise. Headless, idempotent."""
import unreal
DEST = "/Game/Ironwake/Town"
MEL = unreal.MaterialEditingLibrary; AT = unreal.AssetToolsHelpers.get_asset_tools(); EAL = unreal.EditorAssetLibrary
L = lambda m: unreal.log("IWP12M " + str(m))
path = DEST + "/M_IW_Plaster"
if EAL.does_asset_exist(path): EAL.delete_asset(path)
m = AT.create_asset("M_IW_Plaster", DEST, unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("used_with_instanced_static_meshes", True)
E = lambda c, x, y: MEL.create_material_expression(m, c, x, y)
col = E(unreal.MaterialExpressionVectorParameter, -700, -100); col.set_editor_property("parameter_name", "Color"); col.set_editor_property("default_value", unreal.LinearColor(0.6, 0.55, 0.45, 1))
wp = E(unreal.MaterialExpressionWorldPosition, -900, 150)
nz = E(unreal.MaterialExpressionNoise, -700, 150)
nz.set_editor_property("scale", 0.035); nz.set_editor_property("levels", 3); nz.set_editor_property("output_min", 0.0); nz.set_editor_property("output_max", 1.0)
L("noise pos %s" % MEL.connect_material_expressions(wp, "", nz, "Position"))
k1 = E(unreal.MaterialExpressionConstant, -500, 250); k1.set_editor_property("r", 0.40)
mu = E(unreal.MaterialExpressionMultiply, -350, 170)
MEL.connect_material_expressions(nz, "", mu, "A"); MEL.connect_material_expressions(k1, "", mu, "B")
k2 = E(unreal.MaterialExpressionConstant, -350, 280); k2.set_editor_property("r", 0.78)
ad = E(unreal.MaterialExpressionAdd, -200, 200)
MEL.connect_material_expressions(mu, "", ad, "A"); MEL.connect_material_expressions(k2, "", ad, "B")
bc = E(unreal.MaterialExpressionMultiply, -50, 0)
MEL.connect_material_expressions(col, "", bc, "A"); MEL.connect_material_expressions(ad, "", bc, "B")
L("bc %s" % MEL.connect_material_property(bc, "", unreal.MaterialProperty.MP_BASE_COLOR))
ro = E(unreal.MaterialExpressionConstant, -50, 200); ro.set_editor_property("r", 0.88)
L("ro %s" % MEL.connect_material_property(ro, "", unreal.MaterialProperty.MP_ROUGHNESS))
MEL.recompile_material(m); EAL.save_asset(path); L("DONE")
unreal.SystemLibrary.quit_editor()
