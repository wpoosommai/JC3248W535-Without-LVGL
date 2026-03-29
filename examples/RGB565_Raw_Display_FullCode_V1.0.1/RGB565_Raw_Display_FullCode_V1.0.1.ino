/*
===============================================================================
โปรแกรม : RGB565_Raw_Display_FullCode_V1.0.1.ino
Version : V1.0.1
วันที่แก้ไข : 2026-03-29
เวลาแก้ไข : 07:35
ผู้พัฒนา : OpenAI ChatGPT

สรุปการทำงานของโปรแกรม
- โปรแกรมนี้ใช้สำหรับแสดงภาพ RGB565 แบบดิบจากไฟล์ header ชื่อ dashboard_frame_V1.h
- ไม่ต้องถอดรหัส JPEG / PNG เพราะข้อมูลภาพถูกแปลงเป็น uint16_t RGB565 แล้ว
- โปรแกรมจะเริ่มต้นจอ TFT, เปิดไฟ backlight, ทดสอบสีพื้นฐาน, ตรวจค่าพิกเซลแรกของภาพ
  และแสดงภาพเต็มจอ
- ออกแบบสำหรับบอร์ด JC3248W535EN (ESP32-S3) ที่ใช้ Arduino_GFX_Library
- เหมาะสำหรับ splash screen, dashboard frame, ภาพพื้นหลังคงที่ และการเรียนการสอนเรื่อง
  การแสดงผลภาพในระบบสมองกลฝังตัว

คุณสมบัติโปรแกรม
1) เปิดใช้งานจอ 320x480
2) เปิด backlight อัตโนมัติ
3) ทดสอบสีพื้นฐานเต็มจอ
4) ตรวจค่าพิกเซลแรกจาก array ภาพผ่าน Serial Monitor
5) แสดงภาพ RGB565 แบบเต็มจอจากไฟล์ header
6) มี comment อธิบายทุกส่วนเพื่อใช้ในการเรียนการสอน
7) มีเลข Version และวันเวลาแก้ไขชัดเจน

รูปแบบคำสั่งควบคุม
- โปรแกรมนี้ไม่มีการรับคำสั่ง JSON จริงจากภายนอก
- ควบคุมการทำงานโดยแก้ค่าคงที่ในโค้ด เช่น rotation, delay, และชื่อไฟล์ภาพ
- สามารถปรับการหมุนจอได้ที่ DISPLAY_ROTATION

ข้อมูล input
1) ไฟล์ dashboard_frame_V1.h ต้องอยู่ในโฟลเดอร์เดียวกับไฟล์ .ino
2) ภายใน dashboard_frame_V1.h ต้องมีองค์ประกอบดังนี้
   - #define DASHBOARD_FRAME_WIDTH 320
   - #define DASHBOARD_FRAME_HEIGHT 480
   - static const uint16_t dashboard_frame[] PROGMEM
3) ติดตั้งไลบรารี Arduino_GFX_Library แล้ว
4) ใช้บอร์ด ESP32-S3 ที่แมปขาจอตามรุ่น JC3248W535EN

คำสั่งการควบคุมในโปรแกรม
- showColorTest()              : ทดสอบสีพื้นฐานของจอ
- showImageInfo()              : แสดงข้อมูลขนาดภาพและค่าพิกเซลแรกทาง Serial
- drawDashboardFrame()         : วาดภาพ RGB565 แบบเต็มจอ
- drawStatusText(...)          : แสดงข้อความสถานะบนจอ

ตัวอย่าง JSON ควบคุมระบบเพื่อการสอน
{
  "cmd": "show_rgb565_header_image",
  "image_name": "dashboard_frame",
  "x": 0,
  "y": 0,
  "rotation": 0,
  "format": "RGB565",
  "width": 320,
  "height": 480
}

หมายเหตุเพื่อการพัฒนา
- ถ้าภาพหมุนผิด ให้เปลี่ยน DISPLAY_ROTATION เป็น 0, 1, 2 หรือ 3
- ถ้าภาพยังไม่แสดง ให้ตรวจว่า include ชื่อไฟล์ถูกต้องและ array อยู่ครบ
- ถ้าสีผิดปกติ อาจเกิดจาก byte order ของข้อมูลภาพไม่ตรงกับฟังก์ชันที่ใช้วาด
- เวอร์ชันนี้ตั้งใจให้ใช้งานกับไฟล์ dashboard_frame_V1.h ที่แปลงจากภาพล่าสุด
===============================================================================
*/

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "dashboard_frame_V1.h"

// ========================= กำหนดสี RGB565 =========================
static const uint16_t COLOR_BLACK   = 0x0000;
static const uint16_t COLOR_BLUE    = 0x001F;
static const uint16_t COLOR_RED     = 0xF800;
static const uint16_t COLOR_GREEN   = 0x07E0;
static const uint16_t COLOR_CYAN    = 0x07FF;
static const uint16_t COLOR_MAGENTA = 0xF81F;
static const uint16_t COLOR_YELLOW  = 0xFFE0;
static const uint16_t COLOR_WHITE   = 0xFFFF;

// ========================= กำหนดค่าฮาร์ดแวร์ =========================
#define GFX_BL 1
#define DISPLAY_ROTATION 0
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 480

// -----------------------------------------------------------------------------
// กำหนดบัส QSPI สำหรับจอของบอร์ด JC3248W535EN
// พารามิเตอร์คือ CS, SCK, D0, D1, D2, D3
// -----------------------------------------------------------------------------
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  45,
  47,
  21,
  48,
  40,
  39
);

