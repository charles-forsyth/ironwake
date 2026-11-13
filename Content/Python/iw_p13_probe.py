import unreal
L = lambda m: unreal.log("IWP13 " + str(m))
w = unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Ironwake_Sea")
w = unreal.EditorLevelLibrary.get_editor_world()
for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Actor):
    c = a.get_class().get_name()
    if "PostProcess" in c:
        s = a.get_editor_property("settings")
        L(f"PPV {a.get_name()} unbound={a.get_editor_property('unbound')}")
        for p in ["color_saturation","color_gain","color_gamma","color_contrast","color_offset","scene_color_tint","white_temp","auto_exposure_bias","film_slope","film_toe","film_shoulder","vignette_intensity","bloom_intensity","color_saturation_shadows","color_gain_shadows","color_gain_highlights","color_saturation_highlights"]:
            try: L(f"  {p} = {s.get_editor_property(p)}")
            except Exception as e: L(f"  {p} ERR")
unreal.SystemLibrary.quit_editor()
