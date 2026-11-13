"""Audio fix (Chuck 09:55): import harbour_lap_proc / bow_rush_proc / hull_hiss_proc (looping), delete tonal harbour_ambience + sea_wind."""
import unreal, os
L = lambda m: unreal.log("IWAUD " + str(m))
EAL = unreal.EditorAssetLibrary
src = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Audio")
tasks = []
for n in ["harbour_lap_proc", "bow_rush_proc", "hull_hiss_proc"]:
    t = unreal.AssetImportTask(); t.filename = os.path.join(src, n + ".wav"); t.destination_path = "/Game/Ironwake/Audio"; t.destination_name = n
    t.automated = True; t.save = True; t.replace_existing = True; tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for n in ["harbour_lap_proc", "bow_rush_proc", "hull_hiss_proc"]:
    s = unreal.load_asset("/Game/Ironwake/Audio/" + n)
    if s: s.set_editor_property("looping", True); EAL.save_asset("/Game/Ironwake/Audio/" + n)
    L(f"{n} {s} dur={s.get_editor_property('duration') if s else None}")
for n in ["harbour_ambience", "sea_wind"]:
    p = "/Game/Ironwake/Audio/" + n
    if EAL.does_asset_exist(p):
        refs = EAL.find_package_referencers_for_asset(p, False)
        L(f"{n} referencers={list(refs)}")
        L(f"delete {n}: {EAL.delete_asset(p)}")
L("done")
unreal.SystemLibrary.quit_editor()
