#!/usr/bin/env python3
"""Convenience shortcut to flash both AutoDoor devices (192.168.1.11 and 192.168.1.12)."""
import sys
from ota_upload import main

if __name__ == "__main__":
    if "--both" not in sys.argv:
        sys.argv.insert(1, "--both")
    main()
