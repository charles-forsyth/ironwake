"""Ironwake storm test v2: rebuild level cleanly with correctly sized ocean, then screenshot + quit.

Lessons from v1 (see log): default ocean_extents is 256 m and the WaterZone was scaled 400x
(142 km of water mesh at 1024 px) -> tiny smeared ocean patch. Here: zone scale 1 with
zone_extent 20 km, ocean_extents 20 km, camera at the ocean centre.
"""
import unreal

LOG = lambda m: unreal.log(f"IWTEST {m}")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
KM = 100000.0


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


def build():
    les.new_level("/Game/Maps/StormTest2")
    sun = spawn(unreal.DirectionalLight, rot=(0, -12, 200), label="Sun")
    lc = sun.get_component_by_class(unreal.DirectionalLightComponent)
    setp(lc, "intensity", 5.0)
    setp(lc, "atmosphere_sun_light", True)
    setp(lc, "cast_cloud_shadows", True)
    spawn(unreal.SkyAtmosphere, label="SkyAtmosphere")
    sky = spawn(unreal.SkyLight, label="SkyLight")
    sc = sky.get_component_by_class(unreal.SkyLightComponent)
    setp(sc, "mobility", unreal.ComponentMobility.MOVABLE)
    setp(sc, "real_time_capture", True)
    cl = spawn(unreal.VolumetricCloud, label="Clouds")
    cc = cl.get_component_by_class(unreal.VolumetricCloudComponent)
    setp(cc, "layer_bottom_altitude", 0.6)
    setp(cc, "layer_height", 5.0)
    m = unreal.load_asset("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst")
    if m:
        setp(cc, "material", m)
    fog = spawn(unreal.ExponentialHeightFog, label="Fog")
    fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    setp(fc, "fog_density", 0.006)
    setp(fc, "fog_height_falloff", 0.3)
    setp(fc, "enable_volumetric_fog", True)

    zone = spawn(unreal.WaterZone, label="WaterZone")
    setp(zone, "zone_extent", unreal.Vector2D(20 * KM, 20 * KM))
    ocean = spawn(unreal.WaterBodyOcean, label="Ocean")
    oc = ocean.get_component_by_class(unreal.WaterBodyOceanComponent)
    if oc:
        ok = setp(oc, "ocean_extents", unreal.Vector2D(20 * KM, 20 * KM))
        LOG(f"ocean_extents set: {ok}")
        for p in ["ocean_extents", "collision_extents", "height_offset"]:
            try:
                LOG(f"ocean {p} = {oc.get_editor_property(p)}")
            except Exception:  # noqa: BLE001
                pass
        for fn in ["update_all", "on_water_body_changed"]:
            if hasattr(oc, fn):
                try:
                    getattr(oc, fn)(True)
                except Exception as e:  # noqa: BLE001
                    LOG(f"WARN {fn}: {e}")
    try:
        zone.mark_for_rebuild(unreal.EWaterZoneRebuildFlags.ALL) if hasattr(unreal, "EWaterZoneRebuildFlags") else None
    except Exception as e:  # noqa: BLE001
        LOG(f"WARN rebuild {e}")

    pp = spawn(unreal.PostProcessVolume, label="PostProcess")
    setp(pp, "unbound", True)
    cam = spawn(unreal.CineCameraActor, loc=(0, 0, 800), rot=(0, -5, 20), label="ShotCam")
    les.pilot_level_actor(cam)
    les.save_current_level()
    LOG("v2 built + saved")
    return cam


CAM = build()
_STATE = {"t": 0.0, "stage": 0}


def _tick(dt):
    _STATE["t"] += dt
    t, s = _STATE["t"], _STATE["stage"]
    if s == 0 and t > 35:
        _STATE["stage"] = 1
        unreal.SystemLibrary.execute_console_command(None, "HighResShot 1920x1080")
        LOG("HighResShot issued")
    elif s == 1 and t > 50:
        _STATE["stage"] = 2
        LOG("quitting")
        unreal.unregister_slate_post_tick_callback(_HANDLE)
        unreal.SystemLibrary.quit_editor()


_HANDLE = unreal.register_slate_post_tick_callback(_tick)
