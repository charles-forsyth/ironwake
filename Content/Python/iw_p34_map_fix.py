"""P34: Upgrade Ironwake_Sea WaterZone (36x36 km, 2048x2048 RT, local tessellation around camera) and ExponentialHeightFog (crystal clear maritime visibility, zero grey fog-of-war)."""
import unreal, os

L = lambda m: unreal.log("IWP34_MAP " + str(m))
MAP = "/Game/Maps/Ironwake_Sea"
unreal.EditorLoadingAndSavingUtils.load_map(MAP)

for a in unreal.EditorLevelLibrary.get_all_level_actors():
    cn = a.get_class().get_name()
    if cn == "WaterZone":
        a.set_editor_property("zone_extent", unreal.Vector2D(3600000.0, 3600000.0))
        a.set_editor_property("render_target_resolution", unreal.IntPoint(2048, 2048))
        try:
            a.set_editor_property("enable_local_only_tessellation", True)
            a.set_editor_property("local_tessellation_extent", unreal.Vector(420000.0, 420000.0, 16000.0))
        except Exception as ex:
            L(f"WZ local_tess note: {ex}")
        try:
            a.mark_package_dirty()
        except Exception:
            pass
        L(f"Updated WaterZone: extent={a.get_editor_property('zone_extent')} rt={a.get_editor_property('render_target_resolution')}")
    elif cn == "ExponentialHeightFog":
        fc = a.get_component_by_class(unreal.ExponentialHeightFogComponent)
        if fc:
            fc.set_editor_property("fog_density", 0.0028)
            fc.set_editor_property("fog_height_falloff", 0.0045)
            fc.set_editor_property("start_distance", 180000.0)
            fc.set_editor_property("fog_max_opacity", 0.42)
            fc.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.42, 0.56, 0.78, 1.0))
            L(f"Updated ExponentialHeightFog: density={fc.get_editor_property('fog_density')} start={fc.get_editor_property('start_distance')} maxOp={fc.get_editor_property('fog_max_opacity')}")

ok = unreal.EditorLoadingAndSavingUtils.save_current_level()
L(f"Saved Ironwake_Sea: {ok}")
if not os.environ.get("IW_NOQUIT"):
    unreal.SystemLibrary.quit_editor()
