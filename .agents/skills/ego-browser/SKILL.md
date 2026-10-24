---
name: ego-browser
description: Ultra-fast automation controller for ego (lite) Chromium browser. Operates user-authenticated tabs, Spaces, forms, clicks, snapshots, and web tasks using the embedded Node.js engine with zero subagent tokens.
---

# Ego Browser Automation Skill

Skill นี้ใช้ควบคุม **ego (lite)** บน Windows ผ่าน embedded engine (`ego-browser nodejs`) และ [scripts/ego-runner.js](../../scripts/ego-runner.js):
* **ความเร็วสูงระดับ Native**: ส่งคำสั่งตรงเข้า ego runtime ทำงานทันที 1-2 วินาที
* **ใช้ Session และ Login จริง**: ดึงสิทธิ์ของเว็บที่ล็อกอินค้างไว้บนเครื่องผู้ใช้ได้ทันที
* **ระบบ TaskSpace แยกอิสระ**: Agent ทำงานใน Space ของตัวเอง ไม่รบกวนแท็บทั่วไปของผู้ใช้
* **ประหยัด Token 100%**: ไม่ต้องใช้ Subagent สแกน DOM หรือเรนเดอร์ภาพเปลือง Token

---

## 🚀 วิธีสั่งงานผ่าน CLI ทันที (One-Line Execution)

### 1. เปิดเว็บและดึง Title / ตรวจสอบความพร้อม
```bash
node scripts/ego-runner.js --open "https://example.com"
```

### 2. เปิดเว็บและอ่าน Accessibility Snapshot (DOM Semantic Tree)
```bash
node scripts/ego-runner.js --open "https://example.com" --snapshot
```

### 3. รันโค้ด JavaScript ควบคุมหน้าเว็บแบบเต็มรูปแบบ (`--code`)
ส่ง JavaScript code ที่มี `taskSpace`, `page.click()`, `page.fill()`, `page.snapshot()` เข้าไปตรงๆ:

```bash
node scripts/ego-runner.js --code "
const task = await taskSpace('my-task');
const page = task.page('p1');
await page.goto('https://example.com');
console.log(await page.title());
await task.finish({ keep: [] });
"
```

---

## 🛠️ รูปแบบคำสั่ง Page API ที่รองรับ

ภายในสคริปต์ `taskSpace` คุณสามารถใช้ API มาตรฐานของ ego lite:

### การนำทางและอ่านข้อมูล
* `await page.goto(url, { waitUntil: 'load' })` - ไปยัง URL
* `await page.title()` - อ่าน Title หน้าเว็บ
* `await page.snapshot()` - แคป Accessibility Tree ของ Viewport
* `await page.snapshot({ scope: 'full_page' })` - แคปทั้งหน้าเว็บ
* `await page.screenshot({ path: 'C:/path/to/shot.png' })` - บันทึกภาพหน้าจอ

### การโต้ตอบกับปุ่มและฟอร์ม
* `await page.click('loc=role:button[name=\"Submit\"]')` - คลิกปุ่มตาม Semantic Role
* `await page.click('@12')` - คลิก Element ตาม Node ID จาก Snapshot
* `await page.fill('input[type=\"text\"]', 'ข้อความ')` - กรอกข้อความ
* `await page.waitForSelector('selector', { state: 'visible' })` - รอ Element ปรากฏ

### การจบ TaskSpace (สำคัญ)
* `await task.finish({ keep: [] })` - ปิด TaskSpace เมื่อทำงานเสร็จ ไม่ทิ้งแท็บค้าง
* `await task.finish({ keep: ['p1'] })` - เก็บบานหน้าต่างผลลัพธ์ไว้ให้ผู้ใช้ดูต่อในบราวเซอร์
