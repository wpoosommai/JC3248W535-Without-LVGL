/*
===============================================================================
โปรแกรม : RGB565_Raw_Display_FullCode_V1.0.2.ino
Version : V1.0.2
วันที่แก้ไข : 2026-03-29
เวลาแก้ไข : 07:55
ผู้พัฒนา : OpenAI ChatGPT

สรุปการทำงานของโปรแกรม
- โปรแกรมนี้ใช้แสดงภาพ RGB565 จากไฟล์ dashboard_frame_V1.h บนจอ 320x480
- เวอร์ชันนี้แก้ปัญหาภาพไม่แสดง โดยไม่ใช้ draw16bitRGBBitmap() ตรง ๆ
- เปลี่ยนเป็นวาดภาพแบบทีละพิกเซล เพื่อหลีกเลี่ยงปัญหา byte order / method mismatch
- รองรับการสลับ byte ของข้อมูลภาพด้วยตัวเลือก RGB565_SWAP_BYTES
- เหมาะสำหรับตรวจสอบภาพดิบ RGB565 บนบอร์ด JC3248W535EN (ESP32-S3)

คุณสมบัติโปรแกรม
1) เปิดใช้งานจอ 320x480
2) เปิด backlight อัตโนมัติ
3) ทดสอบสีพื้นฐาน
4) แสดงค่าพิกเซลแรกทาง Serial
5) วาดภาพเต็มจอแบบทีละพิกเซล
6) สามารถเปิด/ปิดการสลับ byte ได้
7) มี comment อธิบายเพื่อการเรียนการสอน

รูปแบบคำสั่งควบคุม
- โปรแกรมนี้ไม่มีการรับ JSON จริง
- ควบคุมโดยการแก้ค่าคงที่ในโค้ด
- ค่า RGB565_SWAP_BYTES = false  : ใช้ข้อมูลตามเดิม
- ค่า RGB565_SWAP_BYTES = true   : สลับ byte ของทุกพิกเซลก่อนแสดง

ข้อมูล input
1) dashboard_frame_V1.h ต้องอยู่ในโฟลเดอร์เดียวกับไฟล์ .ino
2) ภายในไฟล์ต้องมี
   - #define DASHBOARD_FRAME_WIDTH 320
   - #define DASHBOARD_FRAME_HEIGHT 480
   - static const uint16_t dashboard_frame[] PROGMEM
3) ใช้ Arduino_GFX_Library

คำสั่งการควบคุมในโปรแกรม
- showColorTest()            : ทดสอบสีพื้นฐานของจอ
- showImageInfo()            : แสดงขนาดภาพและค่าพิกเซลแรกทาง Serial
- renderImagePixelByPixel()  : วาดภาพเต็มจอทีละพิกเซล
- swap565(...)               : สลับ byte ของข้อมูลสี RGB565

ตัวอย่าง JSON ควบคุมระบบเพื่อการสอน
{
  "cmd": "show_rgb565_header_image",
  "image_name": "dashboard_frame",
  "x": 0,
  "y": 0,
  "rotation": 0,
  "format": "RGB565",
  "swap_bytes": false
}
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
// ตั้งค่า true หากภาพไม่แสดงหรือสีผิด ให้ลองสลับ byte
// false = ใช้ข้อมูลตามปกติ
// true  = สลับ byte ของ RGB565 ทุกพิกเซลก่อนวาด
// -----------------------------------------------------------------------------
#define RGB565_SWAP_BYTES false

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  45,
  47,
  21,
  48,
  40,
  39
);

Arduino_GFX *gfx = new Arduino_AXS15231B(
  bus,
  GFX_NOT_DEFINED,
  0,
  false,
  SCREEN_WIDTH,
  SCREEN_HEIGHT
);

// -----------------------------------------------------------------------------
// ฟังก์ชันสลับ byte ของข้อมูล RGB565
// ใช้กรณีข้อมูลภาพมีลำดับ byte ไม่ตรงกับจอหรือไลบรารี
// -----------------------------------------------------------------------------
uint16_t swap565(uint16_t c) {
  return (uint16_t)((c << 8) | (c >> 8));
}

// -----------------------------------------------------------------------------
// ฟังก์ชันแสดงข้อความสถานะบนจอ
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
// -----------------------------------------------------------------------------
void showImageInfo() {
  uint16_t firstPixel = dashboard_frame[0];
  uint16_t firstPixelShown = RGB565_SWAP_BYTES ? swap565(firstPixel) : firstPixel;

  Serial.println("========================================");
  Serial.println("RGB565 IMAGE INFORMATION");
  Serial.println("========================================");
  Serial.print("Width  : ");
  Serial.println(DASHBOARD_FRAME_WIDTH);
  Serial.print("Height : ");
  Serial.println(DASHBOARD_FRAME_HEIGHT);
  Serial.print("First pixel raw (HEX) : 0x");
  Serial.println(firstPixel, HEX);
  Serial.print("First pixel used(HEX) : 0x");
  Serial.println(firstPixelShown, HEX);
  Serial.print("Swap bytes mode       : ");
  Serial.println(RGB565_SWAP_BYTES ? "true" : "false");
  Serial.println("========================================");

  gfx->fillScreen(COLOR_BLACK);
  gfx->fillRect(0, 0, 100, 100, firstPixelShown);
  gfx->setTextSize(2);
  gfx->setTextColor(COLOR_WHITE, COLOR_BLACK);
  gfx->setCursor(16, 130);
  gfx->println("First Pixel Test");
  delay(1200);
}

// -----------------------------------------------------------------------------
// ฟังก์ชันวาดภาพทีละพิกเซล
// จุดประสงค์: ใช้แทน draw16bitRGBBitmap() เพื่อหลีกเลี่ยงกรณีภาพไม่ขึ้น
// -----------------------------------------------------------------------------
void renderImagePixelByPixel() {
  Serial.println("[STEP] Render image pixel by pixel...");

  gfx->fillScreen(COLOR_BLACK);

  uint32_t index = 0;

  for (int16_t y = 0; y < DASHBOARD_FRAME_HEIGHT; y++) {
    for (int16_t x = 0; x < DASHBOARD_FRAME_WIDTH; x++) {
      uint16_t color = dashboard_frame[index++];
      if (RGB565_SWAP_BYTES) {
        color = swap565(color);
      }
      gfx->drawPixel(x, y, color);
    }

    // แสดงความคืบหน้าใน Serial ทุก 40 บรรทัด
    if ((y % 40) == 0) {
      Serial.print("Rendered line: ");
      Serial.println(y);
    }
  }

  Serial.println("[STEP] Render complete");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  if (!gfx->begin()) {
    Serial.println("ERROR: gfx->begin() failed");
    while (1) {
      delay(1000);
    }
  }

  gfx->setRotation(DISPLAY_ROTATION);

  drawStatusText("Initializing display...", COLOR_YELLOW);
  delay(800);

  drawStatusText("Running color test...", COLOR_CYAN);
  delay(400);
  showColorTest();

  drawStatusText("Checking image data...", COLOR_GREEN);
  delay(400);
  showImageInfo();

  drawStatusText("Rendering image...", COLOR_WHITE);
  delay(400);
  renderImagePixelByPixel();

  Serial.println("System ready.");
}

void loop() {
  delay(1000);
}
