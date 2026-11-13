import unreal
L = lambda m: unreal.log("IWW1 " + str(m))
unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Ironwake_Sea")
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    c = a.get_class().get_name()
    if "WaterBodyOcean" in c or "WaterZone" in c or "Landscape" in c or "WaterBrush" in c:
        L(f"{c} {a.get_actor_label()} loc={a.get_actor_location()} bounds={a.get_actor_bounds(False)}")
    if "WaterBodyOcean" in c:
        comp = a.get_water_body_component()
        for k in ["target_wave_mask_depth", "ocean_extents", "wave_attenuation_factor", "affects_landscape", "water_zone_override", "overlap_material_priority"]:
            try: L(f"  {k} = {comp.get_editor_property(k)}")
            except Exception as e: L(f"  {k} err {str(e)[:80]}")
unreal.SystemLibrary.quit_editor()
