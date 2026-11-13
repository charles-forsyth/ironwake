import unreal
L = lambda m: unreal.log("IWP5C " + str(m))
w = unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Ironwake_Sea")
for a in unreal.EditorLevelLibrary.get_all_level_actors():
    c = a.get_class().get_name()
    if c == "WaterZone":
        for p in ["far_distance_material","far_distance_mesh_extent","far_distance_mesh_height_offset","zone_extent","tessellation_factor" ]:
            try: L("zone %s = %s" % (p, a.get_editor_property(p)))
            except Exception as e: L("zone %s ERR" % p)
        L("loc %s" % a.get_actor_location())
    if c.startswith("WaterBodyOcean"):
        comp = a.get_component_by_class(unreal.WaterBodyOceanComponent)
        for p in ["water_material","ocean_extents"]:
            try: L("ocean %s = %s" % (p, comp.get_editor_property(p)))
            except Exception as e: L("ocean %s ERR" % p)
L("DONE")
unreal.SystemLibrary.quit_editor()
