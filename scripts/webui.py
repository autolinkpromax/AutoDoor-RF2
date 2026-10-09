#!/usr/bin/env python3
"""Web UI dev workflow — bundle, pack, or flash firmware.

Examples:
  python scripts/webui.py bundle     # JS only (~instant)
  python scripts/webui.py pack       # bundle + webui_assets.h + manifest
  python scripts/webui.py build      # pack + compile (no upload)
  python scripts/webui.py flash      # pack + compile + upload
"""
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PACK = ROOT / "scripts" / "pack_webui.py"


def run(cmd: list[str], *, cwd: Path = ROOT) -> int:
    print("+", " ".join(cmd))
    return subprocess.call(cmd, cwd=cwd)


def main() -> int:
    parser = argparse.ArgumentParser(description="AutoDoor Web UI workflow")
    parser.add_argument(
        "step",
        choices=("bundle", "pack", "build", "flash"),
        help="bundle=JS only | pack=PROGMEM header | build=pio run | flash=upload",
    )
    parser.add_argument("-e", "--env", default="esp32-c3-devkitm-1", help="PlatformIO env name")
    args = parser.parse_args()
    py = sys.executable

    if args.step == "bundle":
        return run([py, str(PACK), "--bundle-only"])

    if args.step == "pack":
        return run([py, str(PACK)])

    if args.step == "build":
        return run(["pio", "run", "-e", args.env])

    # flash
    return run(["pio", "run", "-e", args.env, "-t", "upload"])


if __name__ == "__main__":
    raise SystemExit(main())