// -----------------------------------------------------------------------------
// กำหนดไดรเวอร์จอ AXS15231B ขนาด 320x480
// -----------------------------------------------------------------------------
Arduino_GFX *gfx = new Arduino_AXS15231B(
  bus,
  GFX_NOT_DEFINED,
  0,
  false,
  SCREEN_WIDTH,
  SCREEN_HEIGHT
);

// -----------------------------------------------------------------------------
// ฟังก์ชันแสดงข้อความสถานะบนจอ
// ใช้สำหรับแจ้งขั้นตอนการทำงาน เช่น Initializing, Testing, Showing Image
// -----------------------------------------------------------------------------
void drawStatusText(const char *msg, uint16_t textColor) {
  gfx->fillScreen(COLOR_BLACK);
  gfx->setTextSize(2);
  gfx->setTextColor(textColor, COLOR_BLACK);
  gfx->setCursor(16, 30);
  gfx->println("RGB565 RAW DISPLAY");
  gfx->setCursor(16, 65);
  gfx->println(msg);
}

// -----------------------------------------------------------------------------
// ฟังก์ชันทดสอบสีพื้นฐานของจอ
// จบด้วยสีดำ ไม่จบด้วยสีขาว เพื่อไม่ให้สับสนว่าภาพไม่ถูกวาด
// -----------------------------------------------------------------------------
void showColorTest() {
  Serial.println("[STEP] Color test start");

  gfx->fillScreen(COLOR_BLACK);   delay(250);
  gfx->fillScreen(COLOR_BLUE);    delay(250);
  gfx->fillScreen(COLOR_RED);     delay(250);
  gfx->fillScreen(COLOR_GREEN);   delay(250);
  gfx->fillScreen(COLOR_CYAN);    delay(250);
  gfx->fillScreen(COLOR_MAGENTA); delay(250);
  gfx->fillScreen(COLOR_YELLOW);  delay(250);
  gfx->fillScreen(COLOR_BLACK);   delay(250);

  Serial.println("[STEP] Color test complete");
}

// -----------------------------------------------------------------------------
// ฟังก์ชันแสดงข้อมูลภาพผ่าน Serial Monitor
// ใช้ตรวจสอบขนาดภาพและค่าพิกเซลแรก เพื่อตรวจเบื้องต้นว่าข้อมูลภาพถูกอ่านได้
// -----------------------------------------------------------------------------
void showImageInfo() {
  uint16_t firstPixel = dashboard_frame[0];

  Serial.println("========================================");
  Serial.println("RGB565 IMAGE INFORMATION");
  Serial.println("========================================");
  Serial.print("Width  : ");
  Serial.println(DASHBOARD_FRAME_WIDTH);
  Serial.print("Height : ");
  Serial.println(DASHBOARD_FRAME_HEIGHT);
  Serial.print("First pixel (HEX) : 0x");
  Serial.println(firstPixel, HEX);
  Serial.println("========================================");

  // วาดสี่เหลี่ยมตัวอย่างสีของพิกเซลแรก เพื่อดูว่า array ถูกอ่านจริง
  gfx->fillScreen(COLOR_BLACK);
  gfx->fillRect(0, 0, 100, 100, firstPixel);
  gfx->setTextSize(2);
  gfx->setTextColor(COLOR_WHITE, COLOR_BLACK);
  gfx->setCursor(16, 130);
  gfx->println("First Pixel Test");
  delay(1200);
}

// -----------------------------------------------------------------------------
// ฟังก์ชันวาดภาพเต็มจอจาก array RGB565
// ใช้ draw16bitRGBBitmap() เพราะข้อมูลใน header เป็น uint16_t RGB565
// -----------------------------------------------------------------------------
void drawDashboardFrame() {
  Serial.println("[STEP] Draw RGB565 image...");
  gfx->fillScreen(COLOR_BLACK);

  gfx->draw16bitRGBBitmap(
    0,
    0,
    dashboard_frame,
    DASHBOARD_FRAME_WIDTH,
    DASHBOARD_FRAME_HEIGHT
  );

  Serial.println("[STEP] Draw complete");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // เปิดไฟ backlight ของจอ
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  // เริ่มต้นจอแสดงผล
  if (!gfx->begin()) {
    Serial.println("ERROR: gfx->begin() failed");
    while (1) {
      delay(1000);
    }
  }

  // กำหนดการหมุนจอ
  gfx->setRotation(DISPLAY_ROTATION);

  // แสดงสถานะเริ่มต้น
  drawStatusText("Initializing display...", COLOR_YELLOW);
  delay(800);

  // ทดสอบการแสดงผลสีพื้นฐาน
  drawStatusText("Running color test...", COLOR_CYAN);
  delay(400);
  showColorTest();

  // แสดงข้อมูลภาพและทดสอบพิกเซลแรก
  drawStatusText("Checking image data...", COLOR_GREEN);
  delay(400);
  showImageInfo();

  // วาดภาพเต็มจอจริง
  drawStatusText("Drawing full image...", COLOR_WHITE);
  delay(400);
  drawDashboardFrame();

  Serial.println("System ready.");
}

void loop() {
  // เวอร์ชันนี้ให้ภาพค้างบนจอ ไม่ต้องทำงานซ้ำใน loop
  delay(1000);
}
