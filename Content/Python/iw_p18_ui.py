import unreal
eal = unreal.EditorAssetLibrary
c = eal.load_asset("/Game/Ironwake/Audio/Classes/SC_UI")
p = c.get_editor_property("properties"); p.set_editor_property("is_ui_sound", True); c.set_editor_property("properties", p)
eal.save_loaded_asset(c)
unreal.log("IWP18 SC_UI is_ui_sound=%s" % c.get_editor_property("properties").get_editor_property("is_ui_sound"))
unreal.SystemLibrary.quit_editor()
