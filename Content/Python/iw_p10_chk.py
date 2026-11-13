import unreal
MEL=unreal.MaterialEditingLibrary
for n in ("MI_IW_Ground","MI_IW_Setts","MI_IW_Window"):
    m=unreal.load_asset("/Game/Ironwake/Town/"+n)
    p=m.get_editor_property("parent")
    unreal.log("IWP10 chk %s parent=%s tex=%s" % (n, p.get_name() if p else None, MEL.get_material_instance_texture_parameter_value(m,"Diff") if n!="MI_IW_Window" else "-"))
m=unreal.load_asset("/Game/Ironwake/Town/M_IW_Tile")
unreal.log("IWP10 chk master exprs=%d" % len(MEL.get_used_textures(m)) if hasattr(MEL,'get_used_textures') else "IWP10 chk n/a")
unreal.log("IWP10 chk usedtex %s" % [t.get_name() for t in MEL.get_used_textures(m)])
unreal.SystemLibrary.quit_editor()
