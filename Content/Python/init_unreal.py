import os, importlib
_s = os.environ.get("IWTEST_SCRIPT")
if _s:
    importlib.import_module(_s)
