---
name: aipass-image
description: Ultra-fast CLI image generator for AI Pass (https://de.aipass.net/chat). Uses direct Chrome DevTools Protocol (CDP port 9222) via `scripts/aipass-image.js` to generate and download AI images (Nano Banana 2, Seedream, GPT-Image) in seconds with zero subagent tokens.
---

# AI Pass Fast Image Generation Skill

Skill นี้ใช้สคริปต์ [aipass-image.js](../../scripts/aipass-image.js) เพื่อสั่งสร้างรูปภาพผ่าน **AI Pass (`de.aipass.net/chat`)** บน Google Chrome (Debug Port 9222) โดยตรงผ่าน Chrome DevTools Protocol (CDP):
* **ประหยัด Token 100%**: ทำงานผ่าน CLI โดยตรงนอก LLM Context ไม่เสีย Token ในการอ่านหน้าจอหรือเรียก Browser Subagent
* **รวดเร็ว**: สลับโมเดลภาพ, กรอก Prompt, ส่งคำสั่ง, รอผลลัพธ์ และดาวน์โหลดไฟล์ลงเครื่องทันทีในคำสั่งเดียว
* **รองรับโมเดลภาพยอดนิยม**: Google Imagen 3 (Nano Banana), ByteDance SD (Seedream), และ OpenAI DALL-E (GPT-Image)

---

## 🚀 วิธีเรียกใช้งาน (One-Line Execution)

รันคำสั่งผ่าน `run_command` ในเทอร์มินัลได้ทันที:

### 1. สร้างภาพทั่วไป (ใช้โมเดลเริ่มต้น Nano Banana 2)
```bash
node scripts/aipass-image.js "Modern coffee shop counter with digital payment terminal, cinematic lighting" -o "docs/images/cafe_pos.png"
```

### 2. กำหนดโมเดลภาพที่ต้องการ
```bash
# ใช้ Nano Banana 2 (Google Imagen 3 - คมชัดสูง ตัวหนังสือบนภาพเป๊ะ)
node scripts/aipass-image.js "Product photography of ESP32 CYD terminal" -m "Nano Banana 2" -o "docs/images/cyd_render.png"

# ใช้ GPT-Image-2.5 Flare (DALL-E 3)
node scripts/aipass-image.js "Futuristic cyberpunk POS device glowing in neon lights" -m "GPT-Image-2.5 Flare" -o "docs/images/cyberpunk.png"

# ใช้ Seedream 5.0 Lite
node scripts/aipass-image.js "Isometric 3D icon of payment terminal" -m "Seedream 5.0 Lite" -o "docs/images/terminal_icon.png"
```

---

## 🎨 โมเดลภาพที่รองรับ (Supported Image Models)

| โมเดล (Flag `-m`) | จุดเด่น |
| :--- | :--- |
| **`Nano Banana 2`** *(Default)* | **แนะนำสูงสุด**: ภาพถ่าย Realistic, เรนเดอร์อุปกรณ์/ฮาร์ดแวร์, ตัวหนังสือบนหน้าจอคมชัดมาก |
| **`Nano Banana Pro`** | คุณภาพสูง รายละเอียดพื้นผิว แสงและเงาสมจริง |
| **`GPT-Image-2.5 Flare`** | สไตล์ศิลปะ, ภาพวาด, แนวแฟนตาซี หรือภาพประกอบคอนเซ็ปต์ (DALL-E 3) |
| **`GPT-Image-2.5 Sunburst`** | ความคิดสร้างสรรค์สูง คมชัด |
| **`Seedream 5.0 Lite`** | รวดเร็ว สไตล์โมเดิร์น 3D / Illustration |
| **`Seedream 4.0`** | โมเดลเสถียร ใช้พลังงานประมวลผลน้อย |

---

## 🛠️ รายละเอียดพารามิเตอร์ CLI

```text
Usage: node scripts/aipass-image.js "<prompt>" [-m "<model>"] [-o "<output_path>"]

Arguments:
  <prompt>        ข้อความบรรยายภาพที่ต้องการสร้าง (แนะนำภาษาอังกฤษสำหรับความแม่นยำสูงสุด)
  -m, --model     ชื่อโมเดลสร้างภาพ (ค่าเริ่มต้น: "Nano Banana 2")
  -o, --output    พาธไฟล์รูปภาพผลลัพธ์ที่ต้องการบันทึก (เช่น "docs/images/cover.png")
```
