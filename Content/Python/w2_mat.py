import unreal
L = lambda m: unreal.log("IWW2M " + str(m))
MEL = unreal.MaterialEditingLibrary
mi = unreal.load_asset('/Game/Ironwake/Water/MI_Ocean_Storm')
L(f"ocean foam switch before {MEL.get_material_instance_static_switch_parameter_value(mi, 'Enable Ocean Foam')}")
MEL.set_material_instance_static_switch_parameter_value(mi, 'Enable Ocean Foam', True)
# kill the cyan subsurface glow: grey-green scattering, foam scattering white
for n, c in [("Scattering", (0.012, 0.016, 0.018, 1)), ("Foam Scattering", (0.85, 0.87, 0.88, 1)), ("Foam Emissive", (0.0, 0.0, 0.0, 1))]:
    try: MEL.set_material_instance_vector_parameter_value(mi, n, unreal.LinearColor(*c)); L(f"set {n}")
    except Exception as e: L(f"{n} {e}")
for n, v in [("Foam Opacity", 1.0), ("Foam powr", 0.6), ("Foam Texture Blend Min", 0.25)]:
    MEL.set_material_instance_scalar_parameter_value(mi, n, v)
MEL.update_material_instance(mi)
unreal.EditorAssetLibrary.save_asset('/Game/Ironwake/Water/MI_Ocean_Storm')
L(f"ocean foam switch after {MEL.get_material_instance_static_switch_parameter_value(mi, 'Enable Ocean Foam')}")
unreal.SystemLibrary.quit_editor()
