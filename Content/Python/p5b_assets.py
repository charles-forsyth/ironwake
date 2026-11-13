import unreal
EAL = unreal.EditorAssetLibrary
p = "/Game/Ironwake/FX/NS_IW_FunnelSmoke"
if not EAL.does_asset_exist(p):
    EAL.duplicate_asset("/Game/Ironwake/FX/NS_IW_Smoke", p)
    EAL.save_asset(p)
unreal.log("IWP5B funnel exists=%s" % EAL.does_asset_exist(p))
unreal.SystemLibrary.quit_editor()
