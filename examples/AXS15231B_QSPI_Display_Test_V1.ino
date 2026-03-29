/*
========================================================================
Program Name : AXS15231B_QSPI_Display_Test_V1.ino
Version      : V1
Date         : 2026-03-29 23:55 ICT
Platform     : Arduino C++
Board        : ESP32-S3 / JC3248W535 / LCD 320x480
Display IC   : AXS15231B
Library      : Arduino_GFX_Library

สรุปการทำงานของโปรแกรม
1) เริ่มต้นบัส QSPI ของจอ JC3248W535 ขนาด 320x480
2) เริ่มต้นไดรเวอร์จอ AXS15231B
3) เปิดไฟ Backlight
4) ทดสอบแสดงสีเต็มจอ 8 สี
5) ทดสอบวาดข้อความ เส้น กรอบ วงกลม
6) แสดงข้อมูลสรุปฮาร์ดแวร์บนหน้าจอ
7) วนทำงานเพื่อยืนยันว่าจอพร้อมใช้งาน

คุณสมบัติโปรแกรม
1) ใช้สำหรับทดสอบว่าจอแสดงผลทำงานถูกต้องหรือไม่
2) ใช้ตรวจสอบว่า pin QSPI ถูกต้องหรือไม่
3) ใช้ตรวจสอบว่าไดรเวอร์ AXS15231B ถูกต้องหรือไม่
4) ใช้เป็นโค้ดตั้งต้นสำหรับพัฒนาระบบต่อยอด
5) มี Comment อธิบายทุกส่วนเพื่อใช้ในการเรียนการสอน

Hardware Configuration
1) MCU                : ESP32-S3
2) CPU Frequency      : 240MHz
3) Flash Mode         : QIO 120MHz
4) Flash Size         : 16MB
5) PSRAM              : OPI PSRAM
6) Partition Scheme   : Huge APP (3MB No OTA / 1MB SPIFFS)
7) Upload Mode        : UART0 / Hardware CDC
8) USB Mode           : Hardware CDC and JTAG

Pin Configuration
1) LCD QSPI CS        : GPIO45
2) LCD QSPI SCK       : GPIO47
3) LCD QSPI D0        : GPIO21
4) LCD QSPI D1        : GPIO48
5) LCD QSPI D2        : GPIO40
6) LCD QSPI D3        : GPIO39
7) LCD Backlight      : GPIO1

รูปแบบคำสั่งควบคุม
1) โปรแกรมนี้เป็นโปรแกรมทดสอบจอ ไม่รับคำสั่งควบคุมจากภายนอก
2) ใช้การแสดงผลอัตโนมัติภายใน loop()

ข้อมูล Input
1) ไม่มีข้อมูล input จากผู้ใช้
2) ใช้ค่าคงที่ของระบบภายในโปรแกรม

ตัวอย่าง JSON สำหรับมาตรฐานเอกสารระบบ
{
  "cmd": "display_test",
  "target": "AXS15231B",
  "result": "show_color_and_graphic_test"
}

ข้อควรระวัง
1) ต้องติดตั้งไลบรารี Arduino_GFX_Library ก่อนคอมไพล์
2) หากไฟ Backlight ไม่ติด ให้ตรวจสอบขา GPIO1
3) หากจอขาว ให้ตรวจสอบ QSPI pin และไดรเวอร์ AXS15231B
4) หากข้อความไม่แสดง แต่อยู่ไฟติด แสดงว่าการ init จออาจไม่สมบูรณ์

========================================================================
*/

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// ---------------------------------------------------------------------
// กำหนดค่า pin ของจอ JC3248W535 320x480
// ---------------------------------------------------------------------
static const int PIN_LCD_CS   = 45;
static const int PIN_LCD_SCK  = 47;
static const int PIN_LCD_D0   = 21;
static const int PIN_LCD_D1   = 48;
static const int PIN_LCD_D2   = 40;
static const int PIN_LCD_D3   = 39;
static const int PIN_LCD_BL   = 1;

