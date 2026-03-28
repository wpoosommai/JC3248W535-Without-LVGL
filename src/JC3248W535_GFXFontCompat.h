/*
  ============================================================================
  File      : JC3248W535_GFXFontCompat.h
  Library   : JC3248W535
  Version   : V1.1.1
  Updated   : 2026-03-28 23:58 ICT
  Purpose   :
    - จัดเตรียมโครงสร้างข้อมูล GFXglyph และ GFXfont สำหรับไฟล์ฟอนต์
    - แก้ปัญหา compile error กรณีสเก็ตช์ include ฟอนต์โดยยังไม่ได้ include
      ไลบรารีกราฟิกที่ประกาศชนิดข้อมูลดังกล่าวไว้ก่อน

  Notes     :
    - หากมีการ include gfxfont.h หรือไลบรารีกราฟิกอื่นมาก่อน ไฟล์นี้จะไม่ประกาศซ้ำ
    - ใช้เป็น compatibility layer เพื่อให้ Arduino IDE library compile ได้ง่ายขึ้น
  ============================================================================
*/
#ifndef JC3248W535_GFXFONT_COMPAT_H
#define JC3248W535_GFXFONT_COMPAT_H

#include <Arduino.h>

#if defined(__has_include)
  #if __has_include(<gfxfont.h>)
    #include <gfxfont.h>
  #endif
#endif

#ifndef _GFXFONT_H_
#define _GFXFONT_H_

typedef struct {
  uint16_t bitmapOffset;  // Pointer into GFXfont->bitmap
  uint8_t  width;         // Bitmap dimensions in pixels
  uint8_t  height;        // Bitmap dimensions in pixels
  uint8_t  xAdvance;      // Distance to advance cursor (x axis)
  int8_t   xOffset;       // X dist from cursor pos to UL corner
  int8_t   yOffset;       // Y dist from cursor pos to UL corner
} GFXglyph;

typedef struct {
  uint8_t  *bitmap;       // Glyph bitmaps, concatenated
  GFXglyph *glyph;        // Glyph array
  uint16_t first;         // ASCII extents (first char)
  uint16_t last;          // ASCII extents (last char)
  uint8_t  yAdvance;      // Newline distance (y axis)
} GFXfont;

#endif // _GFXFONT_H_

#endif // JC3248W535_GFXFONT_COMPAT_H
