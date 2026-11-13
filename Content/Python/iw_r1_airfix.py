"""R1 airship fix: reimport BrassAirship meshes with recomputed normals, flip the normal map's green (glTF -> UE),
then report. Run: IWTEST_SCRIPT=iw_r1_airfix UnrealEditor IronwakeTest.uproject -log"""
import unreal, os
proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
d = os.path.join(proj, "build/assets/r1/BrassAirship")
dest = "/Game/Ironwake/Ships/BrassAirship"
EAL = unreal.EditorAssetLibrary
for part in ["Hull", "PropStern"]:
    name = "SM_BrassAirship_%s" % part
    mi = unreal.load_asset("%s/MI_BrassAirship_tripo_mat_5ec429a9" % dest)
    t = unreal.AssetImportTask()
    t.filename = os.path.join(d, "BrassAirship_%s.fbx" % part)
    t.destination_path = dest; t.destination_name = name
    t.automated = True; t.save = True; t.replace_existing = True
    o = unreal.FbxImportUI()
    o.import_mesh = True; o.import_as_skeletal = False; o.import_materials = False; o.import_textures = False
    sd = o.static_mesh_import_data
    sd.combine_meshes = True; sd.auto_generate_collision = False; sd.generate_lightmap_u_vs = False
    sd.build_nanite = False   # Nanite clusters shattered this thin AI mesh in game (holes); classic LOD0 190k is fine for 1 instance
    sd.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_COMPUTE_NORMALS
    sd.normal_generation_method = unreal.FBXNormalGenerationMethod.MIKK_T_SPACE
    t.options = o
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    m = unreal.load_asset("%s/%s" % (dest, name))
    if m and mi:
        m.set_material(0, mi)
    if m:
        sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
        ns = sub.get_nanite_settings(m); ns.enabled = False; sub.set_nanite_settings(m, ns, True)
    EAL.save_asset("%s/%s" % (dest, name))
    unreal.log("IWAIRFIX %s tris=%d" % (name, m.get_num_triangles(0) if m else -1))
n = unreal.load_asset("%s/T_BrassAirship_Image_2" % dest)
if n:
    n.set_editor_property("flip_green_channel", True)
    EAL.save_asset("%s/T_BrassAirship_Image_2" % dest)
    unreal.log("IWAIRFIX normal map green flipped")
unreal.log("IWAIRFIX DONE")
unreal.SystemLibrary.quit_editor()
