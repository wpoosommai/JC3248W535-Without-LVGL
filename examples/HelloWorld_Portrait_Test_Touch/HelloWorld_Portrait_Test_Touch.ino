/*
====================================================================================================
Program Name : HelloWorld_Portrait_320x480_V7.ino
Version      : V8
Date         : 2026-03-29
Time         : 12:15 ICT
Platform     : ESP32-S3 + JC3248W535EN
Display      : 320x480 TFT LCD (Portrait)
Library      : Arduino_GFX_Library + JC3248W535-Without-LVGL + Wire

Program Summary
1) โปรแกรมนี้เป็นตัวอย่างแบบง่ายสำหรับทดสอบจอ 320x480 แนวตั้ง พร้อมระบบสัมผัส
2) ใช้โครงสร้างการอ่าน Touch และการแปลงพิกัด X,Y ชุดเดียวกับ IoTAGIAppV50.ino
3) แสดงข้อความ Hello World และแสดงพิกัด Touch X,Y บนหน้าจอแบบเรียลไทม์
4) แสดงสถานะ Touch Controller ผ่าน Serial Monitor เพื่อช่วยตรวจสอบการคาลิเบรต
5) เหมาะสำหรับใช้เป็นไฟล์ต้นแบบก่อนนำ logic touch ไปพัฒนาต่อ

Program Features
1) จอทำงานแนวตั้ง ขนาด 320x480
2) เปิด backlight ที่ GPIO 1
3) อ่านค่าจาก Touch Controller I2C ที่ address 0x3B
4) แปลงพิกัดตาม SCREEN_ROTATION ให้ตรงกับ IoTAGIAppV50
5) แสดงจุดสัมผัสวงกลมและค่าพิกัด X,Y บนจอ

Control Format
1) ไม่มีคำสั่งควบคุมภายนอก
2) โปรแกรมทำงานอัตโนมัติใน setup() และ loop()

Input Data
1) รับข้อมูลจากแผงสัมผัสแบบ touch panel
2) ไม่มี input จาก WiFi, MQTT, HTTP หรือ JSON

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
8) Touch SDA = GPIO 4
9) Touch SCL = GPIO 8
10) Touch RST = GPIO 12
11) Touch INT = GPIO 11

Change Log
V1 : รุ่นเริ่มต้น
V2 : เพิ่มสีและกรอบ
V3 : แก้ชื่อสีซ้ำกับ macro
V4 : เปลี่ยนไปใช้ Arduino_GFX ตามไลบรารีจริง
V5 : เขียนโปรแกรมใหม่แบบจบในไฟล์เดียว ใช้จอแนวตั้ง 320x480 และกรอบหนาด้วย fillRect()
V6 : แก้ compile error โดยตัด Canvas ที่อ้างตัวแปร g ก่อนประกาศ และแก้ PIN_LCD_BL เป็น GFX_BL
V7 : เพิ่มระบบ Touch เต็มชุด โดยใช้ mapping และ rotation logic เดียวกับ IoTAGIAppV50 เพื่อให้พิกัด X,Y ตรงกัน
V8 : ปรับการแสดงผลภาพให้กลับทิศทางถูกต้องสำหรับจอ Portrait โดยใช้ SCREEN_ROTATION = 2 และให้ Touch หมุนตามจออัตโนมัติ
====================================================================================================
*/

#include <Arduino.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>
#include <JC3248W535.h>

// -----------------------------------------------------------------------------------------------
// การตั้งค่าจอและ Touch ให้ตรงกับ IoTAGIAppV50
// -----------------------------------------------------------------------------------------------
#define SCREEN_ROTATION    0
#define LCD_NATIVE_WIDTH   320
#define LCD_NATIVE_HEIGHT  480
#define GFX_BL             1

#define TOUCH_ADDR         0x3B
#define TOUCH_SDA          4
#define TOUCH_SCL          8
#define TOUCH_RST_PIN      12
#define TOUCH_INT_PIN      11
#define TOUCH_I2C_CLOCK    400000
#define AXS_MAX_TOUCH_NUMBER 1

// -----------------------------------------------------------------------------------------------
// ค่าคาลิเบรต Touch ให้ตรงกับ IoTAGIAppV50
// หมายเหตุ: ถ้าพิกัดคลาด ให้ปรับ 4 ค่านี้ภายหลัง
// -----------------------------------------------------------------------------------------------
#define TOUCH_RAW_X_MIN    0
#define TOUCH_RAW_X_MAX    319
#define TOUCH_RAW_Y_MIN    0
#define TOUCH_RAW_Y_MAX    479
#define TOUCH_INVERT_X     false
#define TOUCH_INVERT_Y     false

static const char* TEXT_LINE1 = "Hello";
static const char* TEXT_LINE2 = "World";

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  45,   // CS
  47,   // SCK
  21,   // D0
  48,   // D1
  40,   // D2
  39    // D3
);

