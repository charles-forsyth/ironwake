"""W2 assets (headless): storm audio layers + thunder, NS_IW_Spume / NS_IW_Rain / NS_IW_BowSheet (dups to retune over MCP)."""
import unreal, os
L = lambda m: unreal.log("IWW2 " + str(m))
src = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Audio")
EAL = unreal.EditorAssetLibrary
names = ["wind_sea_proc", "gale_proc", "storm_howling_wind", "rain_proc", "creak_storm_strain", "creak_wooden_hull", "thunder_near", "thunder_far"]
tasks = []
for n in names:
    t = unreal.AssetImportTask(); t.filename = os.path.join(src, n + ".wav"); t.destination_path = "/Game/Ironwake/Audio"; t.destination_name = n
    t.automated = True; t.save = True; t.replace_existing = True; tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for n in names:
    s = unreal.load_asset("/Game/Ironwake/Audio/" + n)
    if s:
        s.set_editor_property("looping", not n.startswith("thunder") and n != "creak_wooden_hull")
        EAL.save_asset("/Game/Ironwake/Audio/" + n)
    L(f"sound {n} {s} dur={s.get_editor_property('duration') if s else None}")
for a, b in [("NS_IW_BowSpray", "NS_IW_Spume"), ("NS_IW_Smoke", "NS_IW_Rain"), ("NS_IW_BowSpray", "NS_IW_BowSheet")]:
    p = "/Game/Ironwake/FX/" + b
    if not EAL.does_asset_exist(p):
        EAL.duplicate_asset("/Game/Ironwake/FX/" + a, p)
        EAL.save_asset(p)
    L(f"{b} exists={EAL.does_asset_exist(p)}")
L("assets done")
unreal.SystemLibrary.quit_editor()
