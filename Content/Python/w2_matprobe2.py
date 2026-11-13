import unreal
L = lambda m: unreal.log("IWMP " + str(m))
MEL = unreal.MaterialEditingLibrary
m = unreal.load_asset('/Water/Materials/WaterSurface/Water_Material')
L(f"useattrs {m.get_editor_property('use_material_attributes')} nexpr {MEL.get_num_material_expressions(m)}")
for p in ["MP_MATERIAL_ATTRIBUTES","MP_BASE_COLOR","MP_OPACITY","MP_NORMAL","MP_WORLD_POSITION_OFFSET","MP_ROUGHNESS"]:
    try:
        n = MEL.get_material_property_input_node(m, getattr(unreal.MaterialProperty, p))
        L(f"{p} -> {n.get_class().get_name() if n else None} {n.get_name() if n else ''} out={MEL.get_material_property_input_node_output_name(m, getattr(unreal.MaterialProperty, p)) if n else ''}")
        if n:
            for i in MEL.get_inputs_for_material_expression(m, n):
                L(f"    in {i.get_class().get_name() if i else None}")
    except Exception as e: L(f"{p} err {e}")
mo = unreal.load_asset('/Water/Materials/WaterSurface/Water_Material_Ocean')
for s in ["Enable Ocean Foam"]:
    try: L(f"{s} {MEL.get_material_instance_static_switch_parameter_value(mo, s)}")
    except Exception as e: L(e)
oc = None
w = unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Ironwake_Sea")
for a in unreal.GameplayStatics.get_all_actors_of_class(unreal.EditorLevelLibrary.get_editor_world(), unreal.Actor):
    if a.get_class().get_name() == "WaterBodyOcean":
        c = a.get_component_by_class(unreal.WaterBodyComponent)
        for prop in ["water_material","water_hlod_material","water_static_mesh_material","underwater_post_process_material","water_info_material"]:
            try: L(f"{prop} = {c.get_editor_property(prop)}")
            except Exception as e: L(f"{prop} err")
    if "WaterZone" in a.get_class().get_name():
        for prop in ["far_distance_material","far_distance_mesh_extent"]:
            try: L(f"zone {prop} = {a.get_editor_property(prop)}")
            except Exception as e: L(f"zone {prop} err {e}")
