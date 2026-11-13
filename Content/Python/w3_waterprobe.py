"""Two-layer bug: list every water / landscape / plane actor on Ironwake_Sea with Z + extents, sample landscape height on a grid."""
import unreal
L = lambda m: unreal.log("IWWP " + str(m))
unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Ironwake_Sea")
w = unreal.EditorLevelLibrary.get_editor_world()
for a in unreal.EditorLevelLibrary.get_all_level_actors():
    c = a.get_class().get_name()
    if any(k in c for k in ("Water", "Landscape", "StaticMeshActor", "Ocean", "Fog", "PostProcess")):
        o, e = a.get_actor_bounds(False)
        L(f"{c} {a.get_actor_label()} loc={a.get_actor_location()} bounds_o={o} ext={e} hidden={a.is_hidden_ed()}")
        if "StaticMeshActor" in c:
            smc = a.static_mesh_component
            L(f"   mesh={smc.static_mesh.get_name() if smc.static_mesh else None} mats={[m.get_name() if m else None for m in smc.get_materials()]}")
for x in range(-400000, 400001, 100000):
    row = []
    for y in range(-400000, 400001, 100000):
        hit = unreal.SystemLibrary.line_trace_single(w, unreal.Vector(x, y, 50000), unreal.Vector(x, y, -50000), unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
        if hit:
            t = hit.to_tuple(); row.append(f"{t[4].z:.0f}:{t[9].get_name() if t[9] else '-'}"[:28])
        else: row.append("none")
    L(f"x={x} " + " | ".join(row))
unreal.SystemLibrary.quit_editor()
