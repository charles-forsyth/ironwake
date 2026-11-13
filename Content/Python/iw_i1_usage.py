"""I1: set bUsedWithInstancedStaticMeshes on the masters used by HISM instances (M_IW_ShipTex2, M_IW_ShipPaint, M_IW_Foliage,
M_IW_IslandGround) and re-save. Without it the cooked game draws instanced rocks/trees with the default material.
IWTEST_SCRIPT=iw_i1_usage"""
import unreal
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
for p in ["/Game/Ironwake/Ships/Materials/M_IW_ShipTex2", "/Game/Ironwake/Ships/Materials/M_IW_ShipPaint",
          "/Game/Ironwake/Islands/M_IW_Foliage", "/Game/Ironwake/Islands/M_IW_IslandGround"]:
    m = unreal.load_asset(p)
    before = m.get_editor_property("used_with_instanced_static_meshes")
    m.set_editor_property("used_with_instanced_static_meshes", True)
    MEL.recompile_material(m)
    EAL.save_asset(p)
    unreal.log("IWI1U %s ism %s -> %s" % (p, before, m.get_editor_property("used_with_instanced_static_meshes")))
unreal.log("IWI1U DONE")
unreal.SystemLibrary.quit_editor()
