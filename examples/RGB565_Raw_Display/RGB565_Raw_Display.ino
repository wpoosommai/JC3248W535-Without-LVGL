/*
===============================================================================
โปรแกรม : RGB565_Raw_Display.ino
Version : V1.0.0
วันที่แก้ไข : 2026-03-28
เวลาแก้ไข : 00:00
ผู้พัฒนา : OpenAI ChatGPT

สรุปการทำงานของโปรแกรม
- โปรแกรมนี้ใช้สำหรับแสดงภาพ RGB565 แบบดิบจากไฟล์ header ชื่อ dashboard_frame.h
- ไม่ต้องถอดรหัส JPEG / PNG เพราะข้อมูลภาพอยู่ในรูปแบบ uint16_t RGB565 พร้อมใช้งาน
- โปรแกรมจะเริ่มต้นจอ TFT, เปิดไฟ backlight, ทดสอบสีพื้นฐาน และแสดงภาพเต็มจอ
- เหมาะสำหรับ splash screen, dashboard frame, และภาพพื้นหลังคงที่ในระบบ Embedded
- ออกแบบสำหรับบอร์ด JC3248W535EN (ESP32-S3) ที่ใช้ Arduino_GFX_Library

คุณสมบัติโปรแกรม
1) เปิดใช้งานจอ 320x480
2) เปิด backlight อัตโนมัติ
3) ทดสอบสีพื้นฐานเต็มจอ 8 สี
4) แสดงภาพ RGB565 จาก dashboard_frame.h เต็มจอ
5) แสดงสถานะผ่าน Serial Monitor
6) มี comment อธิบายเพื่อใช้ในการเรียนการสอน
7) มีส่วนหัวโปรแกรมครบถ้วนสำหรับการพัฒนาต่อในอนาคต

รูปแบบคำสั่งควบคุม
- โปรแกรมนี้ไม่มีการรับคำสั่ง JSON จากภายนอก
- ควบคุมการทำงานโดยแก้ไขค่าคงที่ในโค้ด
- สามารถปรับ rotation ของจอได้ที่ gfx->setRotation(...)

ข้อมูล input
1) ไฟล์ dashboard_frame.h ต้องอยู่ในโฟลเดอร์เดียวกับไฟล์ .ino
2) ภายใน dashboard_frame.h ต้องมีองค์ประกอบดังนี้
   - #define DASHBOARD_FRAME_WIDTH 320
   - #define DASHBOARD_FRAME_HEIGHT 480
   - static const uint16_t dashboard_frame[] PROGMEM
3) บอร์ดต้องติดตั้ง Arduino_GFX_Library แล้ว

ตัวอย่าง JSON อ้างอิงเพื่อการสอน (โปรแกรมนี้ยังไม่ได้ parse JSON)
{
  "cmd": "show_rgb565_header_image",
  "image_name": "dashboard_frame",
  "x": 0,
  "y": 0,
  "rotation": 0
}

หมายเหตุเพื่อการพัฒนา
- ถ้าภาพหมุนผิด ให้เปลี่ยนค่าที่ gfx->setRotation(0) เป็น 1, 2 หรือ 3
- ถ้าจอดำสนิท ให้ตรวจขา GFX_BL และไฟเลี้ยง backlight
- ถ้าคอมไพล์ไม่ผ่าน ให้ตรวจว่าไฟล์ dashboard_frame.h อยู่ถูกตำแหน่ง
===============================================================================
*/

#include <Arduino_GFX_Library.h>
#include "dashboard_frame_V1.h"

// ========================= กำหนดสี RGB565 =========================
static const uint16_t BLACK   = 0x0000;
static const uint16_t BLUE    = 0x001F;
static const uint16_t RED     = 0xF800;
static const uint16_t GREEN   = 0x07E0;
static const uint16_t CYAN    = 0x07FF;
static const uint16_t MAGENTA = 0xF81F;
static const uint16_t YELLOW  = 0xFFE0;
static const uint16_t WHITE   = 0xFFFF;

// ========================= กำหนดขาจอ =========================
#define GFX_BL 1

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
  320,
  480
);

void showColorTest() {
  gfx->fillScreen(BLACK);   delay(300);
  gfx->fillScreen(BLUE);    delay(300);
  gfx->fillScreen(RED);     delay(300);
  gfx->fillScreen(GREEN);   delay(300);
  gfx->fillScreen(CYAN);    delay(300);
  gfx->fillScreen(MAGENTA); delay(300);
  gfx->fillScreen(YELLOW);  delay(300);
  gfx->fillScreen(WHITE);   delay(300);
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

  gfx->setRotation(0);

  Serial.println("Display color test...");
  showColorTest();

  Serial.println("Show dashboard_frame image...");
gfx->draw16bitRGBBitmap(0, 0, dashboard_frame, DASHBOARD_FRAME_WIDTH, DASHBOARD_FRAME_HEIGHT);
  Serial.println("Display complete.");
}

void loop() {
  delay(1000);
}
