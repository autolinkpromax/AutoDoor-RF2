---
name: aipass-chat
description: Ultra-fast CLI bridge & skill for AI Pass (https://de.aipass.net/chat). Uses direct Chrome DevTools Protocol (CDP port 9222) via `scripts/aipass.js` to execute queries in 1-2 seconds with zero subagent tokens.
---

# AI Pass Fast CLI Bridge & Skill

Skill นี้ใช้สคริปต์ [aipass.js](../../scripts/aipass.js) เพื่อเชื่อมต่อกับแท็บ **AI Pass (`de.aipass.net/chat`)** บน Google Chrome (Debug Port 9222) โดยตรงผ่าน Chrome DevTools Protocol (CDP) WebSocket:
* **ความเร็วระดับเสี้ยววินาที**: ส่งและรับคำตอบทันทีผ่าน Native CDP Event Streaming
* **ประหยัด Token 100%**: ไม่ต้องเสีย Token เรียก Subagent หรืออ่าน DOM ให้เปลือง Context
* **CDP Native Input**: ใช้ `Input.insertText` ป้อนข้อความเข้า React Textarea โดยตรง และคลิกปุ่ม `data-testid="send-button"` 100% แม่นยำ
* **Smart Model Switcher**: สลับโมเดลอัตโนมัติผ่าน Modal และคลิกปุ่ม "เลือก" + "ยืนยัน" ให้อัตโนมัติ

---

## 🚀 วิธีเรียกใช้งานทันที (One-Line Execution)

รันคำสั่งผ่าน `run_command` ในเทอร์มินัลได้ทันที:

### 1. ถามคำถามทั่วไป
```bash
node scripts/aipass.js "คำถามที่ต้องการถาม..."
```

### 2. สลับโมเดลอัตโนมัติ (เช่น Claude Opus 5.5, Kimi, DeepSeek, Sonar)
```bash
node scripts/aipass.js "วิเคราะห์โค้ดสถาปัตยกรรมนี้..." -m "Claude Opus 5.5"
```

### 3. บันทึกผลลัพธ์ลงไฟล์โดยตรง
```bash
node scripts/aipass.js "เขียนคู่มือ API..." -m "Claude Opus 5.5" -o "docs/api_guide.md"
```

### 4. เรียกใช้เป็น Library ใน Node.js
```javascript
import { askAiPass } from './scripts/aipass.js';

const reply = await askAiPass('ช่วยออกแบบ Schema...', { model: 'Claude Opus 5.5' });
console.log(reply);
```

---

## 🧠 กฎเหล็ก: การวิเคราะห์และเลือก AI Model เพื่อประหยัดเครดิต (Credit Optimization Rule)

> [!IMPORTANT]
> **เรามีเครดิตใช้งานจำกัด!** ก่อนส่งคำถามทุกครั้ง ตัวแทน AI **ต้องประเมินระดับความยากและประเภทงานก่อนเสมอ** แล้วเลือกโมเดลที่คุ้มค่าเครดิตที่สุด ห้ามเรียกโมเดลตัวแพง (เช่น Claude Opus) กับคำถามทั่วไปเด็ดขาด!

### 📊 ลำดับชั้นการเลือกโมเดล (Decision Matrix by Tier & Cost)

| ระดับงาน (Tier) | ลักษณะงาน / ประเภทคำถาม | โมเดลแนะนำ (Flag `-m`) | อัตรากินเครดิต |
| :--- | :--- | :--- | :--- |
| **Tier 0: ฟรี / ประหยัดสูงสุด**<br>*(งานเบา งานถามตอบทั่วไป)* | • ถามความหมาย อธิบายคอนเซปต์สั้นๆ<br>• แปลภาษา สรุปเนื้อหาเบื้องต้น<br>• ระดมไอเดียเบื้องต้น (Brainstorming ทั่วไป)<br>• ตรวจทานคำสะกด จัดฟอร์แมต JSON/YAML | **`Gemini 3.5 Flash Lite`**<br>(หรือ **`Auto`**) | **ฟรี / 0 เครดิต**<br>(ตอบเร็วที่สุด) |
| **Tier 1: ประหยัด คุ้มค่า & งานภาษา**<br>*(งานเร็ว ภาษาไทย และค้นหาเว็บ)* | • สรุปเอกสารภาษาไทย ร่างอีเมล/ข้อความทางการ<br>• ค้นหาข้อมูลสด Real-time จากอินเทอร์เน็ต<br>• งาน Software Engineering ทั่วไป, ออกแบบ REST API เบื้องต้น<br>• ช่วยเขียน regex, ช่วยเขียน bash/powershell สั้นๆ | • งานค้นหาเว็บ: **`Sonar`**<br>• ภาษาไทย/เอกสาร: **`Qwen3-Next`**<br>• งาน Dev รวดเร็ว: **`Gemini 3.8 Flash`** | **ประหยัดมาก**<br>(ใช้เครดิตน้อย) |
| **Tier 2: งานโค้ดและไอเดียเชิงลึก**<br>*(งานโปรแกรมมิ่งจริงจัง แก้บัก)* | • เขียนฟังก์ชัน C++/JS/Python, แก้บั๊ก (Bug fixing)<br>• เขียนสคริปต์ยาวๆ, Debug โค้ดที่ติดปัญหา<br>• ไอเดียผลิตภัณฑ์แปลกใหม่, วิเคราะห์ตรรกะข้อมูล | • โค้ดรวดเร็ว: **`Claude Sonnet 5`**<br>• สคริปต์/Debug ยาวๆ: **`Kimi K2.7 Code`**<br>• ตรรกะ/คณิตศาสตร์: **`DeepSeek V3.2`**<br>• วิเคราะห์ตรรกะ/ไอเดียใหม่: **`Grok 4.6`** | **ปานกลาง**<br>(คุ้มค่าประสิทธิภาพ) |
| **Tier 3: มหาเทพเรือธง (ใช้เฉพาะงานยากที่สุด)**<br>*(งาน Critical Architecture)* | • ออกแบบสถาปัตยกรรมระบบขั้นสูง (System Architecture)<br>• ตรรกะซับซ้อนระดับ Multi-layer / Concurrency / Memory Safety<br>• งาน Refactor ระบบใหญ่ที่มีผลกระทบกว้างขวาง<br>*(ห้ามใช้กับคำถามสั้นๆ หรือไอเดียทั่วๆ ไป)* | **`Claude Opus 5.5`** | **กินเครดิตสูงสุด**<br>(ใช้เมื่อจำเป็นจริงๆ เท่านั้น) |

---

## ⚡ Model Quick-Lookup (ตารางอ้างอิงคุณสมบัติ)

| โมเดล (Flag `-m`) | จุดเด่น | เมื่อไหร่ควรใช้ |
| :--- | :--- | :--- |
| **`Gemini 3.5 Flash Lite`** | ฟรี 100%, ไวมาก | คำถามทั่วไป, ถาม-ตอบ, แปลภาษา, งานที่ไม่ต้องการโค้ดระดับสูง |
| **`Gemini 3.8 Flash`** | ไว, เข้าใจโครงสร้าง API ดี | ออกแบบ Schema, JSON, เขียน API Controller ทั่วไป |
| **`Qwen3-Next`** | เก่งภาษาไทยมาก | เขียนคำอธิบายภาษาไทย, ร่างข้อความ, งานเอกสาร |
| **`Sonar`** | ค้นหาเว็บสด (Web Search) | เมื่อต้องการข้อมูลอัปเดตล่าสุดจากอินเทอร์เน็ต |
| **`Claude Sonnet 5`** | เขียนโค้ดเร็ว แม่นยำ ฉลาด | เขียนฟีเจอร์ใหม่, แก้บักโค้ด, เขียน Unit test |
| **`Kimi K2.7 Code`** | เก่งโค้ดยาวๆ บริบทกว้าง | สคริปต์เครื่องมือ, สแกนไฟล์โค้ดยาวๆ, Debugging |
| **`DeepSeek V3.2`** | คำนวณ, อัลกอริทึม | งานตรรกะตัวเลข, คำนวณ Cryptography, Optimize อัลกอริทึม |
| **`Grok 4.6`** | ตรรกะเฉียบคม, ไอเดียสดใหม่ | คิด Use case หลุดกรอบ, วิเคราะห์ทางเลือกแก้ปัญหา |
| **`Claude Opus 5.5`** | ฉลาดที่สุด เหตุผลลึกซึ้ง | สถาปัตยกรรมระดับ Core, State Machine ซับซ้อน |

---

## 🛠️ โครงสร้างสคริปต์
* ไฟล์หลัก: [scripts/aipass.js](file:///c:/Users/max/Desktop/workplace/QRPlay/scripts/aipass.js)
* พอร์ตเชื่อมต่อ: `http://127.0.0.1:9222` (Chrome Remote Debugging)
* ทำงานด้วย Native WebSocket ของ Node.js ไม่ต้องติดตั้งโมดูลภายนอกเพิ่ม
