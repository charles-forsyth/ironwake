"""S2: import the steam-sail frigate hull (Svjatoi Nikolai, sails/yards stripped) + its materials, and create the
vertex-colour cloth master M_IW_Cloth used by the procedural sails, flags and pennants.
Run headless: IWTEST_SCRIPT=iw_s2_import UnrealEditor IronwakeTest.uproject -log   (quits when done). Idempotent."""
import os, importlib, unreal
os.environ["IW_SHIPS"] = "Frigate"
os.environ["IW_NOQUIT"] = "1"
import iw_r1_import          # noqa: F401  (runs the import for IW_SHIPS)
import iw_r1_materials       # noqa: F401  (material instances for IW_SHIPS)
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
MEL.recompile_material(m)
EAL.save_asset(path)
unreal.log("IWS2 cloth material OK")
fr = unreal.load_asset("/Game/Ironwake/Ships/Frigate/SM_Frigate_Hull")
unreal.log("IWS2 frigate hull %s tris=%s" % (fr is not None, fr.get_num_triangles(0) if fr else -1))
unreal.log("IWS2 DONE")
unreal.SystemLibrary.quit_editor()
