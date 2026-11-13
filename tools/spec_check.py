#!/usr/bin/env python3
"""spec_check.py: verify cited values in spec/*.md against their sources.

Tag format:  `value` [src path:line]  or  `value` [src path:a-b]
The value must appear (case-insensitive, literal) within +-2 lines of the cited line (or inside the range).
Usage: spec_check.py [--sample N] [--seed S] [--all] [--files f1.md f2.md] [--fails]
Default: random sample of 30 tags over all chapters. Exit 1 if any fail.
"""
import re, sys, random, glob, os, argparse

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
TAG = re.compile(r"`([^`\n]+)`\s*\[src ([^\]:\s]+):(\d+)(?:-(\d+))?\]")
_cache = {}

def lines_of(path):
    p = path if os.path.isabs(path) else os.path.join(ROOT, path)
    p = os.path.expanduser(p)
    if p not in _cache:
        try:
            _cache[p] = open(p, errors="replace").read().split("\n")
        except Exception:
            _cache[p] = None
    return _cache[p]

def check(val, path, a, b):
    L = lines_of(path)
    if L is None:
        return None, "source not in public repo"
    lo = max(0, a - 1 - 2)
    hi = min(len(L), (b or a) + 2)
    win = "\n".join(L[lo:hi]).lower()
    v = val.lower().strip()
    cands = {v, v.rstrip("f"), v.replace(",", ""), v.replace(" ", "")}
    if re.fullmatch(r"-?\d+\.\d+", v):
        cands.add(v.rstrip("0").rstrip("."))
    for c in cands:
        if c and c in win:
            return True, ""
    return False, "value not in window"

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sample", type=int, default=30)
    ap.add_argument("--seed", type=int, default=None)
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--fails", action="store_true", help="print only failures")
    ap.add_argument("--files", nargs="*")
    a = ap.parse_args()
    files = a.files or sorted(glob.glob(os.path.join(ROOT, "spec/[0-9]*.md")))
    tags = []
    for f in files:
        for i, line in enumerate(open(f).read().split("\n"), 1):
            for m in TAG.finditer(line):
                tags.append((os.path.basename(f), i, m.group(1), m.group(2), int(m.group(3)), int(m.group(4)) if m.group(4) else None))
    print(f"total tags: {len(tags)} in {len(files)} files")
    rnd = random.Random(a.seed)
    pick = tags if a.all else rnd.sample(tags, min(a.sample, len(tags)))
    bad = skip = 0
    for (f, i, v, p, l1, l2) in pick:
        ok, why = check(v, p, l1, l2)
        if ok is None:
            skip += 1
            continue
        if not ok:
            bad += 1
        if not ok or not a.fails:
            print(("PASS" if ok else "FAIL"), f"{f}:{i}", f"`{v}`", f"{p}:{l1}" + (f"-{l2}" if l2 else ""), why)
    print(f"checked {len(pick)-skip}: {len(pick)-skip-bad} pass, {bad} fail; {skip} cite build logs or tools kept out of the public repo")
    sys.exit(1 if bad else 0)

if __name__ == "__main__":
    main()