// ---------------------------------------------------------------------
// สีพื้นฐานแบบ RGB565
// ---------------------------------------------------------------------
static const uint16_t COLOR_BLACK   = 0x0000;
static const uint16_t COLOR_BLUE    = 0x001F;
static const uint16_t COLOR_RED     = 0xF800;
static const uint16_t COLOR_GREEN   = 0x07E0;
static const uint16_t COLOR_CYAN    = 0x07FF;
static const uint16_t COLOR_MAGENTA = 0xF81F;
static const uint16_t COLOR_YELLOW  = 0xFFE0;
static const uint16_t COLOR_WHITE   = 0xFFFF;
static const uint16_t COLOR_ORANGE  = 0xFD20;
static const uint16_t COLOR_GRAY    = 0x8410;

// ---------------------------------------------------------------------
// สร้าง QSPI bus ของ ESP32-S3 ไปยังจอ LCD
// ---------------------------------------------------------------------
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  PIN_LCD_CS,
  PIN_LCD_SCK,
  PIN_LCD_D0,
  PIN_LCD_D1,
  PIN_LCD_D2,
  PIN_LCD_D3
);

// ---------------------------------------------------------------------
// สร้างออบเจ็กต์จอ AXS15231B
// ---------------------------------------------------------------------
Arduino_GFX *gfx = new Arduino_AXS15231B(
  bus,
  GFX_NOT_DEFINED,   // ไม่มีขา reset ที่กำหนดใช้งานในตัวอย่างนี้
  0,                 // rotation เริ่มต้น = 0
  false,             // IPS
  320,               // width
  480                // height
);

// ---------------------------------------------------------------------
// เปิดไฟ Backlight ของจอ
// ---------------------------------------------------------------------
void setBacklight(bool on)
{
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, on ? HIGH : LOW);
}

// ---------------------------------------------------------------------
// แสดงข้อความกึ่งกลางแบบง่าย
// ---------------------------------------------------------------------
void drawCenteredText(const String &text, int y, uint16_t color, uint8_t textSize)
{
  gfx->setTextSize(textSize);

  int16_t x1, y1;
  uint16_t w, h;
  gfx->getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  int16_t x = (gfx->width() - w) / 2;
  if (x < 0) x = 0;

  gfx->setTextColor(color);
  gfx->setCursor(x, y);
  gfx->print(text);
}

// ---------------------------------------------------------------------
// ทดสอบแสดงสีเต็มจอทีละสี
// ---------------------------------------------------------------------
void testFullScreenColors()
{
  struct ColorItem
  {
    uint16_t color;
    const char *name;
  };

  ColorItem colors[] =
  {
    { COLOR_BLACK,   "BLACK"   },
    { COLOR_RED,     "RED"     },
    { COLOR_GREEN,   "GREEN"   },
    { COLOR_BLUE,    "BLUE"    },
    { COLOR_CYAN,    "CYAN"    },
    { COLOR_MAGENTA, "MAGENTA" },
    { COLOR_YELLOW,  "YELLOW"  },
    { COLOR_WHITE,   "WHITE"   }
  };

  for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); i++)
  {
    gfx->fillScreen(colors[i].color);

    uint16_t textColor = (colors[i].color == COLOR_WHITE || colors[i].color == COLOR_YELLOW) ? COLOR_BLACK : COLOR_WHITE;
    drawCenteredText("AXS15231B DISPLAY TEST", 210, textColor, 2);
    drawCenteredText(colors[i].name, 250, textColor, 3);

    delay(700);
  }
}

