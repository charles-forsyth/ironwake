"""Storm v4: on /Game/Maps/OceanStorm (copy of Epic's water sample), replace the ocean waves with a
project-owned Gerstner storm generator, darken sky/light, thicken fog, hide the test island props,
then shoot from low over open water. Logs before/after wave settings."""
import unreal

LOG = lambda m: unreal.log(f"IWTEST {m}")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
EAL = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

les.load_level("/Game/Maps/OceanStorm")
acts = eas.get_all_level_actors()
ocean = next(a for a in acts if a.get_class().get_name() == "WaterBodyOcean")


def setp(obj, name, val):
    try:
        obj.set_editor_property(name, val)
        return True
    except Exception as e:  # noqa: BLE001
        LOG(f"WARN set {name}: {e}")
        return False


# ---- Waves ---------------------------------------------------------------
old = ocean.get_editor_property("water_waves")
LOG(f"old waves: {old} class={old.get_class().get_name() if old else None}")
try:
    wrapped = old.get_editor_property("water_waves") if old else None
    LOG(f"old wrapped: {wrapped}")
except Exception as e:  # noqa: BLE001
    LOG(f"old wrapped err {e}")

gen = unreal.new_object(unreal.GerstnerWaterWaveGeneratorSimple, ocean)
for k, v in {
    "num_waves": 24, "seed": 1890, "randomness": 0.4,
    "min_wavelength": 800.0, "max_wavelength": 18000.0, "wavelength_falloff": 2.0,
    "min_amplitude": 8.0, "max_amplitude": 420.0, "amplitude_falloff": 2.0,
    "wind_angle_deg": -30.0, "direction_angular_spread_deg": 75.0,
    "small_wave_steepness": 0.45, "large_wave_steepness": 0.25, "steepness_falloff": 1.0,
}.items():
    setp(gen, k, v)
gw = unreal.new_object(unreal.GerstnerWaterWaves, ocean)
ok_gen = setp(gw, "gerstner_wave_generator", gen)
LOG(f"generator set on GerstnerWaterWaves: {ok_gen}")
try:
    gw.call_method("RecomputeWaves") if hasattr(gw, "call_method") else None
except Exception as e:  # noqa: BLE001
    LOG(f"recompute (optional) {e}")
try:
    ocean.set_water_waves(gw)
    LOG("ocean.set_water_waves(GerstnerWaterWaves) ok")
except Exception as e:  # noqa: BLE001
    LOG(f"WARN set_water_waves direct: {e}")
new = ocean.get_editor_property("water_waves")
LOG(f"new waves: {new} class={new.get_class().get_name() if new else None}")
for obj, tag in [(new, "new")]:
    try:
        inner = obj.get_editor_property("water_waves") if obj and obj.get_class().get_name() != "GerstnerWaterWaves" else obj
        LOG(f"{tag} inner={inner}")
        LOG(f"max_wave_height now: {inner.get_editor_property('max_wave_height')}")
        LOG(f"num gerstner waves: {len(inner.get_editor_property('gerstner_waves'))}")
    except Exception as e:  # noqa: BLE001
        LOG(f"wave readback err {e}")

# ---- Storm look ----------------------------------------------------------
for a in eas.get_all_level_actors():
    c = a.get_class().get_name()
    if c == "DirectionalLight":
        lc = a.get_component_by_class(unreal.DirectionalLightComponent)
        setp(lc, "intensity", 2.2)
        setp(lc, "light_color", unreal.Color(190, 200, 215, 255))
        a.set_actor_rotation(unreal.Rotator(0, -18, 200), False)
    elif c == "ExponentialHeightFog":
        fc = a.get_component_by_class(unreal.ExponentialHeightFogComponent)
        setp(fc, "fog_density", 0.03)
        setp(fc, "fog_height_falloff", 0.12)
        setp(fc, "fog_inscattering_luminance", unreal.LinearColor(0.20, 0.23, 0.26, 1))
        setp(fc, "enable_volumetric_fog", True)
    elif c == "VolumetricCloud":
        cc = a.get_component_by_class(unreal.VolumetricCloudComponent)
        setp(cc, "layer_bottom_altitude", 0.4)
        setp(cc, "layer_height", 8.0)
        setp(cc, "ground_albedo", unreal.Color(60, 60, 60, 255))
    elif c in ("StaticMeshActor", "PlayerStart", "WaterBodyLake", "WaterBodyRiver", "InstancedFoliageActor"):
        a.set_is_temporarily_hidden_in_editor(True)
        a.set_actor_hidden_in_game(True)

cam = next((a for a in eas.get_all_level_actors() if a.get_actor_label() == "ShotCam"), None)
if not cam:
    cam = eas.spawn_actor_from_class(unreal.CineCameraActor, unreal.Vector(35000, 0, 900), unreal.Rotator(0, -3, 0))
    cam.set_actor_label("ShotCam")
cam.set_actor_location(unreal.Vector(35000, 0, 700), False, False)
les.pilot_level_actor(cam)
les.save_current_level()
LOG("v4 saved")

SHOTS = [(0, -3), (150, -4), (270, -6)]
S = {"t": 0.0, "i": 0, "next": 45.0, "done": False}


def tick(dt):
    if S["done"]:
        return
    S["t"] += dt
    if S["t"] < S["next"]:
        return
    i = S["i"]
    if i < len(SHOTS):
        if S.get("aimed") != i:
            yaw, pitch = SHOTS[i]
            cam.set_actor_rotation(unreal.Rotator(0, pitch, yaw), False)
            S["aimed"] = i
            S["next"] = S["t"] + 5
            return
        unreal.AutomationLibrary.take_high_res_screenshot(1600, 900, f"STORM4_{i}.png", cam)
        LOG(f"shot {SHOTS[i]}")
        S["i"] = i + 1
        S["next"] = S["t"] + 8
    else:
        S["done"] = True
        unreal.unregister_slate_post_tick_callback(H)
        LOG("quitting")
        unreal.SystemLibrary.quit_editor()


H = unreal.register_slate_post_tick_callback(tick)
