# Web UI — โครงสร้างและ workflow

## แก้โค้ดที่ไหน

| ไฟล์ | ใช้เมื่อ |
|------|----------|
| `js/src/*.js` | logic แอป (แก้ที่นี่เท่านั้น) |
| `html/partials/*.html` | markup แต่ละหน้า — ใช้ `data-action` ไม่ใช้ `onclick` |
| `css/*.css` | สไตล์ |
| `js/bootstrap.js`, `js/core/loader.js` | boot + lazy load views (ES module) |

**ห้ามแก้** `js/app-bundle.js` โดยตรง — ไฟล์นี้ generate จาก `js/src/`

## โมดูล JS (`js/src/`)

เรียง concat ตามชื่อไฟล์:

- `00-constants` … `19-system` — domain แยกชัด
- `99-init.js` — `initActions()` + export `window.checkWebLogin` / `showView` / `startLivePoll`

Event: `data-action` บน element → handler ใน `05-actions.js` (delegate บน `document.body`)

## คำสั่ง

```bash
# เร็ว — รวม JS อย่างเดียว (เช็ก syntax / diff)
python scripts/webui.py bundle
pio run -t bundle-webui

# pack assets → src/webui_assets.h + manifest (ไม่ compile)
python scripts/webui.py pack
pio run -t pack-webui

# build firmware (pack อัตโนมัติก่อน compile ทุกครั้ง)
python scripts/webui.py build
pio run

# อัปโหลดบอร์ด
python scripts/webui.py flash
pio run -t upload
```

`pio run` เรียก `scripts/pack_webui.py` เป็น pre-build อยู่แล้ว — แก้ `src/` แล้ว `pio run -t upload` พอ

## เพิ่ม action ใหม่

1. เขียน function ในโมดูลที่เหมาะสม
2. ลงทะเบียนใน `05-actions.js` (`onActionClick` / `onActionChange` / `onActionInput`)
3. HTML: `data-action="ชื่อ-action"` (+ `data-*` ถ้าต้องส่งพารามิเตอร์)
4. `python scripts/webui.py bundle` แล้ว `pio run -t upload`

## ทดสอบบนบอร์ด

เปิด IP เครื่องในเบราว์เซอร์ → hard refresh (`Ctrl+Shift+R`) หลัง flash เพื่อไม่ใช้ cache เก่า
