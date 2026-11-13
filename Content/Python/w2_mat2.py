import unreal
MEL = unreal.MaterialEditingLibrary
mi = unreal.load_asset('/Game/Ironwake/Water/MI_Ocean_Storm')
for n, v in [("Foam Distance", 30000.0), ("Foam MacroScale", 600.0), ("Front Foam Scale", 300.0), ("FoamContrast", 1.6), ("Foam Dpeth Min", 0.0), ("Foam Texture Blend Width", 0.6)]:
    MEL.set_material_instance_scalar_parameter_value(mi, n, v)
MEL.update_material_instance(mi)
unreal.EditorAssetLibrary.save_asset('/Game/Ironwake/Water/MI_Ocean_Storm')
unreal.log("IWW2M mat2 done")
unreal.SystemLibrary.quit_editor()
