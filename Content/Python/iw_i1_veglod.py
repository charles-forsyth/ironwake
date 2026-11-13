"""I1: vegetation meshes: Nanite OFF (Nanite masked foliage rendered bare branches at range on Metal) and auto LODs
(4 LODs, 50/25/12 %) so HISM foliage stays cheap. IWTEST_SCRIPT=iw_i1_veglod"""
import unreal
EAL = unreal.EditorAssetLibrary
sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
for n in ["IslandTree01", "IslandTree02", "IslandTree03", "FirTree01", "Shrub02", "Shrub03", "GrassMedium01"]:
    p = "/Game/Ironwake/World/%s/SM_%s" % (n, n)
    m = unreal.load_asset(p)
    ns = sub.get_nanite_settings(m); ns.enabled = False; sub.set_nanite_settings(m, ns, True)
    o = unreal.EditorScriptingMeshReductionOptions()
    o.auto_compute_lod_screen_size = True
    o.reduction_settings = [unreal.EditorScriptingMeshReductionSettings(1.0, 1.0), unreal.EditorScriptingMeshReductionSettings(0.5, 0.5),
                            unreal.EditorScriptingMeshReductionSettings(0.25, 0.25), unreal.EditorScriptingMeshReductionSettings(0.12, 0.12)]
    k = sub.set_lods(m, o)
    EAL.save_asset(p)
    unreal.log("IWI1V %s nanite=%s lods=%s tris0=%d" % (n, sub.get_nanite_settings(m).enabled, k, m.get_num_triangles(0)))
unreal.log("IWI1V DONE")
unreal.SystemLibrary.quit_editor()
