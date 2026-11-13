"""I1: foliage fixes: M_IW_Foliage clip 0.4 -> 0.22, leaf alpha textures mip gen Sharpen (keeps canopies at distance).
IWTEST_SCRIPT=iw_i1_folfix"""
import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
m = unreal.load_asset("/Game/Ironwake/Islands/M_IW_Foliage")
m.set_editor_property("opacity_mask_clip_value", 0.22)
MEL.recompile_material(m); EAL.save_asset("/Game/Ironwake/Islands/M_IW_Foliage")
n = 0
for d in ["IslandTree01", "IslandTree02", "IslandTree03", "FirTree01", "Shrub02", "Shrub03", "GrassMedium01"]:
    for ap in EAL.list_assets("/Game/Ironwake/World/%s" % d, recursive=False):
        a = unreal.load_asset(ap.split(".")[0])
        if isinstance(a, unreal.Texture2D) and "alpha" in a.get_name().lower():
            a.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SHARPEN5)
            EAL.save_asset(ap.split(".")[0]); n += 1
unreal.log("IWI1F DONE alpha textures=%d" % n)
unreal.SystemLibrary.quit_editor()
