# tag-esp8285 — Problems & Fixes (AI context)

Project: ESP8285 tag firmware (ESP-01 form factor), ported from `tag-esp32c6`.  
Hub protocol: `TagBeacon` (`0x41 0x44`) + `ReceiverAnnounce` (`0x41 0x52`), default ESP-NOW channel **9**.

---

## Hardware: ESP8285 / ESP-01

### Minimum wiring (standalone, not USB adapter)

| Pin | Connect |
|-----|---------|
| VCC | 3.3V stable (≥500mA peak for WiFi TX) |
| GND | GND |
| CH_PD (EN) | **VCC** — most common mistake; chip won't run if floating |
| GPIO0 | HIGH or 10k to VCC (run mode; LOW = flash mode) |
| GPIO2 | leave open (module LED often pulls HIGH) |
| TX/RX | only for debug/upload |

Add 100µF + 100nF near module. GPIO15 is internal GND on ESP-01 boards.

### USB adapter (CH340) vs portable use

- Hello World / Serial on USB adapter **works** — proves chip + UART OK.
- ESP-NOW TX **fails** if powered only from CH340 3.3V (insufficient current).
- For portable tag: external 3.3V LDO + CH_PD tied to VCC.

---

## PlatformIO

| Env | Purpose |
|-----|---------|
| `esp8285` | Tag firmware + Serial debug (`TAG_DEBUG_SERIAL=1`) |
| `esp8285-release` | Tag firmware, no Serial |
| `esp8285-hello` | Hello World serial test (`hello/src/main.cpp`) |

Board settings for ESP-01 / ESP8285:

```ini
board = esp8285
board_build.flash_mode = dout
board_build.f_flash = 26000000L
upload_resetmethod = nodemcu
```

Upload: `pio run -e esp8285 -t upload --upload-port COMx`  
Monitor: `pio device monitor -p COMx -b 115200`

---

## Problem 1: Tag cannot connect to hub (no announce, no learn)

### Symptoms

- Channel scan runs (`[ch] auto scan try ch=...`) but never `[hub] announce`.
- Hub Learn mode does not see tag.

### Causes (non-firmware)

1. CH_PD not connected to VCC when off USB adapter.
2. Weak 3.3V supply (brownout during WiFi TX).
3. Hub Learn not active (30s window).
4. Hub ESP-NOW channel differs from tag — tag should auto-scan 1–13 if announce works.

### Firmware expectation

Tag scans ch 1–13 every 100ms until it receives `ReceiverAnnounce`, then locks channel + saves EEPROM.

---

## Problem 2: `esp_now_send err=-3` on every packet (CRITICAL)

### Symptoms

```
[espnow] send err=-3 seq=0 ch=1
[espnow] send err=-3 seq=1 ch=1
...
```

