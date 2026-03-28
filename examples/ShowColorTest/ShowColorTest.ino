/*
============================================================
 Program Name : HelloWorld_DisplayTest_V1.5.2.ino
 Version      : V1.5.2
 Date         : 2026-03-28
 Time         : Asia/Bangkok
 Author       : OpenAI ChatGPT

 Description:
 โปรแกรมทดสอบจอแสดงผลสำหรับบอร์ด ESP32-S3 + LCD 320x480
 โดยแสดงผลสีแบบเต็มหน้าจอทีละสี ครบ 8 สีพื้นฐาน ได้แก่
 BLACK, BLUE, RED, GREEN, CYAN, MAGENTA, YELLOW, WHITE
 พร้อมแสดงชื่อสีบน Serial Monitor เพื่อใช้ตรวจสอบการทำงานของจอ,
 การแสดงสี, การหมุนจอ, และความถูกต้องของไดรเวอร์จอ

 Program Features:
 1) เริ่มต้นระบบจอแสดงผลด้วย Arduino_GFX_Library
 2) แสดงสีเต็มจอทีละสี
 3) หน่วงเวลาให้สังเกตแต่ละสีได้ชัดเจน
 4) วนลูปแสดงสีต่อเนื่องอัตโนมัติ
 5) แสดง log บน Serial Monitor

 Hardware/Library:
 - Board : ESP32-S3
 - Display: JC3248W535EN / LCD 320x480
 - Library: Arduino_GFX_Library

 Input:
 - ไม่มี input จากผู้ใช้
 - ระบบจะทำงานอัตโนมัติหลังเปิดเครื่อง

 JSON Control Format (สำหรับอ้างอิงมาตรฐานการพัฒนา):
 - โปรแกรมนี้ไม่มีการรับ JSON จริง แต่กำหนดรูปแบบไว้ใน header เพื่อใช้ต่อยอด
 Example:
   {
     "cmd": "show_color",
     "color": "RED",
     "duration_ms": 1000
   }

 Control Commands (แนวทางต่อยอด):
 - show_color : แสดงสีเต็มจอตามชื่อสี
 - duration_ms: เวลาหน่วงการแสดงผลต่อสี

 Notes:
 - หากจอภาพกลับหัวหรือหมุนผิดทิศ ให้ปรับค่า gfx->setRotation(0..3)
 - หากจอดำแต่บอร์ดทำงาน ให้ตรวจสอบขา Backlight และไฟเลี้ยงจอ
 - หากสีแสดงไม่ถูกต้อง ให้ตรวจสอบไลบรารีและไดรเวอร์จอที่ใช้งาน
============================================================
*/

#include <Arduino_GFX_Library.h>

// -----------------------------
// กำหนดค่าขา Backlight
// -----------------------------
#ifndef GFX_BL
#define GFX_BL 1
#endif

// -----------------------------
// กำหนดค่าสีแบบ RGB565
// -----------------------------
static const uint16_t BLACK   = 0x0000;
static const uint16_t BLUE    = 0x001F;
static const uint16_t RED     = 0xF800;
static const uint16_t GREEN   = 0x07E0;
static const uint16_t CYAN    = 0x07FF;
static const uint16_t MAGENTA = 0xF81F;
static const uint16_t YELLOW  = 0xFFE0;
static const uint16_t WHITE   = 0xFFFF;

// -----------------------------
// สร้างบัส QSPI สำหรับจอ
// -----------------------------
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  45 /* CS */, 47 /* SCK */, 21 /* D0 */, 48 /* D1 */, 40 /* D2 */, 39 /* D3 */
);

// -----------------------------
// สร้างออบเจ็กต์จอภาพ
// -----------------------------
Arduino_GFX *gfx = new Arduino_AXS15231B(
  bus,
  GFX_NOT_DEFINED /* RST */,
  0 /* rotation */,
  false /* IPS */,
  320 /* width */,
  480 /* height */
);

// -----------------------------
// โครงสร้างข้อมูลสีสำหรับวนแสดงผล
// -----------------------------
struct ColorItem {
  const char *name;
  uint16_t value;
};

ColorItem colorList[] = {
  {"BLACK",   BLACK},
  {"BLUE",    BLUE},
  {"RED",     RED},
  {"GREEN",   GREEN},
  {"CYAN",    CYAN},
  {"MAGENTA", MAGENTA},
  {"YELLOW",  YELLOW},
  {"WHITE",   WHITE}
};

const size_t COLOR_COUNT = sizeof(colorList) / sizeof(colorList[0]);
const uint16_t SHOW_TIME_MS = 300;

// -----------------------------
// ฟังก์ชันแสดงสีเต็มจอ
// -----------------------------
void showFullScreenColor(const char *colorName, uint16_t colorValue) {
  // เติมสีให้เต็มหน้าจอ
  gfx->fillScreen(colorValue);

  // ส่งข้อความแจ้งสถานะไปยัง Serial Monitor
  Serial.print("Showing color: ");
  Serial.println(colorName);
}

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("========================================");
  Serial.println("HelloWorld Display Test V1.5.2");
  Serial.println("Mode: Full Screen Color Test");
  Serial.println("========================================");

  // เปิดไฟ Backlight ของจอ
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  // เริ่มต้นจอภาพ
  if (!gfx->begin()) {
    Serial.println("ERROR: gfx->begin() failed");
    while (true) {
      delay(1000);
    }
  }

  // ปรับทิศทางจอ หากภาพหมุนผิดให้เปลี่ยน 0,1,2,3
  gfx->setRotation(0);

  // ล้างจอเป็นสีดำก่อนเริ่ม
  gfx->fillScreen(BLACK);
  delay(500);
}

void loop() {
  // วนแสดงผลสีเต็มจอครบทุกสี
  for (size_t i = 0; i < COLOR_COUNT; i++) {
    showFullScreenColor(colorList[i].name, colorList[i].value);
    delay(SHOW_TIME_MS);
  }
}
