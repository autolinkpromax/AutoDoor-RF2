#!/usr/bin/env python3
"""Script to upload firmware and metadata to GitHub repository."""
import os
import json
import base64
import urllib.request
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FW_DIR = ROOT / "firmware"
REPO = "autolinkpromax/AutoDoor-RF2"

def get_git_token() -> str:
    token = os.environ.get("GITHUB_TOKEN") or os.environ.get("GH_TOKEN")
    if token:
        return token
    try:
        proc = subprocess.run(
            ["git", "credential", "fill"],
            input="protocol=https\nhost=github.com\n",
            capture_output=True,
            text=True,
            check=True
        )
        for line in proc.stdout.splitlines():
            if line.startswith("password="):
                return line.split("=", 1)[1].strip()
    except Exception:
        pass
    return ""

def upload_file(repo_path: str, local_path: Path, token: str, message: str) -> bool:
    if not local_path.exists():
        print(f"[github] Error: File {local_path} not found")
        return False
    data = local_path.read_bytes()
    b64_content = base64.b64encode(data).decode('utf-8')
    headers = {
        'Authorization': f'Bearer {token}',
        'Accept': 'application/vnd.github+json',
        'User-Agent': 'Python-AutoDoor',
        'Content-Type': 'application/json'
    }
    sha = None
    url = f'https://api.github.com/repos/{REPO}/contents/{repo_path}'
    req_get = urllib.request.Request(url, headers=headers)
    try:
        with urllib.request.urlopen(req_get) as resp:
            info = json.loads(resp.read().decode('utf-8'))
            sha = info.get('sha')
    except urllib.error.HTTPError:
        pass
    payload = {
        'message': message,
        'content': b64_content,
        'branch': 'main'
    }
    if sha:
        payload['sha'] = sha
    req_put = urllib.request.Request(url, data=json.dumps(payload).encode('utf-8'), headers=headers, method='PUT')
    try:
        with urllib.request.urlopen(req_put) as resp:
            print(f"[github] Uploaded {repo_path} ({len(data):,} bytes): HTTP {resp.status}")
            return True
    except urllib.error.HTTPError as e:
        print(f"[github] Failed {repo_path}: HTTP {e.code} {e.read().decode('utf-8')}")
        return False

def main():
    token = get_git_token()
    if not token:
        print("[github] Error: No GitHub token found in credentials or environment")
        return
    print(f"[github] Target Repository: {REPO}")
    upload_file('firmware/version.json', FW_DIR / 'version.json', token, 'Update firmware version metadata')
    upload_file('firmware/firmware_autodoor_ota.bin', FW_DIR / 'firmware_autodoor_ota.bin', token, 'Update firmware OTA binary')

if __name__ == "__main__":
    main()