- Error **-3** = `ESP_ERR_ESPNOW_INTERNAL` on ESP8266/8285 SDK.
- Channel hop logic runs but **zero beacons transmitted**.
- No hub announce received (tag can't be heard).

### Root cause

1. **`ESP_NOW_ROLE_COMBO`** — on ESP8266 Arduino core 3.x, COMBO role breaks `esp_now_send` (returns -3).  
   Reference: `ESP8266WiFiMesh` uses **`ESP_NOW_ROLE_CONTROLLER`** for TX and still registers `recv_cb`.

2. **Wrong init order** — role/callbacks/peers must follow ESP8266 pattern:
   ```
   esp_now_init()
   esp_now_register_recv_cb()
   esp_now_register_send_cb()   // dummy OK
   esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER)
   wifi_set_channel(ch)
   esp_now_add_peer(broadcast, ...)
   ```

3. **Peer churn on channel hop** — `esp_now_del_peer` + `esp_now_add_peer` on every 100ms hop can destabilize radio. Use `esp_now_set_peer_channel()` when peer exists.

4. **`WiFi.disconnect(true)`** — erasing WiFi config can upset ESP-NOW; use `WiFi.disconnect()` without erase.

5. **Heavy Serial logging** during send bursts can worsen timing; rate-limit error logs.

### Fix (implemented in `src/main.cpp`)

- Role: **`ESP_NOW_ROLE_CONTROLLER`** (not COMBO).
- Init order: register recv + send cb → set_role(CONTROLLER) → channel → add_peer.
- Channel change: `wifi_set_channel()` + `delay(5)` + `esp_now_set_peer_channel()`.
- `WiFi.setPhyMode(WIFI_PHY_MODE_11N)`, `WiFi.setSleepMode(WIFI_NONE_SLEEP)`.
- Rate-limit send error logs (every 3s).
- Auto-recovery: if 30 bursts with 0 OK sends → `esp_now_deinit`/reinit + clear bad EEPROM channel.

### Verify fix

After boot, Serial must show:

```
[boot] first bursts ok=9 fail=0
```

`ok` must be > 0. If `fail` only → ESP-NOW still broken (check role/init/power).

---

## Problem 3: EEPROM saved wrong channel (`eeprom=loaded`, start ch=1)

### Symptoms

- Boot log: `start ch=1 default=9 eeprom=loaded`
- Tag starts on wrong channel; still scans if no hub lock.

### Cause

Channel was persisted to EEPROM during earlier failed sessions or wrong announce.

### Fix

- Auto-recovery clears EEPROM if send never succeeds.
- Manual: reflash or call `clearSavedChannel()` path.
- On successful hub lock, correct channel is re-saved.

---

## Problem 4: Serial log order confusing

### Symptom

`send err` lines appeared **before** `=== AutoDoor Tag ESP8285 ===` banner.

### Cause

`sendBeaconBurst()` called before banner print in old `setup()`.

### Fix

Print banner + init ESP-NOW first, then send boot bursts; log `[boot] first bursts ok=... fail=...`.

---

## Serial debug log reference

| Log | Meaning |
|-----|---------|
| `[ch] auto scan try ch=N` | Scanning — no hub heard in 15s |
| `[ch] scan hop X -> Y` | WiFi channel changed during scan |
| `[hub] announce ch=N` | Received hub `ReceiverAnnounce` |
| `[hub] found — stop channel scan` | Locked onto hub |
| `[ch] N -> M (saved, hub locked)` | Channel saved to EEPROM |
| `[stat] SCAN ...` | Scanning, shows ok/fail send counts |
| `[stat] LOCKED ch=N ...` | Hub heard recently, staying on channel |
| `[espnow] send err=-3` | **Broken** — see Problem 2 |
| `[boot] first bursts ok=N fail=0` | **Healthy ESP-NOW TX** |

---

## Code layout

```
tag-esp8285/ (actually tag-esp8266/ in this workspace)
├── src/main.cpp          # Tag ESP-NOW firmware
├── hello/src/main.cpp    # Serial Hello World test
├── platformio.ini
└── AI-NOTES.md           # this file
```

Do **not** use `hello` env for tag testing. Use `esp8285`.

---

## When modifying this firmware (AI checklist)

1. Never use `ESP_NOW_ROLE_COMBO` on ESP8266/8285 for send+recv — use **CONTROLLER** + `recv_cb`.
2. Keep init order: init → recv_cb → send_cb → set_role → channel → peer.
3. On channel change prefer `esp_now_set_peer_channel()` over del/add peer.
4. Add `delay(5)` after `wifi_set_channel()` before send.
5. `IRAM_ATTR` recv callback: use literal bytes (`0x41`, `0x52`), not flash constants.
6. `tagId` = `ESP.getChipId()` (not `getEfuseMac()` — that's ESP32).
7. EEPROM for channel persistence (ESP32 tag uses Preferences).
8. Match hub structs in `src/app/types.h` (parent project).

---

## Related projects

| Project | MCU | ESP-NOW API |
|---------|-----|-------------|
| `tag-esp32c6` | ESP32-C6 | `esp_now.h` (IDF style) |
| `tag-esp32c3` | ESP32-C3 | same as C6 |
| `tag-esp8285` | ESP8285 | `espnow.h` (legacy ESP8266) |
| `tag-esp8266` | ESP8266 | same as 8285 |
| Hub `esp-now/` | ESP32-C3 | receiver + announce |

ESP8285 code must follow **ESP8266** patterns, not ESP32 — especially role and peer APIs.
