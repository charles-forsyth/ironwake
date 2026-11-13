"""Diagnose + fix ocean extent and camera placement, then screenshot."""
import unreal

LOG = lambda m: unreal.log(f"IWTEST {m}")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/Maps/StormTest")

acts = {a.get_actor_label(): a for a in eas.get_all_level_actors()}
for name, a in acts.items():
    o, e = a.get_actor_bounds(False)
    LOG(f"{name} {a.get_class().get_name()} loc={a.get_actor_location()} scale={a.get_actor_scale3d()} "
        f"bounds_origin={o} extent={e}")

ocean = acts.get("Ocean")
zone = acts.get("WaterZone")
if zone:
    for p in ["zone_extent", "render_target_resolution", "extent"]:
        try:
            LOG(f"zone {p} = {zone.get_editor_property(p)}")
        except Exception as ex:  # noqa: BLE001
            pass
    zone.set_actor_scale3d(unreal.Vector(1, 1, 1))
    try:
        zone.set_editor_property("zone_extent", unreal.Vector2D(2000000, 2000000))  # 20 km
        LOG("zone_extent set 20km")
    except Exception as ex:  # noqa: BLE001
        LOG(f"WARN zone_extent {ex}")

if ocean:
    sp = ocean.get_component_by_class(unreal.WaterSplineComponent)
    if sp:
        n = sp.get_number_of_spline_points()
        pts = [sp.get_location_at_spline_point(i, unreal.SplineCoordinateSpace.WORLD) for i in range(n)]
        LOG(f"ocean spline {n} pts: {pts}")
        # Replace with a big square 16 km across, centred on origin
        R = 800000.0
        sp.clear_spline_points(False)
        for x, y in [(-R, -R), (R, -R), (R, R), (-R, R)]:
            sp.add_spline_point(unreal.Vector(x, y, 0), unreal.SplineCoordinateSpace.WORLD, False)
        sp.set_closed_loop(True, True)
        sp.update_spline()
        LOG("ocean spline replaced with 16 km square")
    for p in ["collision_extents", "height_offset"]:
        try:
            LOG(f"ocean {p} = {ocean.get_editor_property(p)}")
        except Exception:  # noqa: BLE001
            pass
    ocean.set_actor_location(unreal.Vector(0, 0, 0), False, False)

cam = acts.get("ShotCam")
cam.set_actor_location(unreal.Vector(0, 0, 900), False, False)
cam.set_actor_rotation(unreal.Rotator(0, -6, 30), False)
les.pilot_level_actor(cam)
les.save_current_level()
LOG("fix applied, saved")

state = {"t": 0.0, "n": 0}


def tick(dt):
    state["t"] += dt
    if state["n"] == 0 and state["t"] > 30:
        unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080, "IWTEST_storm_c.png", cam)
        LOG("screenshot c")
        state["n"] = 1
    if state["n"] == 1 and state["t"] > 40:
        unreal.unregister_slate_post_tick_callback(h)
        unreal.SystemLibrary.quit_editor()


h = unreal.register_slate_post_tick_callback(tick)
