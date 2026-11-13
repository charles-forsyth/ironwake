"""P12: unlit translucent grey material for thin chimney smoke."""
import unreal
DEST = "/Game/Ironwake/Town"
MEL = unreal.MaterialEditingLibrary; AT = unreal.AssetToolsHelpers.get_asset_tools(); EAL = unreal.EditorAssetLibrary
path = DEST + "/M_IW_Smk"
if EAL.does_asset_exist(path): EAL.delete_asset(path)
m = AT.create_asset("M_IW_Smk", DEST, unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("used_with_instanced_static_meshes", True)
E = lambda c, x, y: MEL.create_material_expression(m, c, x, y)
c = E(unreal.MaterialExpressionConstant3Vector, -400, 0); c.set_editor_property("constant", unreal.LinearColor(0.55, 0.55, 0.57, 1))
o = E(unreal.MaterialExpressionScalarParameter, -400, 150); o.set_editor_property("parameter_name", "Op"); o.set_editor_property("default_value", 0.12)
unreal.log("IWP12M e %s" % MEL.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR))
unreal.log("IWP12M o %s" % MEL.connect_material_property(o, "", unreal.MaterialProperty.MP_OPACITY))
MEL.recompile_material(m); EAL.save_asset(path); unreal.log("IWP12M DONE")
unreal.SystemLibrary.quit_editor()
