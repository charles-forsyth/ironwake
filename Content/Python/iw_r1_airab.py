import unreal
EAL = unreal.EditorAssetLibrary; MEL = unreal.MaterialEditingLibrary
p = "/Game/Ironwake/Ships/BrassAirship/MI_BrassAirship_tripo_mat_5ec429a9"
mi = unreal.load_asset(p)
MEL.set_material_instance_texture_parameter_value(mi, "NormalTex", unreal.load_asset("/Engine/EngineMaterials/FlatNormal"))
MEL.set_material_instance_texture_parameter_value(mi, "MRTex", unreal.load_asset("/Game/Ironwake/Ships/Materials/T_IW_WhiteMask"))
MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", 0.55)
MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", 0.0)
EAL.save_asset(p)
unreal.log("IWAIRAB flat normal + no MR applied")
unreal.SystemLibrary.quit_editor()
