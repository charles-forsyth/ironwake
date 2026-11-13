import unreal, os
src = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Audio")
tasks = []
for f in sorted(os.listdir(src)):
    if not f.endswith(".wav"):
        continue
    name = f[:-4]
    if unreal.EditorAssetLibrary.does_asset_exist("/Game/Ironwake/Audio/" + name):
        continue
    t = unreal.AssetImportTask()
    t.filename = os.path.join(src, f)
    t.destination_path = "/Game/Ironwake/Audio"
    t.destination_name = name
    t.automated = True
    t.save = True
    t.replace_existing = True
    tasks.append(t)
if tasks:
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for t in tasks:
    unreal.log("IWIMPORT " + t.filename + " -> " + ", ".join(t.imported_object_paths))
unreal.EditorAssetLibrary.save_directory("/Game/Ironwake/Audio")
unreal.log("IWIMPORT done")
