/*
  ============================================================================
  File      : JC3248W535_DisplayHelper.h
  Library   : JC3248W535
  Version   : V1.4.0
  Updated   : 2026-03-29 00:25 ICT
  Purpose   :
    - รวมฟังก์ชันช่วยสร้างและเริ่มต้นจอ LCD / Canvas / Touch สำหรับบอร์ด JC3248W535
    - ป้องกันการเรียก begin() ซ้ำของ QSPI display bus ซึ่งอาจทำให้ ESP32-S3 รีบูต

  Important :
    1) เมื่อใช้ Arduino_Canvas ให้เรียก begin() ที่ Canvas เพียงครั้งเดียว
    2) ห้ามเรียกทั้ง display->begin() และ canvas->begin() ซ้ำต่อกันกับออบเจ็กต์ชุดเดียวกัน
    3) ฟังก์ชัน beginCanvasDisplay() จะเป็นผู้เริ่มต้นจอหลักให้เรียบร้อยแล้ว
  ============================================================================
*/
#ifndef JC3248W535_DISPLAY_HELPER_H
#define JC3248W535_DISPLAY_HELPER_H

#include <Arduino.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>
#include "JC3248W535_BoardConfig.h"

namespace JC3248W535 {

inline Arduino_DataBus* createBusQSPI() {
  return new Arduino_ESP32QSPI(LCD_CS, LCD_SCK, LCD_D0, LCD_D1, LCD_D2, LCD_D3);
}

inline Arduino_GFX* createDisplay(Arduino_DataBus* bus,
                                  uint16_t width = LCD_WIDTH,
                                  uint16_t height = LCD_HEIGHT) {
  return new Arduino_AXS15231B(bus, GFX_NOT_DEFINED, 0, false, width, height);
}

inline Arduino_Canvas* createCanvas(Arduino_GFX* display,
                                    uint16_t width = LCD_WIDTH,
                                    uint16_t height = LCD_HEIGHT,
                                    int16_t x = 0,
                                    int16_t y = 0,
                                    int16_t rotation = 0) {
  return new Arduino_Canvas(width, height, display, x, y, rotation);
}

inline void beginBacklight() {
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
}

inline bool beginCanvasDisplay(Arduino_Canvas* canvas, uint8_t rotation = 0, uint16_t clearColor = 0x0000) {
  if (canvas == nullptr) {
    return false;
  }
  if (!canvas->begin()) {
    return false;
  }
  canvas->setRotation(rotation);
  canvas->fillScreen(clearColor);
  canvas->flush();
  return true;
}

inline bool resetAndBeginTouchTwoWire(TwoWire &wire = Wire) {
  wire.begin(TOUCH_SDA, TOUCH_SCL, TOUCH_I2C_HZ);
  pinMode(TOUCH_RST, OUTPUT);
  digitalWrite(TOUCH_RST, LOW);
  delay(20);
  digitalWrite(TOUCH_RST, HIGH);
  delay(120);

  wire.beginTransmission(TOUCH_I2C_ADDR);
  return (wire.endTransmission() == 0);
}

} // namespace JC3248W535

#endif
