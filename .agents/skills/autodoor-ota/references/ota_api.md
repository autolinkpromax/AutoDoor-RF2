# AutoDoor-RF2 OTA Technical API Reference

This document details the internal HTTP endpoints and partition scheme used by AutoDoor-RF2 for Over-the-Air (OTA) updates.

## 1. Partition Scheme (`partitions_two_ota.csv`)
AutoDoor-RF2 uses a 4MB SPI Flash layout with dual factory/OTA partitions:

```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x1E0000, # ~1.875 MB
app1,     app,  ota_1,   0x1F0000,0x1E0000, # ~1.875 MB
spiffs,   data, spiffs,  0x3D0000,0x20000,
coredump, data, coredump,0x3F0000,0x10000,
```

- When the active app is `app0`, the next OTA binary is written to `app1`.
- When the active app is `app1`, the next OTA binary is written to `app0`.
- Upon successful verification, the ESP-IDF OTA bootloader marks the new partition as bootable and activates it upon reboot.

---

## 2. API Endpoints

### 2.1 `GET /api/status`
Returns general status, network configuration, and OTA metadata.

**Response Structure (OTA sub-object):**
```json
{
  "ok": true,
  "fwVersion": "1.0.0",
  "buildDate": "Sep 2 2026 22:45:27",
  "doorState": "closed",
  "ota": {
    "runningPartition": "app0",
    "nextPartition": "app1",
    "otaValid": true,
    "flashSize": 4194304,
    "freeSketchSpace": 1900544,
    "sketchSize": 1412992
  }
}
```

### 2.2 `GET /api/health`
Lightweight health check returning version and compilation date.

```json
{
  "ok": true,
  "fwVersion": "1.0.0",
  "buildDate": "Sep 2 2026 22:45:27"
}
```

### 2.3 `POST /api/ota`
Receives the multipart binary payload.

- **Content-Type**: `multipart/form-data; boundary=...`
- **Field Name**: `update`
- **Filename**: any `.bin` filename (e.g. `firmware_autodoor_ota.bin`)
- **Headers (Optional)**: `Authorization: Basic <base64(:password)>` if web authentication is enabled.

**Success Response (HTTP 200):**
```json
{
  "ok": true,
  "message": "อัปเดตเฟิร์มแวร์สำเร็จ กำลังรีสตาร์ท..."
}
```

**Failure Response (HTTP 400):**
```json
{
  "ok": false,
  "error": "OTA ล้มเหลว (รหัสข้อผิดพลาด: <error_code>)"
}
```
Following a 200 response, the ESP32 waits 500ms and issues `ESP.restart()`.
