import unreal
L = lambda m: unreal.log("IWMP " + str(m))
MEL = unreal.MaterialEditingLibrary
mi = unreal.load_asset('/Game/Ironwake/Water/MI_Ocean_Storm')
p = mi
chain=[]
while isinstance(p, unreal.MaterialInstance):
    chain.append(p.get_path_name()); p = p.get_editor_property("parent")
chain.append(p.get_path_name()); L(f"chain {chain}")
m = p
L(f"shading {m.get_editor_property('shading_model')} blend {m.get_editor_property('blend_mode')}")
seen=set()
def walk(n, d, maxd):
    if n is None or d>maxd: return
    key=n.get_path_name()
    desc=n.get_class().get_name()
    for prop in ["parameter_name","desc","function","input_name","name"]:
        try:
            v=n.get_editor_property(prop)
            if v: desc+=f" {prop}={v.get_name() if hasattr(v,'get_name') else v}"
        except Exception: pass
    L("  "*d + desc + (" (seen)" if key in seen else ""))
    if key in seen: return
    seen.add(key)
    try: ins = MEL.get_inputs_for_material_expression(m, n)
    except Exception as e: ins=[]; L(e)
    for i in ins: walk(i, d+1, maxd)
for prop in [unreal.MaterialProperty.MP_BASE_COLOR, unreal.MaterialProperty.MP_ROUGHNESS, unreal.MaterialProperty.MP_OPACITY]:
    L(f"=== {prop}")
    seen.clear()
    walk(MEL.get_material_property_input_node(m, prop), 0, 7)
L("scalars " + str(MEL.get_scalar_parameter_names(m)))
L("vectors " + str(MEL.get_vector_parameter_names(m)))
