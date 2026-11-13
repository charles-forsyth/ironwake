"""Copy Epic's working water sample into our project as /Game/Maps/OceanBase, then take
screenshots from a PILOTED camera via HighResShot (the only capture path proven to write files).
One heading per settle window; quits cleanly."""
import unreal

LOG = lambda m: unreal.log(f"IWTEST {m}")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

les.load_level("/WaterExtras/Maps/WaterTestMap")
acts = eas.get_all_level_actors()
ocean = next(a for a in acts if "WaterBodyOcean" in a.get_class().get_name())
oo, oe = ocean.get_actor_bounds(False)
z = ocean.get_actor_location().z
# Ocean sits beyond the island: put the camera well out over open water, looking back at the island and out to sea
cam = eas.spawn_actor_from_class(unreal.CineCameraActor, unreal.Vector(30000, 0, 600), unreal.Rotator(0, -4, 180))
cam.set_actor_label("ShotCam")
les.pilot_level_actor(cam)
try:
    ok = les.save_current_level_as("/Game/Maps/OceanBase")
except Exception as e:  # noqa: BLE001
    ok = f"err {e}"
LOG(f"saved copy as /Game/Maps/OceanBase: {ok}")

YAWS = [180, 0, 90]
S = {"t": 0.0, "i": 0, "next": 40.0, "done": False}


def tick(dt):
    if S["done"]:
        return
    S["t"] += dt
    if S["t"] < S["next"]:
        return
    i = S["i"]
    if i < len(YAWS):
        if S.get("aimed") != i:
            cam.set_actor_rotation(unreal.Rotator(0, -4, YAWS[i]), False)
            S["aimed"] = i
            S["next"] = S["t"] + 5
            return
        unreal.SystemLibrary.execute_console_command(None, "HighResShot 1600x900")
        LOG(f"shot yaw {YAWS[i]}")
        S["i"] = i + 1
        S["next"] = S["t"] + 6
    else:
        S["done"] = True
        unreal.unregister_slate_post_tick_callback(H)
        LOG("quitting")
        unreal.SystemLibrary.quit_editor()


H = unreal.register_slate_post_tick_callback(tick)