// ---------------------------------------------------------------------
// ทดสอบวาดกราฟิกพื้นฐาน
// ---------------------------------------------------------------------
void testGraphics()
{
  gfx->fillScreen(COLOR_BLACK);

  gfx->drawRect(0, 0, gfx->width(), gfx->height(), COLOR_WHITE);
  gfx->drawRect(2, 2, gfx->width() - 4, gfx->height() - 4, COLOR_CYAN);

  gfx->drawLine(0, 0, gfx->width() - 1, gfx->height() - 1, COLOR_RED);
  gfx->drawLine(gfx->width() - 1, 0, 0, gfx->height() - 1, COLOR_GREEN);

  gfx->fillRoundRect(20, 40, 120, 80, 12, COLOR_BLUE);
  gfx->drawRoundRect(20, 40, 120, 80, 12, COLOR_WHITE);

  gfx->fillCircle(240, 80, 40, COLOR_YELLOW);
  gfx->drawCircle(240, 80, 50, COLOR_WHITE);

  gfx->fillTriangle(60, 180, 20, 250, 100, 250, COLOR_ORANGE);
  gfx->drawTriangle(200, 180, 160, 250, 240, 250, COLOR_MAGENTA);

  gfx->setTextSize(2);
  gfx->setTextColor(COLOR_WHITE);
  gfx->setCursor(20, 310);
  gfx->println("JC3248W535 320x480");

  gfx->setTextColor(COLOR_CYAN);
  gfx->setCursor(20, 340);
  gfx->println("Driver : AXS15231B");

  gfx->setTextColor(COLOR_YELLOW);
  gfx->setCursor(20, 370);
  gfx->println("Bus    : ESP32 QSPI");

  gfx->setTextColor(COLOR_GREEN);
  gfx->setCursor(20, 400);
  gfx->println("Status : DISPLAY OK");

  delay(2500);
}

// ---------------------------------------------------------------------
// แสดงหน้าสรุปฮาร์ดแวร์
// ---------------------------------------------------------------------
void showHardwareSummary()
{
  gfx->fillScreen(COLOR_BLACK);

  drawCenteredText("SYSTEM READY", 30, COLOR_GREEN, 3);

  gfx->drawFastHLine(10, 50, 300, COLOR_WHITE);

  gfx->setTextSize(2);
  gfx->setTextColor(COLOR_WHITE);

  gfx->setCursor(12, 80);
  gfx->println("Board : ESP32-S3");

  gfx->setCursor(12, 110);
  gfx->println("LCD   : AXS15231B 320x480");

  gfx->setCursor(12, 140);
  gfx->println("Bus   : QSPI");

  gfx->setCursor(12, 170);
  gfx->println("CS=45  SCK=47");

  gfx->setCursor(12, 200);
  gfx->println("D0=21  D1=48");

  gfx->setCursor(12, 230);
  gfx->println("D2=40  D3=39");

  gfx->setCursor(12, 260);
  gfx->println("BL=1");

  gfx->setCursor(12, 320);
  gfx->setTextColor(COLOR_YELLOW);
  gfx->println("If you see this screen,");

  gfx->setCursor(12, 350);
  gfx->println("display init is successful.");

  gfx->setCursor(12, 410);
  gfx->setTextColor(COLOR_CYAN);
  gfx->println("By Weerawat.PH");

  delay(3000);
}

// ---------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println("AXS15231B QSPI DISPLAY TEST V1");
  Serial.println("Board : JC3248W535 / ESP32-S3");
  Serial.println("LCD   : 320x480");
  Serial.println("========================================");

  setBacklight(true);

  if (!gfx->begin())
  {
    Serial.println("ERROR: gfx->begin() failed");
    while (true)
    {
      delay(1000);
    }
  }

  gfx->setRotation(0);
  gfx->fillScreen(COLOR_BLACK);

  drawCenteredText("HELLO WORLD", 180, COLOR_YELLOW, 3);
  drawCenteredText("AXS15231B TEST", 225, COLOR_CYAN, 2);
  drawCenteredText("JC3248W535 320x480", 260, COLOR_WHITE, 2);

  delay(2000);

  testFullScreenColors();
  testGraphics();
  showHardwareSummary();
}

// ---------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------
void loop()
{
  static uint32_t lastBlink = 0;
  static bool flag = false;

  if (millis() - lastBlink >= 1000)
  {
    lastBlink = millis();
    flag = !flag;

    gfx->fillRect(250, 445, 60, 25, COLOR_BLACK);
    gfx->setTextSize(2);
    gfx->setCursor(250, 460);
    gfx->setTextColor(flag ? COLOR_GREEN : COLOR_RED);
    gfx->print(flag ? "RUN" : "OK ");

    Serial.println(flag ? "Heartbeat: RUN" : "Heartbeat: OK");
  }
}
