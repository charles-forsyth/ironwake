"""Open Epic's WaterExtras sample map, list its water actors, screenshot from several cameras, quit."""
import unreal

LOG = lambda m: unreal.log(f"IWTEST {m}")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)

les.load_level("/WaterExtras/Maps/WaterTestMap")
acts = eas.get_all_level_actors()
LOG(f"actors: {len(acts)}")
for a in acts:
    c = a.get_class().get_name()
    if any(k in c for k in ["Water", "Landscape", "Ocean", "Lake", "River", "Brush", "Camera", "PlayerStart"]):
        o, e = a.get_actor_bounds(False)
        LOG(f"{a.get_actor_label()} [{c}] loc={a.get_actor_location().to_tuple() if hasattr(a.get_actor_location(),'to_tuple') else a.get_actor_location()} ext=({e.x:.0f},{e.y:.0f},{e.z:.0f})")

ocean = next((a for a in acts if "WaterBodyOcean" in a.get_class().get_name()), None)
views = []
if ocean:
    o, e = ocean.get_actor_bounds(False)
    z = ocean.get_actor_location().z
    views = [(unreal.Vector(o.x, o.y, z + 1500), unreal.Rotator(0, -8, 0)),
             (unreal.Vector(o.x - 5000, o.y, z + 600), unreal.Rotator(0, -4, 30))]
else:
    views = [(unreal.Vector(0, 0, 3000), unreal.Rotator(0, -15, 0))]

S = {"t": 0.0, "i": 0, "next": 30.0}


def tick(dt):
    S["t"] += dt
    if S["t"] < S["next"]:
        return
    if S["i"] < len(views):
        loc, rot = views[S["i"]]
        ues.set_level_viewport_camera_info(loc, rot)
        if S.get("moved") != S["i"]:
            S["moved"] = S["i"]
            S["next"] = S["t"] + 6  # let it settle after moving
            return
        unreal.SystemLibrary.execute_console_command(None, "HighResShot 1920x1080")
        LOG(f"shot {S['i']} at {loc}")
        S["i"] += 1
        S["next"] = S["t"] + 4
    else:
        unreal.unregister_slate_post_tick_callback(H)
        LOG("quitting")
        unreal.SystemLibrary.quit_editor()


H = unreal.register_slate_post_tick_callback(tick)
