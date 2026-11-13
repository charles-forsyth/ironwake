import unreal, os
proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
t = unreal.AssetImportTask()
t.filename = os.path.join(proj, "build/assets/airship/airship110.fbx")
t.destination_path = "/Game/Ironwake/Ships/Airship"
t.destination_name = "SM_Airship110"
t.automated = True; t.save = True; t.replace_existing = True
o = unreal.FbxImportUI()
o.import_mesh = True; o.import_materials = False; o.import_textures = False; o.import_as_skeletal = False
o.static_mesh_import_data.combine_meshes = True
o.static_mesh_import_data.remove_degenerates = False
o.static_mesh_import_data.generate_lightmap_u_vs = False
t.options = o
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
m = unreal.load_asset("/Game/Ironwake/Ships/Airship/SM_Airship110")
mat = unreal.load_asset("/Game/Ironwake/Ships/Airship/Material_0")
m.set_material(0, mat)
b = m.get_bounding_box()
unreal.log("IWIMPORT airship tris=%d bounds=%s %s" % (m.get_num_triangles(0), b.min, b.max))
unreal.EditorAssetLibrary.save_asset("/Game/Ironwake/Ships/Airship/SM_Airship110")
