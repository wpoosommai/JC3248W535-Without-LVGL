/*
====================================================================================================
Program Name : HelloWorld_Portrait_320x480_V6.ino
Version      : V6
Date         : 2026-03-29
Time         : 11:45 ICT
Platform     : ESP32-S3 + JC3248W535EN
Display      : 320x480 TFT LCD (Portrait)
Library      : Arduino_GFX_Library + JC3248W535-Without-LVGL

Program Summary
1) โปรแกรมนี้เป็นตัวอย่างแบบง่ายสำหรับทดสอบจอ 320x480 แนวตั้ง
2) ใช้เฉพาะการแสดงผลบนจอ ไม่ใช้ WiFi, MQTT, Touch, HTTP หรือ JPEG
3) แสดงพื้นหลังสีดำ วาดกรอบ และแสดงข้อความ Hello World
4) ตัดส่วน Canvas ออกเพื่อให้คอมไพล์ง่ายและเสถียรขึ้น
5) แก้ปัญหา compile error จากตัวแปร g และ PIN_LCD_BL

Program Features
1) จอทำงานแนวตั้ง ขนาด 320x480
2) เปิด backlight ที่ GPIO 1
3) ทดสอบสีแดง เขียว น้ำเงิน ก่อนเข้าหน้าหลัก
4) วาดกรอบหนาด้วย fillRect() 4 ด้าน
5) เหมาะสำหรับใช้เป็นโปรแกรมตั้งต้นเพื่อนำไปพัฒนาต่อ

Control Format
1) ไม่มีคำสั่งควบคุมภายนอก
2) โปรแกรมทำงานอัตโนมัติในฟังก์ชัน setup()

Input Data
1) ไม่มี input จากผู้ใช้
2) หากต้องการเปลี่ยนข้อความ ให้แก้ที่ตัวแปร TEXT_LINE1 และ TEXT_LINE2

Control JSON Example
1) ไม่มีการใช้งาน JSON ในโปรแกรมนี้
2) ตัวอย่าง:
   {}
   {"cmd":"none"}

Pin / Display Structure
1) QSPI CS  = 45
2) QSPI SCK = 47
3) QSPI D0  = 21
4) QSPI D1  = 48
5) QSPI D2  = 40
6) QSPI D3  = 39
7) Backlight = GPIO 1

Change Log
V1 : รุ่นเริ่มต้น
V2 : เพิ่มสีและกรอบ
V3 : แก้ชื่อสีซ้ำกับ macro
V4 : เปลี่ยนไปใช้ Arduino_GFX ตามไลบรารีจริง
V5 : เขียนโปรแกรมใหม่แบบจบในไฟล์เดียว ใช้จอแนวตั้ง 320x480 และกรอบหนาด้วย fillRect()
V6 : แก้ compile error โดยตัด Canvas ที่อ้างตัวแปร g ก่อนประกาศ และแก้ PIN_LCD_BL เป็น GFX_BL
====================================================================================================
*/

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <JC3248W535.h>

// -----------------------------------------------------------------------------------------------
// การตั้งค่าจอ
// -----------------------------------------------------------------------------------------------
#define LCD_NATIVE_WIDTH   320
#define LCD_NATIVE_HEIGHT  480
#define SCREEN_ROTATION    2
#define GFX_BL             1

// -----------------------------------------------------------------------------------------------
// ข้อความที่ต้องการแสดง
// -----------------------------------------------------------------------------------------------
static const char* TEXT_LINE1 = "Hello";
static const char* TEXT_LINE2 = "World";

// -----------------------------------------------------------------------------------------------
// สร้างบัส QSPI สำหรับจอรุ่นนี้
// -----------------------------------------------------------------------------------------------
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  45,   // CS
  47,   // SCK
  21,   // D0
  48,   // D1
  40,   // D2
  39    // D3
);

// -----------------------------------------------------------------------------------------------
// สร้างออบเจ็กต์จอ AXS15231B
// -----------------------------------------------------------------------------------------------
Arduino_GFX *gfx = new Arduino_AXS15231B(
  bus,
  GFX_NOT_DEFINED,
  0,
  false,
  LCD_NATIVE_WIDTH,
  LCD_NATIVE_HEIGHT
);

// -----------------------------------------------------------------------------------------------
// เปิด/ปิดแสงพื้นหลังจอ
// -----------------------------------------------------------------------------------------------
void setBacklight(bool on)
{
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, on ? HIGH : LOW);
}

// -----------------------------------------------------------------------------------------------
// วาดกรอบหนาด้วย fillRect() 4 ด้าน
// -----------------------------------------------------------------------------------------------
void drawThickBorder(int x, int y, int w, int h, int t, uint16_t color)
{
  int x2 = x + w - 1;
  int y2 = y + h - 1;

  gfx->fillRect(x, y, w, t, color);          // ด้านบน
  gfx->fillRect(x, y2 - t + 1, w, t, color); // ด้านล่าง
  gfx->fillRect(x, y, t, h, color);          // ด้านซ้าย
  gfx->fillRect(x2 - t + 1, y, t, h, color); // ด้านขวา
}

// -----------------------------------------------------------------------------------------------
// วาดหน้าจอ Hello World
// -----------------------------------------------------------------------------------------------
void drawHelloWorld()
{
  const int boxX = 40;
  const int boxY = 150;
  const int boxW = 240;
  const int boxH = 120;
  const int boxT = 4;

  gfx->fillScreen(COLOR_BLACK);
  drawThickBorder(boxX, boxY, boxW, boxH, boxT, COLOR_YELLOW);

  gfx->setTextColor(COLOR_WHITE, COLOR_BLACK);
  gfx->setTextSize(3);

  gfx->setCursor(95, 185);
  gfx->print(TEXT_LINE1);

  gfx->setCursor(95, 225);
  gfx->print(TEXT_LINE2);
}

// -----------------------------------------------------------------------------------------------
// setup()
// -----------------------------------------------------------------------------------------------
void setup()
{
  setBacklight(true);
  delay(200);

  if (!gfx->begin())
  {
    while (1)
    {
      delay(1000);
    }
  }

  gfx->setRotation(SCREEN_ROTATION);

  // ทดสอบสีพื้นฐานก่อน
  gfx->fillScreen(COLOR_RED);
  delay(400);
  gfx->fillScreen(COLOR_GREEN);
  delay(400);
  gfx->fillScreen(COLOR_BLUE);
  delay(400);
  // ล้างพื้นที่ตัวเลขให้กว้างขึ้น เพื่อรองรับตัวหนังสือขนาดใหญ่ขึ้น
  gfx->fillRect(34, 94, 82, 30, BLACK);     // Air Temp
  gfx->fillRect(116, 94, 82, 30, BLACK);    // Humidity
  gfx->fillRect(220, 94, 90, 30, BLACK);    // Soil Moisture

  // แสดงหน้าหลัก
  //drawHelloWorld();
}

// -----------------------------------------------------------------------------------------------
// loop()
// -----------------------------------------------------------------------------------------------
void loop()
{
  delay(1000);
}
