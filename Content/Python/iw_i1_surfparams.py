"""I1: tweak M_IW_Surf scalar defaults in place (rebuilding with delete_all_material_expressions crashes on a loaded asset).
IWTEST_SCRIPT=iw_i1_surfparams   env IWI1_SURF="Glow=0.12,Thresh=0.55"."""
import unreal, os
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
p = "/Game/Ironwake/Islands/M_IW_Surf"
m = unreal.load_asset(p)
want = dict(kv.split("=") for kv in os.environ.get("IWI1_SURF", "Glow=0.12,Thresh=0.55").split(","))
for e in MEL.get_material_expressions(m) if hasattr(MEL, "get_material_expressions") else []:
    if isinstance(e, unreal.MaterialExpressionScalarParameter):
        n = str(e.get_editor_property("parameter_name"))
        if n in want:
            e.set_editor_property("default_value", float(want[n])); unreal.log("IWI1S %s = %s" % (n, want[n]))
MEL.recompile_material(m)
EAL.save_asset(p)
unreal.log("IWI1S DONE")
unreal.SystemLibrary.quit_editor()