// ======================================================================
// สร้างออบเจ็กต์จอจริง
// ======================================================================
Arduino_GFX *g = new Arduino_AXS15231B(
  bus,
  GFX_NOT_DEFINED,
  0,
  false,
  LCD_NATIVE_WIDTH,
  LCD_NATIVE_HEIGHT
);

// ======================================================================
// ใช้ Canvas เพื่อลดการกระพริบระหว่างวาดภาพ
// ======================================================================
Arduino_Canvas *gfx = new Arduino_Canvas(
  LCD_NATIVE_WIDTH,
  LCD_NATIVE_HEIGHT,
  g,
  0,
  0,
  0
);


uint16_t g_touchX = 0;
uint16_t g_touchY = 0;
bool g_lastTouchState = false;
bool g_touchDetected = false;

void setBacklight(bool on)
{
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, on ? HIGH : LOW);
}

uint16_t mapTouchRange(uint16_t value, uint16_t inMin, uint16_t inMax, uint16_t screenMax)
{
  if (value < inMin) value = inMin;
  if (value > inMax) value = inMax;

  long out = map((long)value, (long)inMin, (long)inMax, 0L, (long)screenMax - 1L);
  if (out < 0) out = 0;
  if (out >= screenMax) out = screenMax - 1;
  return (uint16_t)out;
}

