import unreal
for n in ["TownBandstand","TownChapel","TownCrateStack","TownCustomsHouse","TownHandcart","TownMarketStall","TownRowCottages","TownStoneTownhouse","TownStoneWall","TownTimberHouse","WharfBuilding","VictorianFactory","DockWarehouse","CoalingStation","SteamCrane","GasLamp","WallLamp","HangingLamp","Barrel03","WoodenBarrels","MilitaryCrate","BollardCapstan","Rowboat","SteamBuoy","WoodenPier"]:
    m = unreal.load_asset("/Game/Ironwake/World/%s/SM_%s" % (n, n))
    if not m: unreal.log("IWB %s MISSING" % n); continue
    b = m.get_bounds(); o = b.origin; e = b.box_extent
    unreal.log("IWB %s origin=(%.0f,%.0f,%.0f) ext=(%.0f,%.0f,%.0f) cm" % (n, o.x,o.y,o.z, e.x,e.y,e.z))
unreal.SystemLibrary.quit_editor()
