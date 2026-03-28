/*
  ============================================================================
  Example   : TouchPointViewer
  Library   : JC3248W535
  Version   : V1.4.0
  Updated   : 2026-03-29 00:25 ICT
  Purpose   :
    - ตัวอย่างสำหรับทดสอบจอ LCD และระบบสัมผัสของบอร์ด ESP32-S3 + JC3248W535
    - เมื่อแตะหน้าจอ จะแสดงจุดตำแหน่งที่แตะ พร้อมแสดงค่าพิกัด X,Y บนหน้าจอ
    - ใช้สำหรับตรวจสอบการคาลิเบรต การกลับแกน และความแม่นยำของ touch panel

  Hardware :
    1) บอร์ด ESP32-S3 + จอ JC3248W535 ขนาด 320x480
    2) จอ LCD แบบ QSPI
    3) Touch controller I2C address 0x3B

  Input :
    1) การแตะหน้าจอจากผู้ใช้

  Output :
    1) แสดงกากบาทและวงกลม ณ จุดที่แตะล่าสุด
    2) แสดงพิกัด X,Y ล่าสุดบนหน้าจอ
    3) แสดงสถานะ Touch Found / Touch Not Found / Touch Released
    4) พิมพ์ข้อมูลพิกัดลง Serial Monitor

  หมายเหตุ :
    1) ตัวอย่างนี้ไม่ใช้ WiFi และ MQTT เพื่อให้ทดสอบ touch ได้ง่ายที่สุด
    2) ถ้าจุดที่แตะไม่ตรงตำแหน่งจริง ให้ปรับค่า TOUCH_RAW_* หรือ TOUCH_INVERT_*
    3) ถ้าทิศทางจอไม่ตรง ให้ปรับ SCREEN_ROTATION
  ============================================================================
*/

#include <Arduino.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>
#include <JC3248W535.h>

#define TOUCH_POINT_VIEWER_VERSION "V1.4.0"
#define TOUCH_POINT_VIEWER_UPDATED "2026-03-29 00:25 ICT"

#define SCREEN_ROTATION      0
#define LCD_NATIVE_WIDTH     320
#define LCD_NATIVE_HEIGHT    480
#define TOUCH_I2C_ADDR_EX    0x3B
#define TOUCH_I2C_CLOCK_EX   400000UL
#define AXS_MAX_TOUCH_NUMBER 1

#define TOUCH_RAW_X_MIN      0
#define TOUCH_RAW_X_MAX      319
#define TOUCH_RAW_Y_MIN      0
#define TOUCH_RAW_Y_MAX      479
#define TOUCH_INVERT_X       false
#define TOUCH_INVERT_Y       false

// ใช้ helper ของ library เพื่อให้การสร้าง bus / display / canvas เป็นมาตรฐานเดียวกัน
// และลดความเสี่ยงจากการเรียก begin() ซ้ำ
Arduino_DataBus *bus = JC3248W535::createBusQSPI();
Arduino_GFX *g = JC3248W535::createDisplay(bus, LCD_NATIVE_WIDTH, LCD_NATIVE_HEIGHT);
Arduino_Canvas *gfx = JC3248W535::createCanvas(g, LCD_NATIVE_WIDTH, LCD_NATIVE_HEIGHT, 0, 0, 0);

bool touchControllerDetected = false;
bool lastTouchState = false;
uint16_t lastX = LCD_NATIVE_WIDTH / 2;
uint16_t lastY = LCD_NATIVE_HEIGHT / 2;
unsigned long lastDrawMs = 0;

uint16_t mapTouchRange(uint16_t value, uint16_t inMin, uint16_t inMax, uint16_t screenMax) {
  if (value < inMin) value = inMin;
  if (value > inMax) value = inMax;

  long out = map((long)value, (long)inMin, (long)inMax, 0L, (long)screenMax - 1L);
  if (out < 0) out = 0;
  if (out >= screenMax) out = screenMax - 1;
  return (uint16_t)out;
}

