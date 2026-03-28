/*
  ============================================================================
  Program    : HelloWorld_DisplayTest_V1.5.0.ino
  Version    : V1.5.0
  Updated    : 2026-03-28 17:15 ICT
  Board      : JC3248W535EN / ESP32-S3 / LCD 3.5" 320x480
  Purpose    :
    1) ทดสอบการเริ่มต้นจอ LCD
    2) แสดงข้อความ "Hello, World!"
    3) แสดงกราฟิกพื้นฐาน เช่น เส้น สี่เหลี่ยม วงกลม สามเหลี่ยม และแถบสี
    4) ใช้เป็นโปรแกรมพื้นฐานสำหรับตรวจสอบว่าจอทำงานปกติ

  Main Features :
    - แสดงข้อความหัวข้อกลางจอ
    - แสดงกรอบสี่เหลี่ยมและเส้นทแยงมุม
    - แสดงวงกลมและสามเหลี่ยม
    - แสดง color bars สำหรับทดสอบสีของจอ
    - แสดงผลผ่าน Serial Monitor เพื่อช่วยตรวจสอบสถานะการเริ่มระบบ

  Control Format :
    - โปรแกรมนี้เป็นโปรแกรมทดสอบแบบ Standalone
    - ไม่มีการรับคำสั่งควบคุมจาก MQTT / HTTP / JSON

  Input Data :
    - ไม่มี input จากภายนอก
    - ใช้ค่าคงที่ภายในโปรแกรมเพื่อทดสอบการแสดงผล

  Example JSON Control :
    - ไม่มี

  Required Library :
    - Arduino_GFX_Library

  Hardware Mapping :
    - LCD CS   = 45
    - LCD SCK  = 47
    - LCD D0   = 21
    - LCD D1   = 48
    - LCD D2   = 40
    - LCD D3   = 39
    - LCD RST  = -1
    - LCD DC   = -1
    - BL PIN   = 1

  Notes :
    - หากจอไม่ขึ้น ให้ตรวจสอบการเลือกบอร์ด ESP32-S3 และการติดตั้งไลบรารี
    - หากสีเพี้ยนหรือภาพกลับด้าน ให้ตรวจสอบค่า setRotation()
  ============================================================================
*/

#include <Arduino_GFX_Library.h>

// =========================
// กำหนดค่าฮาร์ดแวร์ของบอร์ด
// =========================
#define GFX_BL 1
#define LCD_WIDTH  320
#define LCD_HEIGHT 480

// สร้าง QSPI bus สำหรับจอของบอร์ด JC3248W535EN
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  45 /* CS */, 47 /* SCK */, 21 /* D0 */, 48 /* D1 */, 40 /* D2 */, 39 /* D3 */
);

// สร้างอ็อบเจ็กต์จอภาพ AXS15231B ความละเอียด 320x480
Arduino_GFX *gfx = new Arduino_AXS15231B(
  bus,
  -1 /* RST */,   // บางบอร์ดไม่ใช้ขา RST แยก
  0  /* rotation */,
  false,
  LCD_WIDTH,
  LCD_HEIGHT
);

// --------------------------------------------------
// ฟังก์ชันแสดงข้อความตรงกึ่งกลางแนวนอนแบบง่าย
// --------------------------------------------------
void drawCenteredText(const char *text, int y, uint16_t color, uint8_t textSize) {
  int16_t x1, y1;
  uint16_t w, h;
  gfx->setTextSize(textSize);
  gfx->getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  int16_t x = (LCD_WIDTH - w) / 2;
  gfx->setCursor(x, y);
  gfx->setTextColor(color);
  gfx->print(text);
}

// --------------------------------------------------
// วาดแถบสีทดสอบ RGB และสีพื้นฐาน
// --------------------------------------------------
void drawColorBars() {
  const int barY = 380;
  const int barH = 60;
  const int barW = LCD_WIDTH / 6;

  gfx->fillRect(0 * barW, barY, barW, barH, RED);
  gfx->fillRect(1 * barW, barY, barW, barH, GREEN);
  gfx->fillRect(2 * barW, barY, barW, barH, BLUE);
  gfx->fillRect(3 * barW, barY, barW, barH, YELLOW);
  gfx->fillRect(4 * barW, barY, barW, barH, CYAN);
  gfx->fillRect(5 * barW, barY, LCD_WIDTH - (5 * barW), barH, MAGENTA);

  gfx->drawRect(0, barY, LCD_WIDTH, barH, WHITE);
}

// --------------------------------------------------
// วาดกราฟิกพื้นฐานเพื่อทดสอบจอ
// --------------------------------------------------
void drawTestGraphics() {
  // พื้นหลังดำ
  gfx->fillScreen(BLACK);

  // กรอบนอก
  gfx->drawRect(0, 0, LCD_WIDTH, LCD_HEIGHT, WHITE);

  // หัวข้อหลัก
  drawCenteredText("Hello, World!", 40, YELLOW, 3);
  drawCenteredText("LCD Graphic Test", 85, CYAN, 2);

  // เส้นทแยงมุมทดสอบการวาดเส้น
  gfx->drawLine(10, 120, 150, 260, RED);
  gfx->drawLine(150, 120, 10, 260, GREEN);

  // สี่เหลี่ยมทึบและกรอบ
  gfx->fillRoundRect(170, 120, 120, 70, 10, BLUE);
  gfx->drawRoundRect(170, 120, 120, 70, 10, WHITE);
  drawCenteredText("BOX", 160, WHITE, 2);

  // วงกลม
  gfx->fillCircle(80, 320, 40, MAGENTA);
  gfx->drawCircle(80, 320, 50, WHITE);

  // สามเหลี่ยม
  gfx->fillTriangle(170, 350, 240, 250, 300, 350, GREEN);
  gfx->drawTriangle(170, 350, 240, 250, 300, 350, WHITE);

  // เส้นแนวนอนและแนวตั้งสำหรับเช็ค geometry
  gfx->drawFastHLine(20, 220, 120, YELLOW);
  gfx->drawFastVLine(60, 180, 80, CYAN);

  // ข้อความสถานะล่างจอ
  drawCenteredText("Display OK / Color OK / Graphics OK", 360, WHITE, 1);

  // แถบสีทดสอบด้านล่าง
  drawColorBars();
}

void setup() {
  // เปิด Serial Monitor สำหรับดูสถานะเริ่มต้นระบบ
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("========================================");
  Serial.println("Hello World + Graphics LCD Test Start");
  Serial.println("Board: JC3248W535EN / ESP32-S3");
  Serial.println("========================================");

  // เปิดไฟ backlight ของจอ
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  // เริ่มต้นจอ
  if (!gfx->begin()) {
    Serial.println("ERROR: gfx->begin() failed");
    while (1) {
      delay(1000);
    }
  }

  // ตั้งการหมุนจอ
  // 0,1,2,3 ให้ทดลองปรับตามทิศทางจอจริง
  gfx->setRotation(0);

  Serial.println("LCD begin OK");
  Serial.println("Drawing test screen...");

  // วาดหน้าทดสอบจอ
  drawTestGraphics();

  Serial.println("Display test complete");
}

void loop() {
  // ไม่มีงานวนซ้ำ เพราะเป็นโปรแกรมทดสอบค้างหน้าจอ
}