bool probeI2CDevice(uint8_t addr)
{
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

bool getTouchPointRaw(uint16_t &x, uint16_t &y)
{
  uint8_t data[AXS_MAX_TOUCH_NUMBER * 6 + 2] = {0};

  const uint8_t read_cmd[11] = {
    0xB5, 0xAB, 0xA5, 0x5A, 0x00, 0x00,
    (uint8_t)((AXS_MAX_TOUCH_NUMBER * 6 + 2) >> 8),
    (uint8_t)((AXS_MAX_TOUCH_NUMBER * 6 + 2) & 0xFF),
    0x00, 0x00, 0x00
  };

  Wire.beginTransmission(TOUCH_ADDR);
  Wire.write(read_cmd, 11);
  if (Wire.endTransmission() != 0) return false;

  if (Wire.requestFrom(TOUCH_ADDR, (uint8_t)sizeof(data)) != sizeof(data)) return false;

  for (int i = 0; i < (int)sizeof(data); i++) {
    data[i] = Wire.read();
  }

  if (data[1] == 0 || data[1] > AXS_MAX_TOUCH_NUMBER) return false;

  uint16_t rawX = ((data[2] & 0x0F) << 8) | data[3];
  uint16_t rawY = ((data[4] & 0x0F) << 8) | data[5];

  if (rawX > 4095 || rawY > 4095) return false;

  uint16_t px = mapTouchRange(rawX, TOUCH_RAW_X_MIN, TOUCH_RAW_X_MAX, LCD_NATIVE_WIDTH);
  uint16_t py = mapTouchRange(rawY, TOUCH_RAW_Y_MIN, TOUCH_RAW_Y_MAX, LCD_NATIVE_HEIGHT);

  if (TOUCH_INVERT_X) px = (LCD_NATIVE_WIDTH - 1) - px;
  if (TOUCH_INVERT_Y) py = (LCD_NATIVE_HEIGHT - 1) - py;

  x = px;
  y = py;
  return true;
}

bool getTouchPoint(uint16_t &x, uint16_t &y)
{
  uint16_t tx, ty;
  if (!getTouchPointRaw(tx, ty)) return false;

  switch (SCREEN_ROTATION) {
    case 0:
      x = tx;
      y = ty;
      break;
    case 1:
      x = ty;
      y = (LCD_NATIVE_WIDTH - 1) - tx;
      break;
    case 2:
      x = (LCD_NATIVE_WIDTH - 1) - tx;
      y = (LCD_NATIVE_HEIGHT - 1) - ty;
      break;
    case 3:
      x = (LCD_NATIVE_HEIGHT - 1) - ty;
      y = tx;
      break;
    default:
      x = tx;
      y = ty;
      break;
  }

  if (x >= gfx->width())  x = gfx->width() - 1;
  if (y >= gfx->height()) y = gfx->height() - 1;
  return true;
}

void drawThickBorder(int x, int y, int w, int h, int t, uint16_t color)
{
  int x2 = x + w - 1;
  int y2 = y + h - 1;

  gfx->fillRect(x, y, w, t, color);
  gfx->fillRect(x, y2 - t + 1, w, t, color);
  gfx->fillRect(x, y, t, h, color);
  gfx->fillRect(x2 - t + 1, y, t, h, color);
}

void drawBaseScreen()
{
  gfx->fillScreen(COLOR_BLACK);
  drawThickBorder(0, 0, 320, 480, 1, COLOR_YELLOW);

  gfx->setTextColor(COLOR_WHITE, COLOR_BLACK);
  gfx->setTextSize(3);
  gfx->setCursor(95, 80);
  gfx->print(TEXT_LINE1);
  gfx->setCursor(95, 120);
  gfx->print(TEXT_LINE2);

  gfx->setTextSize(2);
  gfx->setCursor(30, 180);
  gfx->print("Touch Test : 320X480");

  gfx->setCursor(30, 220);
  gfx->print("X:");
  gfx->setCursor(30, 255);
  gfx->print("Y:");

  gfx->setCursor(30, 320);
  gfx->print("Tap Screen");

  gfx->setCursor(30, 350);
  gfx->print("To");

  gfx->setCursor(30, 380);
  gfx->print("Show Point");


  gfx->flush();   // <<< เพิ่มบรรทัดนี้
}

void updateTouchInfo(bool touched, uint16_t x, uint16_t y)
{
  gfx->fillRect(70, 215, 170, 70, COLOR_BLACK);
  gfx->setTextColor(COLOR_CYAN, COLOR_BLACK);
  gfx->setTextSize(3);
  gfx->setCursor(70, 220);
  gfx->print(x);
  gfx->setCursor(70, 255);
  gfx->print(y);

  gfx->fillRect(30, 350, 260, 70, COLOR_BLACK);
  gfx->setTextSize(2);
  if (touched) {
    gfx->setTextColor(COLOR_GREEN, COLOR_BLACK);
    gfx->setCursor(30, 355);
    gfx->print("Touch = DETECTED");

    gfx->drawCircle(x, y, 10, COLOR_RED);
    gfx->drawCircle(x, y, 11, COLOR_RED);
    gfx->fillCircle(x, y, 3, COLOR_YELLOW);
  } else {
    gfx->setTextColor(COLOR_RED, COLOR_BLACK);
    gfx->setCursor(30, 355);
    gfx->print("Touch = RELEASED");
  }

  gfx->flush();   // <<< เพิ่มบรรทัดนี้
}


// ======================================================================
// ฟังก์ชันแสดงข้อความกึ่งกลางจอ
// ======================================================================
void drawCenteredText(const char* text, int16_t y, uint16_t color, uint8_t textSize) {
  int16_t x1, y1;
  uint16_t w, h;

  gfx->setTextSize(textSize);
  gfx->setTextColor(color);
  gfx->getTextBounds(text, 0, y, &x1, &y1, &w, &h);

  int16_t x = (gfx->width() - w) / 2;
  gfx->setCursor(x, y);
  gfx->print(text);
}
// ======================================================================
// แสดงหลายบรรทัดแบบย่อบนจอ สำหรับ debug หน้างาน
// ======================================================================
void drawStatusScreen(const char* line1, const char* line2, const char* line3, uint16_t color1, uint16_t color2, uint16_t color3) {
  gfx->fillScreen(COLOR_BLACK);
  if (line1) drawCenteredText(line1, 180, color1, 2);
  if (line2) drawCenteredText(line2, 220, color2, 2);
  if (line3) drawCenteredText(line3, 255, color3, 1);
  gfx->flush();
}

void setup()
{
  setCpuFrequencyMhz(240);

  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("Program start: dashboard_frame_viewer_V2");

  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed");
    while (1) delay(100);
  }

  if (GFX_BL != GFX_NOT_DEFINED) {
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, HIGH);
  }

  gfx->setRotation(SCREEN_ROTATION);

  drawStatusScreen("System Booting...", "Init display", "JC3248W535", COLOR_WHITE, COLOR_CYAN, COLOR_WHITE);


  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  Wire.setClock(TOUCH_I2C_CLOCK);
  pinMode(TOUCH_INT_PIN, INPUT_PULLUP);
  pinMode(TOUCH_RST_PIN, OUTPUT);

  digitalWrite(TOUCH_RST_PIN, LOW);
  delay(100);
  digitalWrite(TOUCH_RST_PIN, HIGH);
  delay(100);

  g_touchDetected = probeI2CDevice(TOUCH_ADDR);
  Serial.print("Touch controller @0x3B = ");
  Serial.println(g_touchDetected ? "DETECTED" : "NOT FOUND");

  gfx->fillScreen(COLOR_BLACK);
  drawStatusScreen("System Ready", "Touch Screen", "JC3248W535", COLOR_WHITE, COLOR_CYAN, COLOR_WHITE);

  drawBaseScreen();
}

void loop()
{
  bool touched = getTouchPoint(g_touchX, g_touchY);

  if (touched) {
    updateTouchInfo(true, g_touchX, g_touchY);

    if (!g_lastTouchState) {
      Serial.print("Touch X=");
      Serial.print(g_touchX);
      Serial.print(" Y=");
      Serial.println(g_touchY);
    }
  } else if (g_lastTouchState) {
    updateTouchInfo(false, g_touchX, g_touchY);
  }

  g_lastTouchState = touched;
  delay(20);
}
