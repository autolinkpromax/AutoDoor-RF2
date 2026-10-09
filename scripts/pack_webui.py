#!/usr/bin/env python3
"""Pack webui/ sources into src/webui_assets.h (PROGMEM) + manifest.json."""
from __future__ import annotations

import hashlib
import json
import re
from datetime import datetime, timezone
from pathlib import Path

try:
    Import("env")
    ROOT = Path(env["PROJECT_DIR"])
except Exception:
    ROOT = Path(__file__).resolve().parent.parent
WEBUI = ROOT / "webui"
OUT_H = ROOT / "src" / "webui_assets.h"
MANIFEST_OUT = WEBUI / "manifest.json"
INDEX_SRC = WEBUI / "index.html"

SKIP = {
    "manifest.json",
    "README.md",
    # loader.js must be packed — bootstrap imports it as ES module
}

APP_SRC_DIR = WEBUI / "js" / "src"
APP_BUNDLE_OUT = WEBUI / "js" / "app-bundle.js"


def bundle_app_js() -> None:
    """Concat webui/js/src/*.js -> webui/js/app-bundle.js (load order = filename)."""
    if not APP_SRC_DIR.is_dir():
        return
    parts = sorted(APP_SRC_DIR.glob("*.js"))
    if not parts:
        return
    chunks = ["/* generated — edit webui/js/src/ */"]
    for path in parts:
        chunks.append(path.read_text(encoding="utf-8").rstrip())
    APP_BUNDLE_OUT.write_text("\n".join(chunks) + "\n", encoding="utf-8")


def minify_text(text: str, ext: str) -> str:
    if ext in (".css", ".js"):
        lines = [ln.strip() for ln in text.splitlines() if ln.strip()]
        return "\n".join(lines)
    if ext == ".html":
        return re.sub(r">\s+<", "><", text.strip())
    return text

MIME = {
    ".html": "text/html; charset=utf-8",
    ".css": "text/css; charset=utf-8",
    ".js": "application/javascript; charset=utf-8",
    ".json": "application/json; charset=utf-8",
}


