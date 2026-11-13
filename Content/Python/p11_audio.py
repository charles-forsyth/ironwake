"""P11: re-import the regenerated broadband sailing loops (no tonal hum)."""
import unreal, os
L = lambda m: unreal.log("IWP11AUD " + str(m))
EAL = unreal.EditorAssetLibrary
src = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Audio")
names = ["bow_wash_loop", "hull_wake_loop", "hull_hiss_proc"]
tasks = []
for n in names:
    t = unreal.AssetImportTask(); t.filename = os.path.join(src, n + ".wav"); t.destination_path = "/Game/Ironwake/Audio"; t.destination_name = n
    t.automated = True; t.save = True; t.replace_existing = True; tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for n in names:
    s = unreal.load_asset("/Game/Ironwake/Audio/" + n)
    if s:
        s.set_editor_property("looping", True); EAL.save_asset("/Game/Ironwake/Audio/" + n)
    L(f"{n} {s} dur={s.get_editor_property('duration') if s else None} loop={s.get_editor_property('looping') if s else None}")
L("done")
unreal.SystemLibrary.quit_editor()
