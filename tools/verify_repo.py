#!/usr/bin/env python3
"""Repository integrity, link, asset and privacy gate for CI and local runs.

Usage: python3 tools/verify_repo.py
Exit 1 on the first failure.
"""
from __future__ import annotations

import json
import re
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"

# Built from fragments so this file does not match itself.
FORBIDDEN = re.compile(
    "|".join([
        "/Us" + "ers/",
        "/ho" + "me/[a-z]",
        "forsyth" + "c",
        "@ucr" + r"\.edu",
        "ucr" + "slurmcl",
        "ursa-" + "major",
        "bif" + "rost",
        "Omstead" + "Live",
        "192" + r"\.168\.",
        "100" + r"\.127\.",
        "storage" + r"\.googleapis\.com/ucr",
        "X-Goog-" + "Signature=[0-9a-f]{20}",
        "BEGIN [A-Z ]*PRIVATE" + " KEY",
        "gh" + "p_[A-Za-z0-9]{20,}",
        "AK" + "IA[0-9A-Z]{16}",
        "sk-" + "[A-Za-z0-9]{32,}",
    ]),
    re.IGNORECASE,
)
BINARY = {".png", ".jpg", ".jpeg", ".gif", ".mp3", ".mp4", ".glb", ".bin", ".uasset", ".umap"}


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def glb_json(path: Path) -> dict:
    data = path.read_bytes()
    if data[:4] != b"glTF":
        fail(f"bad GLB header: {path}")
    n = struct.unpack("<I", data[12:16])[0]
    return json.loads(data[20:20 + n])


def main() -> None:
    # 1. Web models: manifest entries exist, are valid glTF 2.0, under 4.5 MB.
    manifest = json.loads((DOCS / "models" / "manifest.json").read_text())
    models = manifest["models"]
    if len(models) != manifest["count"] or len(models) < 18:
        fail("models/manifest.json count mismatch")
    for m in models:
        p = DOCS / "models" / m["filename"]
        if not p.is_file():
            fail(f"missing model {p}")
        if p.stat().st_size > int(4.5 * 1024 * 1024):
            fail(f"model too large for the web: {p}")
        if glb_json(p).get("asset", {}).get("version") != "2.0":
            fail(f"not glTF 2.0: {p}")
        if not (DOCS / "assets" / "thumbs" / f"{m['id']}.jpg").is_file():
            fail(f"missing thumbnail for {m['id']}")

    # 2. Audio: every manifest clip has its MP3.
    audio = json.loads((DOCS / "audio" / "manifest.json").read_text())
    for c in audio["clips"]:
        if not (DOCS / c["file"]).is_file():
            fail(f"missing audio {c['file']}")

    # 3. Page: one h1, no inline style, unique ids, every local reference resolves.
    html = (DOCS / "index.html").read_text(encoding="utf-8")
    if "style=" in html:
        fail("docs/index.html has an inline style= attribute")
    if len(re.findall(r"<h1\b", html)) != 1:
        fail("docs/index.html must have exactly one <h1>")
    ids = re.findall(r'\bid="([^"]+)"', html)
    dupes = sorted({i for i in ids if ids.count(i) > 1})
    if dupes:
        fail(f"duplicate ids in docs/index.html: {dupes}")
    for ref in re.findall(r'\b(?:src|href|poster)="([^"]+)"', html):
        if ref.startswith(("http://", "https://", "#", "mailto:")):
            continue
        if not (DOCS / ref.split("?")[0]).exists():
            fail(f"broken reference in docs/index.html: {ref}")
    js = (DOCS / "site.js").read_text(encoding="utf-8")
    for ref in re.findall(r'"((?:assets|video|models|audio)/[^"+]+\.(?:jpg|png|mp4|json|glb))"', js):
        if not (DOCS / ref).exists():
            fail(f"broken reference in docs/site.js: {ref}")
    for jsid in re.findall(r'\$\("([^"]+)"\)', js):
        if jsid not in ids:
            fail(f"site.js looks up missing id #{jsid}")
    for base in re.findall(r'"assets/" \+ v\[1\] \+ ".jpg"', js):
        pass
    for name in re.findall(r'\["[^"]+", "([a-z0-9_]+)"\]', js):
        if not (DOCS / "assets" / f"{name}.jpg").exists():
            fail(f"site.js port view image missing: assets/{name}.jpg")
    for name in re.findall(r'\["([a-z0-9_]+)", "g-', js):
        if not (DOCS / "assets" / f"{name}.jpg").exists():
            fail(f"site.js gallery image missing: assets/{name}.jpg")

    # 4. Markdown relative links.
    for md in ["README.md", "SPEC.md", "CONTRIBUTING.md", "SECURITY.md", "CREDITS.md"]:
        text = (ROOT / md).read_text(encoding="utf-8")
        for ref in re.findall(r"\]\(([^)\s]+)\)", text):
            ref = ref.split("#")[0]
            if not ref or ref.startswith(("http://", "https://", "mailto:")):
                continue
            if not (ROOT / ref).exists():
                fail(f"broken link in {md}: {ref}")
        for ref in re.findall(r'<img[^>]+src="([^"]+)"', text):
            if not ref.startswith("http") and not (ROOT / ref).exists():
                fail(f"broken image in {md}: {ref}")

    # 5. Privacy scan of every tracked text file, plus image/GLB metadata strings.
    me = Path(__file__).resolve().relative_to(ROOT)
    tracked = subprocess.check_output(["git", "-C", str(ROOT), "ls-files"], text=True).splitlines()
    if not tracked:
        fail("no tracked files (run inside the git checkout)")
    for rel in tracked:
        fp = ROOT / rel
        if Path(rel) == me or not fp.is_file():
            continue
        if fp.suffix.lower() in BINARY:
            if fp.suffix.lower() == ".glb":
                hit = FORBIDDEN.search(json.dumps(glb_json(fp)))
            else:
                head = fp.read_bytes()[:65536].decode("latin-1")
                hit = FORBIDDEN.search(head)
        else:
            hit = FORBIDDEN.search(fp.read_text(encoding="utf-8", errors="ignore"))
        if hit:
            fail(f"private string '{hit.group(0)}' in {rel}")

    print(f"VERIFY OK: {len(models)} models, {len(audio['clips'])} sounds, page refs, links and privacy scan of {len(tracked)} files.")


if __name__ == "__main__":
    main()