def file_hash(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()[:6]


def safe_c_name(path: str) -> str:
    s = re.sub(r"[^a-zA-Z0-9]", "_", path)
    if s and s[0].isdigit():
        s = "f_" + s
    return s


def escape_c(s: str) -> str:
    return (
        s.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("\n", "\\n")
        .replace("\r", "")
        .replace("\t", "\\t")
    )


def chunk_string(s: str, width: int = 120) -> list[str]:
    lines = []
    i = 0
    while i < len(s):
        lines.append('"' + escape_c(s[i : i + width]) + '"')
        i += width
    return lines or ['""']


def collect_assets() -> list[tuple[str, bytes]]:
    items: list[tuple[str, bytes]] = []
    for path in sorted(WEBUI.rglob("*")):
        if not path.is_file():
            continue
        rel = path.relative_to(WEBUI).as_posix()
        if rel in SKIP or path.name.startswith("."):
            continue
        if path.suffix == ".template.json":
            continue
        if rel == "index.html":
            continue
        if rel.startswith("js/src/"):
            continue
        raw = path.read_bytes()
        text = minify_text(raw.decode("utf-8"), path.suffix)
        items.append((rel, text.encode("utf-8")))
    return items


def build_manifest(versioned: dict[str, str]) -> dict:
    def v(rel: str) -> str:
        return versioned[rel]

    return {
        "build": datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S"),
        "core": {
            "css": [
                v("css/base.css"),
                v("css/components.css"),
                v("css/loader.css"),
            ],
            "js": [v("js/app-bundle.js")],
        },
        "views": {
            "dash": {
                "html": v("html/partials/dash.html"),
                "css": [v("css/gate.css")],
                "js": [],
            },
            "devices": {
                "html": v("html/partials/devices.html"),
                "css": [v("css/devices.css")],
                "js": [],
            },
            "settings": {
                "html": v("html/partials/settings.html"),
                "css": [v("css/settings.css")],
                "js": [],
            },
            "editor": {
                "html": v("html/partials/editor.html"),
                "css": [v("css/editor.css")],
                "js": [],
            },
        },
        "partials": {
            "login": v("html/partials/login.html"),
            "safemode": v("html/partials/safemode.html"),
            "nav": v("html/partials/nav.html"),
        },
        "bootstrap": v("js/bootstrap.js"),
    }


def main():
    if not WEBUI.is_dir():
        raise SystemExit(f"Missing {WEBUI}")

    bundle_app_js()
    raw_assets = collect_assets()
    payloads: dict[str, bytes] = dict(raw_assets)

    # ES module import must use packed flat URL (not /assets/core/loader.js).
    loader_vname = None
    if "js/core/loader.js" in payloads:
        loader_vname = f"loader.{file_hash(payloads['js/core/loader.js'])}.js"
    if "js/bootstrap.js" in payloads and loader_vname:
        text = payloads["js/bootstrap.js"].decode("utf-8")
        text = text.replace(
            "from './core/loader.js'",
            f"from '/assets/{loader_vname}'",
        )
        payloads["js/bootstrap.js"] = text.encode("utf-8")

    versioned: dict[str, str] = {}
    for rel, data in payloads.items():
        p = Path(rel)
        versioned[rel] = f"{p.stem}.{file_hash(data)}{p.suffix}"

    manifest = build_manifest(versioned)
    manifest_bytes = json.dumps(manifest, ensure_ascii=False, separators=(",", ":")).encode("utf-8")

    index_tpl = INDEX_SRC.read_text(encoding="utf-8")
    index_html = index_tpl.replace("__BOOTSTRAP_JS__", manifest["bootstrap"])
    index_bytes = index_html.encode("utf-8")

    blobs: list[tuple[str, str, str, bytes, bool]] = []

    def add(path: str, mime: str, data: bytes, immutable: bool = True):
        etag = file_hash(data)
        blobs.append((path, mime, etag, data, immutable))

    for rel, data in payloads.items():
        add(versioned[rel], MIME.get(Path(rel).suffix, "application/octet-stream"), data, True)

    add("manifest.json", MIME[".json"], manifest_bytes, False)
    add("index.html", MIME[".html"], index_bytes, False)

    MANIFEST_OUT.write_text(manifest_bytes.decode("utf-8"), encoding="utf-8")

    lines = [
        "#pragma once",
        "#include <pgmspace.h>",
        "#include <stddef.h>",
        "",
        "struct WebUiAsset {",
        "  const char* path;",
        "  const char* mime;",
        "  const char* etag;",
        "  const char* data;",
        "  size_t len;",
        "  bool immutable;",
        "};",
        "",
    ]

    for i, (path, mime, etag, data, immutable) in enumerate(blobs):
        cname = safe_c_name(f"a{i}_{path}")
        text = data.decode("utf-8")
        lines.append(f"static const char {cname}[] PROGMEM =")
        for part in chunk_string(text):
            lines.append("  " + part)
        lines.append(";")
        lines.append(f"static const size_t {cname}_len = {len(data)};")
        lines.append("")

    lines.append("static const WebUiAsset kWebUiAssets[] = {")
    for i, (path, mime, etag, data, immutable) in enumerate(blobs):
        cname = safe_c_name(f"a{i}_{path}")
        imm = "true" if immutable else "false"
        lines.append(
            f'  {{ "{path}", "{mime}", "\\"{etag}\\"", {cname}, {cname}_len, {imm} }},'
        )
    lines.append("};")
    lines.append(f"static const size_t kWebUiAssetCount = {len(blobs)};")
    lines.append("")

    OUT_H.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"[pack_webui] {len(blobs)} assets -> {OUT_H.relative_to(ROOT)}")


if __name__ == "__main__":
    import sys

    if "--bundle-only" in sys.argv:
        if not WEBUI.is_dir():
            raise SystemExit(f"Missing {WEBUI}")
        bundle_app_js()
        print(f"[pack_webui] bundled {len(list(APP_SRC_DIR.glob('*.js')))} modules -> {APP_BUNDLE_OUT.relative_to(ROOT)}")
    else:
        main()
