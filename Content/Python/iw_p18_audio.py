# P18: sound classes (Master root + 7 children), assign every cooked sound, import ui_click. Idempotent.
import unreal
BASE = "/Game/Ironwake/Audio"
CLS = "/Game/Ironwake/Audio/Classes"
eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

# ui_click import
if not eal.does_asset_exist(BASE + "/ui_click"):
    t = unreal.AssetImportTask()
    t.filename = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()) + "Audio/ui_click.wav"
    t.destination_path = BASE; t.destination_name = "ui_click"
    t.automated = True; t.save = True; t.replace_existing = True
    tools.import_asset_tasks([t])
    unreal.log("IWP18 imported ui_click")

def get_or_make(name):
    p = CLS + "/" + name
    if eal.does_asset_exist(p):
        return eal.load_asset(p)
    a = tools.create_asset(name, CLS, unreal.SoundClass, unreal.SoundClassFactory())
    unreal.log("IWP18 created " + p)
    return a

master = get_or_make("SC_Master")
kids = {k: get_or_make("SC_" + k) for k in ["Music", "Sea", "Wind", "Ship", "Guns", "Voices", "UI"]}
master.set_editor_property("child_classes", [kids[k] for k in ["Music", "Sea", "Wind", "Ship", "Guns", "Voices", "UI"]])
for k, c in list(kids.items()) + [("Master", master)]:
    props = c.get_editor_property("properties")
    props.set_editor_property("volume", 1.0)
    c.set_editor_property("properties", props)
    eal.save_loaded_asset(c)
eal.save_loaded_asset(master)

MAP = {
 "Music": ["battle_night_storm", "main_theme"],
 "Sea": ["bow_wash_loop", "hull_wake_loop", "hull_hiss_proc", "slow_lap_loop", "bow_rush_proc", "harbour_lap_proc", "surf_proc"],
 "Wind": ["wind_sea_proc", "gale_proc", "storm_howling_wind", "rain_proc", "thunder_near", "thunder_far"],
 "Ship": ["creak_storm_strain", "creak_wooden_hull"],
 "Guns": ["foghorn_sfx", "broadside_sfx"],
 "Voices": ["captain_contact", "captain_load", "officer_correct", "officer_damage", "officer_splash"],
 "UI": ["ui_click"],
}
seen = set()
for k, names in MAP.items():
    for n in names:
        s = eal.load_asset(BASE + "/" + n)
        if not s:
            unreal.log_error("IWP18 MISSING " + n); continue
        s.set_editor_property("sound_class_object", kids[k])
        eal.save_loaded_asset(s); seen.add(n)
        unreal.log("IWP18 classed %s -> %s" % (n, k))
for p in eal.list_assets(BASE, recursive=False):
    n = p.split("/")[-1].split(".")[0]
    if n not in seen:
        unreal.log_warning("IWP18 UNCLASSED asset " + n)
# verify parents
for k, c in kids.items():
    par = c.get_editor_property("parent_class")
    unreal.log("IWP18 parent %s -> %s" % (k, par.get_name() if par else None))
unreal.log("IWP18 audio done")
try:
    unreal.SystemLibrary.quit_editor()
except Exception:
    pass
