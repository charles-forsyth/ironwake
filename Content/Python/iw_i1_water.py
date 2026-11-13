"""I1: inspect water zone / ocean extents on Ironwake_Sea (read-only unless IWI1_SETEXT=<metres full width>)."""
import unreal, os
L = lambda m: unreal.log("IWI1W " + m)
w = unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Ironwake_Sea")
world = unreal.EditorLevelLibrary.get_editor_world()
new = os.environ.get("IWI1_SETEXT")
for a in unreal.EditorLevelLibrary.get_all_level_actors():
    c = a.get_class().get_name()
    if c in ("WaterZone",):
        for p in ["zone_extent", "render_target_resolution", "enable_local_only_tessellation", "local_tessellation_extent"]:
            try: L("%s %s %s = %s" % (c, a.get_name(), p, a.get_editor_property(p)))
            except Exception as e: L("%s %s ERR %s" % (c, p, e))
        L("zone loc %s scale %s" % (a.get_actor_location(), a.get_actor_scale3d()))
        if new:
            v = float(new) * 100.0
            a.set_editor_property("zone_extent", unreal.Vector2D(v, v)); L("zone extent set")
    if c.startswith("WaterBodyOcean"):
        comp = a.get_editor_property("water_body_component") if hasattr(a, "water_body_component") else None
        comp = comp or a.get_component_by_class(unreal.WaterBodyOceanComponent)
        for p in ["ocean_extents", "collision_extents", "height_offset"]:
            try: L("ocean %s = %s" % (p, comp.get_editor_property(p)))
            except Exception as e: L("ocean %s ERR %s" % (p, e))
        L("ocean loc %s" % a.get_actor_location())
        if new:
            v = float(new) * 100.0
            comp.set_editor_property("ocean_extents", unreal.Vector2D(v, v))
            L("ocean extents set")
if new:
    L("saving")
    ok = unreal.EditorLevelLibrary.save_current_level()
    L("saved new extent %s m" % new)
L("DONE")
unreal.SystemLibrary.quit_editor()
