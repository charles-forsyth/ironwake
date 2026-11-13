import unreal
L = lambda m: unreal.log("IWMP " + str(m))
MEL = unreal.MaterialEditingLibrary
m = unreal.load_asset('/Game/Ironwake/Water/M_IW_WaterRoot')
n = MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
L(f"attrs <- {n.get_class().get_name() if n else None}")
if n:
    for i in MEL.get_inputs_for_material_expression(m, n): L(f"  in {i.get_class().get_name() if i else None}")
s = unreal.load_asset('/Game/Ironwake/Water/MI_Ocean_Storm')
p = s
while isinstance(p, unreal.MaterialInstance): L(p.get_path_name()); p = p.get_editor_property("parent")
L(p.get_path_name())
L(f"scalars on storm: {[x for x in MEL.get_scalar_parameter_names(s) if 'IW' in str(x)]}")
# find SLW output node
for e in unreal.MaterialEditingLibrary.get_used_textures(m)[:3]: L(f"tex {e.get_name()}")
