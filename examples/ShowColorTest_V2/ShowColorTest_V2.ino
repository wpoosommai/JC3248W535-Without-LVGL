/*
============================================================
 Program Name : HelloWorld_DisplayTest_V1.6.0.ino
 Version      : V1.6.0
 Date         : 2026-03-29
 Time         : Asia/Bangkok
 Author       : OpenAI ChatGPT

 Description:
 โปรแกรมทดสอบจอแสดงผลสำหรับบอร์ด ESP32-S3 + LCD 320x480
 เวอร์ชันนี้ปรับปรุงจากเดิมให้สามารถแสดงสีเต็มหน้าจอได้ 2 รูปแบบ คือ
 1) แสดงสีพื้นฐาน 8 สี
 2) แสดงสีแบบสุ่มจากช่วงสี 24-bit จำนวน 16,777,216 ค่าสี (16 ล้านสี)
 แล้วแปลงเป็น RGB565 เพื่อแสดงผลบนจอ LCD

 หมายเหตุสำคัญ:
 - ค่าสีที่สุ่มถูกสร้างจากระบบ 24-bit RGB (8 bit ต่อช่องสี)
   ซึ่งมีความเป็นไปได้ 16,777,216 สี
 - แต่จอ LCD ที่ใช้กับ Arduino_GFX ในโปรแกรมนี้แสดงผลจริงแบบ RGB565
   ดังนั้นค่าสี 24-bit จะถูกแปลงเป็น 16-bit ก่อนแสดงผล
 - จุดประสงค์คือทดสอบการสุ่มสีจากช่วง 16 ล้านสี และดูการตอบสนองของจอ

 Program Features:
 1) เริ่มต้นระบบจอแสดงผลด้วย Arduino_GFX_Library
 2) แสดงสีเต็มจอทีละสีแบบพื้นฐาน 8 สี
 3) แสดงสีเต็มจอแบบสุ่มจากช่วงสี 24-bit
 4) แสดงค่า R, G, B และค่า RGB565 ทาง Serial Monitor
 5) วนลูปทำงานอัตโนมัติอย่างต่อเนื่อง

 Hardware/Library:
 - Board : ESP32-S3
 - Display: JC3248W535EN / LCD 320x480
 - Library: Arduino_GFX_Library

 Input:
 - ไม่มี input จากผู้ใช้
 - ระบบทำงานอัตโนมัติหลังเปิดเครื่อง

 JSON Control Format (สำหรับอ้างอิงมาตรฐานการพัฒนา):
 Example:
   {
     "cmd": "show_random_color",
     "mode": "random24bit",
     "duration_ms": 500,
     "count": 20
   }

 Control Commands (แนวทางต่อยอด):
 - show_color         : แสดงสีตามชื่อสีพื้นฐาน
 - show_random_color  : แสดงสีสุ่มจากระบบ 24-bit
 - duration_ms        : เวลาหน่วงการแสดงผลต่อสี
 - count              : จำนวนรอบที่ต้องการสุ่ม

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
// กำหนดค่าสีแบบ RGB565 พื้นฐาน
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
// โครงสร้างข้อมูลสีพื้นฐาน
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
const uint16_t BASIC_SHOW_TIME_MS  = 700;
const uint16_t RANDOM_SHOW_TIME_MS = 250;
const uint8_t RANDOM_COLOR_COUNT   = 500;

// -----------------------------
// ฟังก์ชันแปลงค่าสี 24-bit RGB เป็น RGB565
// r = 0..255, g = 0..255, b = 0..255
// -----------------------------
uint16_t rgb888To565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// -----------------------------
// ฟังก์ชันแสดงสีเต็มจอจากชื่อสีพื้นฐาน
// -----------------------------
void showFullScreenColor(const char *colorName, uint16_t colorValue) {
  // เติมสีให้เต็มหน้าจอ
  gfx->fillScreen(colorValue);

  // ส่งข้อความแจ้งสถานะไปยัง Serial Monitor
  Serial.print("Showing basic color: ");
  Serial.println(colorName);
}

// -----------------------------
// ฟังก์ชันแสดงสีแบบสุ่มจาก 24-bit RGB
// -----------------------------
void showRandom24BitColor() {
  // สุ่มค่าสี R, G, B อย่างละ 8 bit
  uint8_t r = (uint8_t)random(0, 256);
  uint8_t g = (uint8_t)random(0, 256);
  uint8_t b = (uint8_t)random(0, 256);

  // แปลงเป็น RGB565 สำหรับจอ LCD
  uint16_t color565 = rgb888To565(r, g, b);

  // แสดงผลเต็มหน้าจอ
  gfx->fillScreen(color565);

  // แสดงรายละเอียดสีบน Serial Monitor
  Serial.print("Random 24-bit color -> ");
  Serial.print("R=");
  Serial.print(r);
  Serial.print(", G=");
  Serial.print(g);
  Serial.print(", B=");
  Serial.print(b);
  Serial.print(" | RGB565=0x");
  Serial.println(color565, HEX);
}

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("========================================");
  Serial.println("HelloWorld Display Test V1.6.0");
  Serial.println("Mode: Basic Colors + Random 24-bit Colors");
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

  // ตั้งค่า seed สำหรับ random
  // ใช้ค่าจากเวลาไมโครวินาทีเพื่อให้รูปแบบการสุ่มไม่ซ้ำเดิมทุกครั้ง
  randomSeed((uint32_t)micros());

  // ล้างจอเป็นสีดำก่อนเริ่ม
  gfx->fillScreen(BLACK);
  delay(500);
}

void loop() {
  // -----------------------------
  // ช่วงที่ 1: แสดงสีพื้นฐาน 8 สี
  // -----------------------------
  for (size_t i = 0; i < COLOR_COUNT; i++) {
    showFullScreenColor(colorList[i].name, colorList[i].value);
    delay(BASIC_SHOW_TIME_MS);
  }

  // -----------------------------
  // ช่วงที่ 2: แสดงสีสุ่มจากระบบ 24-bit
  // -----------------------------
  for (uint8_t i = 0; i < RANDOM_COLOR_COUNT; i++) {
    showRandom24BitColor();
    delay(RANDOM_SHOW_TIME_MS);
  }
}
