import unreal
L = lambda m: unreal.log("IWW1 " + str(m))
MEL = unreal.MaterialEditingLibrary
for p in ["/Game/Ironwake/FX/M_IW_Particle", "/Game/Ironwake/FX/M_WakeFoam"]:
    m = unreal.load_asset(p)
    L(f"{p} blend={m.get_editor_property('blend_mode')} shading={m.get_editor_property('shading_model')}")
    for prop in [unreal.MaterialProperty.MP_OPACITY, unreal.MaterialProperty.MP_EMISSIVE_COLOR, unreal.MaterialProperty.MP_BASE_COLOR]:
        n = MEL.get_material_property_input_node(m, prop)
        L(f"  {prop} <- {n.get_class().get_name() if n else None}")
    for e in MEL.get_material_expressions(m) if hasattr(MEL,'get_material_expressions') else []:
        L(f"  expr {e.get_class().get_name()}")
w = unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Ironwake_Sea")
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    c = a.get_class().get_name()
    if "WaterBodyOcean" in c:
        comp = a.get_editor_property("water_body_component") if hasattr(a, "get_editor_property") else None
        try:
            comp = a.get_water_body_component()
        except Exception as e: L(e)
        L(f"ocean {a.get_actor_location()} comp={comp}")
        for k in ["water_material", "height_offset", "max_wave_height_offset", "fixed_water_depth", "collision_height_offset"]:
            try: L(f"  {k} = {comp.get_editor_property(k)}")
            except Exception as e: L(f"  {k} err {e}")
        mat = comp.get_editor_property("water_material")
        try:
            for n in unreal.MaterialEditingLibrary.get_scalar_parameter_names(mat): L(f"  scalar {n} = {unreal.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(mat, n) if isinstance(mat, unreal.MaterialInstance) else '?'}")
        except Exception as e: L(f"scalar err {e}")
        ww = a.get_editor_property("water_waves")
        L(f"  waves {ww}")
        try:
            g = ww.get_editor_property("gerstner_wave_generator")
            for k in ["num_waves","seed","randomness","min_wavelength","max_wavelength","wavelength_falloff","min_amplitude","max_amplitude","amplitude_falloff","wind_angle_deg","direction_angular_spread_deg","small_wave_steepness","large_wave_steepness","steepness_falloff"]:
                L(f"  gen {k} = {g.get_editor_property(k)}")
            L(f"  maxH {ww.get_editor_property('max_wave_height')}")
        except Exception as e: L(f"gen err {e}")
    if "WaterZone" in c or "WaterMesh" in c:
        L(f"zone {c} {a.get_actor_location()}")
        for k in ["zone_extent", "render_target_resolution"]:
            try: L(f"  {k} = {a.get_editor_property(k)}")
            except Exception as e: pass
unreal.SystemLibrary.quit_editor()
