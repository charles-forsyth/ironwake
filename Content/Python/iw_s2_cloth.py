"""S2: (re)create M_IW_Cloth only (two-sided foliage canvas). IWTEST_SCRIPT=iw_s2_cloth"""
import os, unreal
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
path = "/Game/Ironwake/Ships/Materials/M_IW_Cloth"
if EAL.does_asset_exist(path):
    EAL.delete_asset(path)
m = AT.create_asset("M_IW_Cloth", "/Game/Ironwake/Ships/Materials", unreal.Material, unreal.MaterialFactoryNew())
vc = MEL.create_material_expression(m, unreal.MaterialExpressionVertexColor, -600, -100)
tint = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -600, 150)
tint.set_editor_property("parameter_name", "Tint"); tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
mul = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 0)
MEL.connect_material_expressions(vc, "", mul, "A"); MEL.connect_material_expressions(tint, "", mul, "B")
MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
r = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -300, 200); r.set_editor_property("r", 0.88)
MEL.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
m.set_editor_property("two_sided", True)
# light passes through canvas: the windward face seen from astern glows instead of rendering black
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
ss = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 350)
k = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -500, 420); k.set_editor_property("r", 0.85)
MEL.connect_material_expressions(mul, "", ss, "A"); MEL.connect_material_expressions(k, "", ss, "B")
MEL.connect_material_property(ss, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
MEL.recompile_material(m)
EAL.save_asset(path)
unreal.log("IWS2 cloth material OK")
fr = None and unreal.load_asset("/Game/Ironwake/Ships/Frigate/SM_Frigate_Hull")
unreal.log("IWS2 frigate hull %s tris=%s" % (fr is not None, -1))
unreal.log("IWS2 DONE")
unreal.SystemLibrary.quit_editor()
