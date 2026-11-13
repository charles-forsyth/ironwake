import unreal
eal = unreal.EditorAssetLibrary
C = "/Game/Ironwake/Audio/Classes/"
m = eal.load_asset(C + "SC_Master")
for k in ["Music", "Sea", "Wind", "Ship", "Guns", "Voices", "UI"]:
    c = eal.load_asset(C + "SC_" + k)
    try:
        c.set_editor_property("parent_class", m)
    except Exception as e:
        unreal.log_warning("IWP18 parent set fail %s: %s" % (k, e))
    eal.save_loaded_asset(c)
eal.save_loaded_asset(m)
for k in ["Music", "Sea", "Wind", "Ship", "Guns", "Voices", "UI"]:
    c = eal.load_asset(C + "SC_" + k)
    p = c.get_editor_property("parent_class")
    unreal.log("IWP18 parent %s -> %s" % (k, p.get_name() if p else None))
unreal.log("IWP18 children of master: " + ",".join(x.get_name() for x in m.get_editor_property("child_classes")))
unreal.SystemLibrary.quit_editor()
