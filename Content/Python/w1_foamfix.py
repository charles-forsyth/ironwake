import unreal
MEL = unreal.MaterialEditingLibrary
m = unreal.load_asset("/Game/Ironwake/FX/M_IW_BowFoam")
for e in MEL.get_material_expressions(m) if hasattr(MEL, "get_material_expressions") else []:
    if isinstance(e, unreal.MaterialExpressionDepthFade):
        e.set_editor_property("fade_distance_default", 8.0)
        unreal.log("IWW1 depth fade -> 8")
MEL.recompile_material(m)
unreal.EditorAssetLibrary.save_asset("/Game/Ironwake/FX/M_IW_BowFoam")
unreal.log("IWW1 foamfix done")
unreal.SystemLibrary.quit_editor()
