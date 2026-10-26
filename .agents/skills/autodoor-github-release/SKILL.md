---
name: autodoor-github-release
description: >-
  Release and upload firmware .bin & version.json to GitHub repository (origin/main) for online Web UI OTA updates. Use when user asks to upload/release firmware to GitHub, อัพเดตลง github, release firmware, bump version, publish online OTA, or enable other devices to update firmware over GitHub.
---

# AutoDoor-RF2 GitHub Firmware Release Skill

Automate building, bundling WebUI, updating version metadata, and releasing firmware binaries to the GitHub repository (`autolinkpromax/AutoDoor-RF2` on branch `main`). This enables remote AutoDoor devices to check for updates and flash wirelessly via the Web UI ("🔍 ตรวจสอบเวอร์ชันใหม่" ➔ "🚀 อัปเดตทันที (ออนไลน์)").

## Quick CLI Commands (Run in Workspace Root)

### 1. Auto-Bump Patch Version & Build & Release (Recommended)
Automatically bumps patch version (e.g. `1.0.2` ➔ `1.0.3`), compiles firmware, and pushes to GitHub:
```bash
python scripts/release_github.py --bump --build -m "V1.0.3: รายละเอียดการอัปเดต..."
```

### 2. Release Current Compiled Binary Directly (Fast Release)
Releases the currently compiled binary and existing metadata without rebuilding:
```bash
python scripts/release_github.py
```

### 3. Release Specific Version with Custom Changelog
```bash
python scripts/release_github.py --version 1.0.3 --build -m "V1.0.3: แก้ไขบั๊กและเพิ่มฟีเจอร์ใหม่"
```

### 4. Test Release (Dry-Run / No Push)
Verify trees, blobs, and commit creation without pushing to GitHub:
```bash
python scripts/release_github.py --dry-run
```

---

## Command Options Reference

| Option | Flag | Description |
| :--- | :---: | :--- |
| `--bump` | `-u` | Automatically increment patch version (`1.0.2` ➔ `1.0.3`) and sync `config.h` |
| `--version <VER>` | `-v` | Specify exact release version (e.g. `1.0.3` or `1.1.0`) |
| `--changelog <MSG>` | `-m` | Set changelog text to display in the Web UI update prompt |
| `--build` | `-b` | Pack WebUI assets and recompile release firmware via PlatformIO first |
| `--dry-run` | | Simulate tree & commit creation without pushing to remote GitHub |

---

## How It Works Under the Hood
1. **Sync Version**: Synchronizes `kFirmwareVersion` in `src/app/config.h` and `firmware/version.json`.
2. **Build Release**: (If `--build` is used) Packs WebUI (`pack_webui.py`) and compiles `esp32-c3-release`.
3. **Plumbing Git Commits**: Uses `git hash-object` and `git mktree` with clean LF line-endings to create an orphan commit on top of `origin/main` without affecting the local working tree or dirtying Git status.
4. **Instant Push**: Pushes to `origin/main` via Windows Credential Manager (`autolinkpromax@github.com`).
5. **Client WebUI Detection**: Other AutoDoor devices fetch `raw.githubusercontent.com/autolinkpromax/AutoDoor-RF2/main/firmware/version.json`, detect the newer semver, and allow users to click 1-click online OTA.
