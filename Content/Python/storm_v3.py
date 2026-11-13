"""Storm v3: start from Epic's working water sample (landscape + brush manager + ocean),
save it into our project, add storm sky/light/fog, push the waves up, screenshot, quit.
Logs wave-asset properties so they can be tuned in the next pass."""
import unreal

LOG = lambda m: unreal.log(f"IWTEST {m}")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
EAL = unreal.EditorAssetLibrary


def setp(obj, name, val):
    try:
        obj.set_editor_property(name, val)
        return True
    except Exception as e:  # noqa: BLE001
        LOG(f"WARN set {name}: {e}")
        return False


def spawn(cls, loc=(0, 0, 0), rot=(0, 0, 0), label=None):
    a = eas.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(*rot))
    if a and label:
        a.set_actor_label(label)
    return a


def dump(obj, tag, depth=0):
    """Log editor properties of a wave generator object (best effort)."""
    names = [n for n in dir(obj) if not n.startswith("_")]
    for n in names:
        try:
            v = obj.get_editor_property(n)
        except Exception:  # noqa: BLE001
            continue
        if callable(v):
            continue
        LOG(f"{tag}.{n} = {str(v)[:160]}")


if EAL.does_asset_exist("/Game/Maps/OceanStorm"):
    les.load_level("/Game/Maps/OceanStorm")
    LOG("loaded existing OceanStorm")
else:
    les.load_level("/WaterExtras/Maps/WaterTestMap")
    world = ues.get_editor_world()
    ok = unreal.EditorLoadingAndSavingUtils.save_map(world, "/Game/Maps/OceanStorm")
    LOG(f"save_map OceanStorm: {ok}")
    les.load_level("/Game/Maps/OceanStorm")

acts = eas.get_all_level_actors()
LOG(f"actors after load: {len(acts)}")
for a in acts:
    c = a.get_class().get_name()
    # Remove test props (player starts, custom water blobs) and any old sky we added
    if c in ("WaterBodyCustom",):
        eas.destroy_actor(a)
have = {a.get_class().get_name() for a in eas.get_all_level_actors()}
LOG(f"classes: {sorted(have)}")

if "DirectionalLight" not in have:
    sun = spawn(unreal.DirectionalLight, rot=(0, -10, 210), label="Sun")
else:
    sun = next(a for a in eas.get_all_level_actors() if a.get_class().get_name() == "DirectionalLight")
lc = sun.get_component_by_class(unreal.DirectionalLightComponent)
setp(lc, "intensity", 3.0)
setp(lc, "light_color", unreal.Color(200, 210, 225, 255))
setp(lc, "atmosphere_sun_light", True)
setp(lc, "cast_cloud_shadows", True)
setp(lc, "mobility", unreal.ComponentMobility.MOVABLE)
if "SkyAtmosphere" not in have:
    spawn(unreal.SkyAtmosphere, label="SkyAtmosphere")
if "SkyLight" not in have:
    sky = spawn(unreal.SkyLight, label="SkyLight")
    sc = sky.get_component_by_class(unreal.SkyLightComponent)
    setp(sc, "mobility", unreal.ComponentMobility.MOVABLE)
    setp(sc, "real_time_capture", True)
if "VolumetricCloud" not in have:
    cl = spawn(unreal.VolumetricCloud, label="Clouds")
    cc = cl.get_component_by_class(unreal.VolumetricCloudComponent)
    setp(cc, "layer_bottom_altitude", 0.5)
    setp(cc, "layer_height", 6.0)
    m = unreal.load_asset("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst")
    if m:
        setp(cc, "material", m)
if "ExponentialHeightFog" not in have:
    fog = spawn(unreal.ExponentialHeightFog, label="Fog")
    fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    setp(fc, "fog_density", 0.02)
    setp(fc, "fog_height_falloff", 0.2)
    setp(fc, "enable_volumetric_fog", True)
    setp(fc, "fog_inscattering_luminance", unreal.LinearColor(0.35, 0.38, 0.42, 1))

# Waves: inspect the ocean's wave asset and generator
ocean = next(a for a in eas.get_all_level_actors() if "WaterBodyOcean" in a.get_class().get_name())
oc = ocean.get_component_by_class(unreal.WaterBodyOceanComponent)
wa = None
for getter in ["get_water_waves", "get_editor_property"]:
    try:
        wa = oc.get_editor_property("water_waves") if getter == "get_editor_property" else getattr(ocean, getter)()
        if wa:
            break
    except Exception as e:  # noqa: BLE001
        LOG(f"WARN waves via {getter}: {e}")
LOG(f"wave obj: {wa} ({type(wa).__name__ if wa else None})")
if wa:
    dump(wa, "waves")
    for inner in ["water_waves", "gerstner_wave_generator", "wave_generator"]:
        try:
            iv = wa.get_editor_property(inner)
            LOG(f"inner {inner}: {iv}")
            if iv:
                dump(iv, f"waves.{inner}")
                for g in ["gerstner_wave_generator", "wave_generator"]:
                    try:
                        gv = iv.get_editor_property(g)
                        if gv:
                            LOG(f"generator {g}: {gv}")
                            dump(gv, f"gen")
                    except Exception:  # noqa: BLE001
                        pass
        except Exception:  # noqa: BLE001
            pass

cam = spawn(unreal.CineCameraActor, loc=(30000, 0, 700), rot=(0, -4, 180), label="ShotCam")
les.pilot_level_actor(cam)
les.save_current_level()
LOG("v3 saved")

YAWS = [180, 0]
S = {"t": 0.0, "i": 0, "next": 45.0, "done": False}


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
