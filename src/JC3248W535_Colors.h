/*
  ============================================================================
  File      : JC3248W535_Colors.h
  Library   : JC3248W535
  Version   : V1.3.0
  Updated   : 2026-03-28 23:59 ICT
  Purpose   :
    - กำหนดชุดสีมาตรฐานแบบ RGB565 สำหรับใช้งานกับ Arduino_GFX และตัวอย่างทั้งหมด
    - ลดปัญหา Compilation error เช่น 'RED' was not declared in this scope

  Notes     :
    - รองรับทั้งชื่อแบบ COLOR_* และ alias แบบสั้น เช่น RED, GREEN, BLUE
    - หากโปรเจกต์อื่นประกาศชื่อสีไว้แล้ว จะไม่เขียนทับด้วย #ifndef
  ============================================================================
*/
#ifndef JC3248W535_COLORS_H
#define JC3248W535_COLORS_H

#define COLOR_BLACK     0x0000
#define COLOR_NAVY      0x000F
#define COLOR_DARKGREEN 0x03E0
#define COLOR_DARKCYAN  0x03EF
#define COLOR_MAROON    0x7800
#define COLOR_PURPLE    0x780F
#define COLOR_OLIVE     0x7BE0
#define COLOR_LIGHTGREY 0xC618
#define COLOR_DARKGREY  0x7BEF
#define COLOR_BLUE      0x001F
#define COLOR_GREEN     0x07E0
#define COLOR_CYAN      0x07FF
#define COLOR_RED       0xF800
#define COLOR_MAGENTA   0xF81F
#define COLOR_YELLOW    0xFFE0
#define COLOR_WHITE     0xFFFF
#define COLOR_ORANGE    0xFD20
#define COLOR_PINK      0xF81F

#ifndef BLACK
#define BLACK COLOR_BLACK
#endif
#ifndef NAVY
#define NAVY COLOR_NAVY
#endif
#ifndef DARKGREEN
#define DARKGREEN COLOR_DARKGREEN
#endif
#ifndef DARKCYAN
#define DARKCYAN COLOR_DARKCYAN
#endif
#ifndef MAROON
#define MAROON COLOR_MAROON
#endif
#ifndef PURPLE
#define PURPLE COLOR_PURPLE
#endif
#ifndef OLIVE
#define OLIVE COLOR_OLIVE
#endif
#ifndef LIGHTGREY
#define LIGHTGREY COLOR_LIGHTGREY
#endif
#ifndef DARKGREY
#define DARKGREY COLOR_DARKGREY
#endif
#ifndef BLUE
#define BLUE COLOR_BLUE
#endif
#ifndef GREEN
#define GREEN COLOR_GREEN
#endif
#ifndef CYAN
#define CYAN COLOR_CYAN
#endif
#ifndef RED
#define RED COLOR_RED
#endif
#ifndef MAGENTA
#define MAGENTA COLOR_MAGENTA
#endif
#ifndef YELLOW
#define YELLOW COLOR_YELLOW
#endif
#ifndef WHITE
#define WHITE COLOR_WHITE
#endif
#ifndef ORANGE
#define ORANGE COLOR_ORANGE
#endif
#ifndef PINK
#define PINK COLOR_PINK
#endif

#endif // JC3248W535_COLORS_H
