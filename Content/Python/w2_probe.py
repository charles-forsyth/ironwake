import unreal
L = lambda m: unreal.log("IWW2P " + str(m))
w = unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Ironwake_Sea")
w = unreal.EditorLevelLibrary.get_editor_world()
for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Actor):
    c = a.get_class().get_name()
    if any(k in c for k in ["WaterZone","WaterBody","Landscape","VolumetricCloud","SkyAtmosphere","ExponentialHeightFog","DirectionalLight","SkyLight","PostProcess"]):
        o, e = a.get_actor_bounds(False)
        L(f"{c} {a.get_name()} loc={a.get_actor_location()} ext={e}")
        if "WaterZone" in c:
            try: L(f"  zone_extent={a.get_editor_property('zone_extent')}")
            except Exception as ex: L(ex)
        if "VolumetricCloud" in c:
            cc = a.get_component_by_class(unreal.VolumetricCloudComponent)
            m = cc.get_editor_property("material")
            L(f"  cloud mat={m.get_path_name() if m else None} bottom={cc.get_editor_property('layer_bottom_altitude')} h={cc.get_editor_property('layer_height')}")
            if m:
                for n in unreal.MaterialEditingLibrary.get_scalar_parameter_names(m): L(f"   S {n}")
                for n in unreal.MaterialEditingLibrary.get_vector_parameter_names(m): L(f"   V {n}")
unreal.SystemLibrary.quit_editor()
