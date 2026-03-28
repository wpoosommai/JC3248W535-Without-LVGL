/*
  ============================================================================
  File      : JC3248W535_BoardConfig.h
  Library   : JC3248W535
  Version   : V1.4.0
  Updated   : 2026-03-29 00:25 ICT
  Purpose   :
    - รวมค่าคงที่ด้านฮาร์ดแวร์ของบอร์ด ESP32-S3 + JC3248W535
    - ใช้อ้างอิงในการพัฒนาโปรแกรม Arduino IDE ให้ตรงกับชุดฮาร์ดแวร์
  ============================================================================
*/
#ifndef JC3248W535_BOARD_CONFIG_H
#define JC3248W535_BOARD_CONFIG_H

#include <Arduino.h>

namespace JC3248W535 {
  static constexpr uint16_t LCD_WIDTH  = 320;
  static constexpr uint16_t LCD_HEIGHT = 480;

  // QSPI LCD
  static constexpr int LCD_CS  = 45;
  static constexpr int LCD_SCK = 47;
  static constexpr int LCD_D0  = 21;
  static constexpr int LCD_D1  = 48;
  static constexpr int LCD_D2  = 40;
  static constexpr int LCD_D3  = 39;

  // Touch controller
  static constexpr uint8_t TOUCH_I2C_ADDR = 0x3B;
  static constexpr int TOUCH_SDA = 4;
  static constexpr int TOUCH_SCL = 8;
  static constexpr int TOUCH_RST = 12;
  static constexpr int TOUCH_INT = 11;
  static constexpr uint32_t TOUCH_I2C_HZ = 400000UL;

  // Backlight
  static constexpr int GFX_BL = 1;

  // Standard icon size
  static constexpr uint16_t ICON_STD_W = 50;
  static constexpr uint16_t ICON_STD_H = 50;
}

#endif // JC3248W535_BOARD_CONFIG_H
