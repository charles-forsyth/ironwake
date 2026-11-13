import unreal
MEL = unreal.MaterialEditingLibrary
m = unreal.load_asset("/Game/Ironwake/Water/M_IW_WaterRoot")
unreal.log("IWP19 numexpr %d" % len(MEL.get_material_expressions(m)))
e0 = MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
unreal.log("IWP19 top " + e0.get_name())
tmp = unreal.load_asset("/Game/Ironwake/Water/M_IW_WaterRoot")
# throwaway material to test names
pkg = "/Game/Ironwake/Water/M_P19_Test"
if unreal.EditorAssetLibrary.does_asset_exist(pkg): unreal.EditorAssetLibrary.delete_asset(pkg)
t = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_P19_Test", "/Game/Ironwake/Water", unreal.Material, unreal.MaterialFactoryNew())
br = MEL.create_material_expression(t, unreal.MaterialExpressionBreakMaterialAttributes, 0, 0)
mk = MEL.create_material_expression(t, unreal.MaterialExpressionMakeMaterialAttributes, 300, 0)
unreal.log("IWP19 break outs: " + str(MEL.get_material_expression_output_names(br)))
unreal.log("IWP19 make ins: " + str(MEL.get_material_expression_input_names(mk)))
unreal.log("IWP19 bl ins: " + str(MEL.get_material_expression_input_names(MEL.create_material_expression(t, unreal.MaterialExpressionBlendMaterialAttributes, 0, 300))))
unreal.log("IWP19 cm: " + str([p for p in dir(unreal.MaterialExpressionComponentMask) if not p.startswith('_')][:10]))
unreal.EditorAssetLibrary.delete_asset(pkg)
unreal.log("IWP19 done")
unreal.SystemLibrary.quit_editor()
