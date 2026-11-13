"""W4: run the foam node + spray material builders and leave the editor up (MCP) for the Niagara edits (build/tools/w4fx.py)."""
import importlib, unreal
for mod in ("w4_foammat", "w4_spray"):
    try:
        importlib.import_module(mod)
    except Exception as e:
        unreal.log_error(f"IWW4 {mod} failed: {e}")
unreal.log("IWW4 BUILD DONE")
