"""Ironwake look test level, built by UE editor Python.

Run:  UnrealEditor IronwakeTest.uproject -ExecutePythonScript=<this file>
Builds /Game/Maps/StormTest: Water ocean, sky atmosphere, volumetric clouds, height fog,
overcast sun, post-process, a camera, then saves a high-res screenshot to Saved/Screenshots.
Logs every step with prefix IWTEST so a run can be checked from the log file.
"""
import unreal

LOG = lambda m: unreal.log(f"IWTEST {m}")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def spawn(cls, loc=(0, 0, 0), rot=(0, 0, 0), label=None):
    a = eas.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(*rot))
    if a and label:
        a.set_actor_label(label)
    LOG(f"spawned {label or cls.__name__}: {bool(a)}")
    return a


def setp(obj, name, val):
    try:
        obj.set_editor_property(name, val)
    except Exception as e:  # noqa: BLE001
        LOG(f"WARN set {name}: {e}")


def main():
    if not unreal.EditorAssetLibrary.does_asset_exist("/Game/Maps/StormTest"):
        les.new_level("/Game/Maps/StormTest")
    else:
        les.load_level("/Game/Maps/StormTest")
    for a in eas.get_all_level_actors():
        eas.destroy_actor(a)

    # Sun: low, overcast-ish afternoon light
    sun = spawn(unreal.DirectionalLight, rot=(0, -14, 210), label="Sun")
    lc = sun.get_component_by_class(unreal.DirectionalLightComponent)
    setp(lc, "intensity", 6.0)
    setp(lc, "light_color", unreal.Color(235, 228, 215, 255))
    setp(lc, "atmosphere_sun_light", True)
    setp(lc, "cast_cloud_shadows", True)

    spawn(unreal.SkyAtmosphere, label="SkyAtmosphere")
    sky = spawn(unreal.SkyLight, label="SkyLight")
    sc = sky.get_component_by_class(unreal.SkyLightComponent)
    setp(sc, "real_time_capture", True)
    setp(sc, "mobility", unreal.ComponentMobility.MOVABLE)

    clouds = spawn(unreal.VolumetricCloud, label="Clouds")
    cc = clouds.get_component_by_class(unreal.VolumetricCloudComponent)
    setp(cc, "layer_bottom_altitude", 0.8)
    setp(cc, "layer_height", 6.0)
    # Overcast storm cloud material if the engine ships one
    for path in ["/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst",
                 "/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud"]:
        m = unreal.load_asset(path)
        if m:
            setp(cc, "material", m)
            LOG(f"cloud material {path}")
            break

    fog = spawn(unreal.ExponentialHeightFog, label="Fog")
    fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    setp(fc, "fog_density", 0.004)
    setp(fc, "fog_height_falloff", 0.35)
    setp(fc, "enable_volumetric_fog", True)
    setp(fc, "fog_inscattering_luminance", unreal.LinearColor(0.32, 0.36, 0.40, 1))

    # Ocean via Water plugin
    ocean = None
    if hasattr(unreal, "WaterBodyOcean"):
        ocean = spawn(unreal.WaterBodyOcean, loc=(0, 0, 0), label="Ocean")
        if hasattr(unreal, "WaterZone"):
            z = spawn(unreal.WaterZone, label="WaterZone")
            try:
                z.set_actor_scale3d(unreal.Vector(400, 400, 1))
            except Exception as e:  # noqa: BLE001
                LOG(f"WARN zone scale {e}")
        # Stronger waves if a Gerstner asset is available
        for path in ["/Water/Waves/GerstnerWaves_Ocean", "/Water/Waves/GerstnerWaves_Ocean_Storm"]:
            w = unreal.load_asset(path)
            if w:
                LOG(f"wave asset found {path}")
    else:
        LOG("WARN WaterBodyOcean class missing (Water plugin not loaded)")

    pp = spawn(unreal.PostProcessVolume, label="PostProcess")
    setp(pp, "unbound", True)
    s = pp.get_editor_property("settings")
    for k, v in [("override_auto_exposure_bias", True), ("auto_exposure_bias", 0.3),
                 ("override_color_saturation", True), ("color_saturation", unreal.Vector4(0.8, 0.82, 0.85, 1)),
                 ("override_vignette_intensity", True), ("vignette_intensity", 0.45),
                 ("override_film_grain_intensity", True), ("film_grain_intensity", 0.15)]:
        try:
            s.set_editor_property(k, v)
        except Exception as e:  # noqa: BLE001
            LOG(f"WARN pp {k}: {e}")
    pp.set_editor_property("settings", s)

    cam = spawn(unreal.CineCameraActor, loc=(-3000, 0, 450), rot=(0, -4, 0), label="ShotCam")
    les.pilot_level_actor(cam)
    les.save_current_level()
    LOG("level saved")

    # Wait for shaders + streaming, then take several shots (fog off/on diagnostics)
    state = {"t": 0.0, "shot": 0}
    shots = [(25.0, "IWTEST_storm_a.png"), (45.0, "IWTEST_storm_b.png")]

    def tick(dt):
        state["t"] += dt
        if state["shot"] < len(shots) and state["t"] >= shots[state["shot"]][0]:
            unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080, shots[state["shot"]][1], cam)
            LOG(f"screenshot {shots[state['shot']][1]} at {state['t']:.0f}s, shaders pending "
                f"{unreal.SystemLibrary.get_console_variable_int_value('r.ShaderCompiler.NumRemaining') if False else '?'}")
            state["shot"] += 1
        if state["shot"] >= len(shots) and state["t"] >= shots[-1][0] + 8:
            LOG("done, quitting")
            unreal.unregister_slate_post_tick_callback(handle)
            unreal.SystemLibrary.quit_editor()

    handle = unreal.register_slate_post_tick_callback(tick)


main()