bool probeI2CDevice(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

bool getTouchPointRaw(uint16_t &x, uint16_t &y) {
  uint8_t data[AXS_MAX_TOUCH_NUMBER * 6 + 2] = {0};

  const uint8_t read_cmd[11] = {
    0xB5, 0xAB, 0xA5, 0x5A, 0x00, 0x00,
    (uint8_t)((AXS_MAX_TOUCH_NUMBER * 6 + 2) >> 8),
    (uint8_t)((AXS_MAX_TOUCH_NUMBER * 6 + 2) & 0xFF),
    0x00, 0x00, 0x00
  };

  Wire.beginTransmission(TOUCH_I2C_ADDR_EX);
  Wire.write(read_cmd, 11);
  if (Wire.endTransmission() != 0) return false;

  if (Wire.requestFrom(TOUCH_I2C_ADDR_EX, (uint8_t)sizeof(data)) != sizeof(data)) return false;

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

bool getTouchPoint(uint16_t &x, uint16_t &y) {
  uint16_t tx, ty;

  if (!getTouchPointRaw(tx, ty)) return false;

  switch (SCREEN_ROTATION) {
    case 0:
      x = tx; y = ty;
      break;
    case 1:
      x = ty; y = (LCD_NATIVE_WIDTH - 1) - tx;
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
      x = tx; y = ty;
      break;
  }

  if (x >= gfx->width())  x = gfx->width() - 1;
  if (y >= gfx->height()) y = gfx->height() - 1;
  return true;
}

void drawCrosshair(uint16_t x, uint16_t y) {
  const int r = 10;
  gfx->drawCircle(x, y, r, RED);
  gfx->drawCircle(x, y, r + 1, YELLOW);
  gfx->drawFastHLine((x > 15) ? x - 15 : 0, y, 31, WHITE);
  gfx->drawFastVLine(x, (y > 15) ? y - 15 : 0, 31, WHITE);
  gfx->fillCircle(x, y, 3, CYAN);
}

void drawHeader() {
  gfx->fillRoundRect(8, 8, 304, 70, 10, DARKGREY);
  gfx->drawRoundRect(8, 8, 304, 70, 10, WHITE);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(2);
  gfx->setCursor(18, 22);
  gfx->print("Touch Point Viewer");

  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(18, 50);
  gfx->print("Tap screen to show point + XY coordinate");
}

void drawTouchInfo(uint16_t x, uint16_t y, bool touching) {
  gfx->fillRoundRect(8, 88, 304, 90, 10, BLACK);
  gfx->drawRoundRect(8, 88, 304, 90, 10, BLUE);

  gfx->setTextSize(2);
  gfx->setTextColor(GREEN);
  gfx->setCursor(20, 105);
  gfx->print("X: ");
  gfx->print(x);

  gfx->setCursor(170, 105);
  gfx->print("Y: ");
  gfx->print(y);

  gfx->setTextSize(1);
  gfx->setTextColor(WHITE);
  gfx->setCursor(20, 140);
  gfx->print("Touch IC : ");
  gfx->print(touchControllerDetected ? "Detected @0x3B" : "Not found");

  gfx->setCursor(20, 156);
  gfx->print("State    : ");
  if (!touchControllerDetected) {
    gfx->print("Touch unavailable");
  } else if (touching) {
    gfx->print("Touching");
  } else {
    gfx->print("Released");
  }
}

void drawGuideArea() {
  gfx->drawRect(0, 0, gfx->width(), gfx->height(), BLUE);
  gfx->drawLine(0, gfx->height() / 2, gfx->width() - 1, gfx->height() / 2, DARKGREY);
  gfx->drawLine(gfx->width() / 2, 0, gfx->width() / 2, gfx->height() - 1, DARKGREY);

  gfx->setTextColor(LIGHTGREY);
  gfx->setTextSize(1);
  gfx->setCursor(10, 200);
  gfx->print("(0,0)");
  gfx->setCursor(gfx->width() - 48, 200);
  gfx->print("(319,0)");
  gfx->setCursor(10, gfx->height() - 12);
  gfx->print("(0,479)");
  gfx->setCursor(gfx->width() - 60, gfx->height() - 12);
  gfx->print("(319,479)");
}

void redrawScreen(uint16_t x, uint16_t y, bool touching) {
  gfx->fillScreen(BLACK);
  drawGuideArea();
  drawHeader();
  drawTouchInfo(x, y, touching);
  drawCrosshair(x, y);
  gfx->flush();
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("========================================");
  Serial.println("JC3248W535 Touch Point Viewer");
  Serial.print("Version : "); Serial.println(TOUCH_POINT_VIEWER_VERSION);
  Serial.print("Updated : "); Serial.println(TOUCH_POINT_VIEWER_UPDATED);
  Serial.println("========================================");

  JC3248W535::beginBacklight();

  // สำคัญ: ใช้ begin() ที่ Canvas เพียงครั้งเดียวเท่านั้น
  // เพื่อหลีกเลี่ยงปัญหา "SPI bus already initialized"
  if (!JC3248W535::beginCanvasDisplay(gfx, SCREEN_ROTATION, BLACK)) {
    Serial.println("LCD / Canvas begin failed");
    while (1) delay(1000);
  }

  touchControllerDetected = JC3248W535::resetAndBeginTouchTwoWire(Wire);
  Serial.print("Touch controller : ");
  Serial.println(touchControllerDetected ? "Detected" : "Not found");

  redrawScreen(lastX, lastY, false);
}

void loop() {
  uint16_t x = 0, y = 0;
  bool touched = touchControllerDetected && getTouchPoint(x, y);

  if (touched) {
    lastX = x;
    lastY = y;
  }

  if (touched != lastTouchState || (touched && millis() - lastDrawMs > 40)) {
    redrawScreen(lastX, lastY, touched);
    lastDrawMs = millis();

    if (touched) {
      Serial.print("Touch X=");
      Serial.print(lastX);
      Serial.print(" Y=");
      Serial.println(lastY);
    }
  }

  lastTouchState = touched;
  delay(10);
}
