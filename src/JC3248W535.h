/*
  ============================================================================
  File      : JC3248W535.h
  Library   : JC3248W535
  Version   : V1.4.0
  Updated   : 2026-03-29 00:25 ICT
  Purpose   :
    - จุดรวม include สำหรับใช้งานบอร์ด ESP32-S3 + JC3248W535 ใน Arduino IDE
    - รวมค่าคงที่ของบอร์ด ฟอนต์ และไอคอน RGB565 ไว้ใน library เดียว

  Usage     :
    #include <JC3248W535.h>

  Included  :
    - JC3248W535_BoardConfig.h
    - JC3248W535_Colors.h
    - Roboto_Regular16pt7b.h
    - ShortBaby_Mg2w16pt7b.h
    - icons_rgb565_V1.h
  ============================================================================
*/
#ifndef JC3248W535_MAIN_H
#define JC3248W535_MAIN_H

#include <Arduino.h>
#include "JC3248W535_BoardConfig.h"
#include "JC3248W535_GFXFontCompat.h"
#include "JC3248W535_Colors.h"
#include "JC3248W535_DisplayHelper.h"
#include "Roboto_Regular16pt7b.h"
#include "ShortBaby_Mg2w16pt7b.h"
#include "icons_rgb565_V1.h"

#endif // JC3248W535_MAIN_H
