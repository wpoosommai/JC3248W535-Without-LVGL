/*
  ============================================================================
  Example   : BasicLibraryInfo
  Library   : JC3248W535
  Version   : V1.4.0
  Updated   : 2026-03-29 00:25 ICT
  Purpose   :
    - ทดสอบว่า Arduino IDE มองเห็น library JC3248W535 ถูกต้อง
    - แสดงข้อมูลพินหลักและทดสอบอ่านข้อมูลไอคอนจาก PROGMEM
  ============================================================================
*/

#include <JC3248W535.h>
#include <pgmspace.h>

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("JC3248W535 library test");
  Serial.print("LCD size      : ");
  Serial.print(JC3248W535::LCD_WIDTH);
  Serial.print("x");
  Serial.println(JC3248W535::LCD_HEIGHT);
  Serial.print("LCD CS pin    : "); Serial.println(JC3248W535::LCD_CS);
  Serial.print("Touch I2C addr: 0x"); Serial.println(JC3248W535::TOUCH_I2C_ADDR, HEX);
  Serial.print("Backlight pin : "); Serial.println(JC3248W535::GFX_BL);

  uint16_t fanFirst = pgm_read_word(&icon_fan_on[0]);
  Serial.print("icon_fan_on[0]: 0x");
  Serial.println(fanFirst, HEX);
}

void loop() {
}
