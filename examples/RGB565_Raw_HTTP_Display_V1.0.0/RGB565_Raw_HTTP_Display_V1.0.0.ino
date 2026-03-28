/*
===============================================================================
โปรแกรม : RGB565_Raw_HTTP_Display_V1.0.0.ino
Version : V1.0.0
วันที่แก้ไข : 2026-03-28
เวลาแก้ไข : 00:00
ผู้พัฒนา : OpenAI ChatGPT

สรุปการทำงานของโปรแกรม
- โปรแกรมนี้ใช้สำหรับแสดงภาพแบบ RGB565 Raw โดยไม่ต้องถอดรหัส JPEG/PNG
- โปรแกรมจะเชื่อมต่อ WiFi แล้วดาวน์โหลดไฟล์ภาพ .raw / .rgb565 จาก URL
- จากนั้นอ่านข้อมูลพิกเซลแบบ 16-bit RGB565 แล้วส่งลงจอ TFT โดยตรง
- เหมาะสำหรับภาพพื้นหลัง, splash screen, dashboard frame ที่ต้องการวาดเร็ว
- รองรับจอ 320x480 บนบอร์ด JC3248W535EN (ESP32-S3) ที่ใช้ Arduino_GFX_Library

คุณสมบัติโปรแกรม
1) แสดงภาพ RGB565 Raw เต็มจอ
2) ไม่ใช้ตัวถอดรหัส JPEG / PNG
3) อ่านข้อมูลเป็นบล็อกทีละหลายบรรทัด เพื่อลดการใช้ RAM
4) แสดงสถานะผ่าน Serial Monitor
5) รองรับ HTTPS ด้วย WiFiClientSecure แบบ setInsecure() สำหรับทดสอบ

ข้อกำหนดไฟล์ภาพ
1) ต้องเป็นภาพ RGB565 Raw 16-bit เท่านั้น
2) ขนาดภาพต้องตรงกับค่าที่กำหนดใน RAW_IMAGE_WIDTH และ RAW_IMAGE_HEIGHT
3) สำหรับจอ 320x480 ขนาดไฟล์ต้องเป็น 320 x 480 x 2 = 307200 bytes
4) รูปแบบข้อมูลในไฟล์:
   - 1 พิกเซล = 2 ไบต์
   - ลำดับสี = RGB565
   - ค่าแต่ละพิกเซลนิยมเก็บแบบ Big-Endian:
       High byte ก่อน, Low byte ตามหลัง
   - หากสีเพี้ยน ให้สลับลำดับ byte ในฟังก์ชัน readAndDisplayRGB565Raw()

รูปแบบคำสั่งควบคุม / ข้อมูล input
- โปรแกรมนี้ไม่มีคำสั่ง JSON ควบคุมการทำงานจากภายนอก
- ผู้ใช้กำหนดค่าผ่านตัวแปรคงที่ในโค้ด ดังนี้:
  1) WIFI_SSID
  2) WIFI_PASSWORD
  3) RAW_IMAGE_URL
  4) RAW_IMAGE_WIDTH
  5) RAW_IMAGE_HEIGHT

ตัวอย่างข้อมูล input ที่ใช้จริงในโปรแกรม
- WIFI_SSID      = "YOUR_WIFI_SSID"
- WIFI_PASSWORD  = "YOUR_WIFI_PASSWORD"
- RAW_IMAGE_URL  = "https://example.com/dashboard_frame_320x480.rgb565"
- RAW_IMAGE_WIDTH  = 320
- RAW_IMAGE_HEIGHT = 480

ตัวอย่าง JSON อ้างอิงเพื่อการสอน (โปรแกรมนี้ยังไม่ได้ parse JSON)
{
  "cmd": "display_raw_rgb565",
  "url": "https://example.com/dashboard_frame_320x480.rgb565",
  "width": 320,
  "height": 480
}

หมายเหตุเพื่อการพัฒนา
- หากต้องการใช้ไฟล์จาก GitHub ให้แปลงภาพเป็น .raw หรือ .rgb565 ก่อนอัปโหลด
- ถ้าจะใช้ภาพเดียวประจำระบบและไม่ต้องการดาวน์โหลดผ่านเน็ต สามารถแปลงเป็น
  const uint16_t image[] PROGMEM แล้วใช้ draw16bitRGBBitmap() ได้
- หากภาพไม่ขึ้น ให้ตรวจสอบ:
  1) WiFi
  2) HTTP code
  3) ขนาดไฟล์ต้องตรงกับ width x height x 2
  4) byte order ของ RGB565
===============================================================================
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Arduino_GFX_Library.h>

// กำหนดค่าสี RGB565
static const uint16_t BLACK   = 0x0000;
static const uint16_t BLUE    = 0x001F;
static const uint16_t RED     = 0xF800;
static const uint16_t GREEN   = 0x07E0;
static const uint16_t CYAN    = 0x07FF;
static const uint16_t MAGENTA = 0xF81F;
static const uint16_t YELLOW  = 0xFFE0;
static const uint16_t WHITE   = 0xFFFF;
// ========================= การตั้งค่า WiFi และ URL =========================
const char* WIFI_SSID     = "SmartAgritronics";
const char* WIFI_PASSWORD = "99999999";
// ใส่ URL ของไฟล์ .raw หรือ .rgb565 ที่เป็นภาพ 320x480 RGB565
const char* RAW_IMAGE_URL =
  "https://example.com/dashboard_frame_320x480.rgb565";

// ขนาดภาพดิบที่ต้องตรงกับไฟล์จริง
static const uint16_t RAW_IMAGE_WIDTH  = 320;
static const uint16_t RAW_IMAGE_HEIGHT = 480;

// จำนวนบรรทัดที่อ่านต่อรอบ ยิ่งมากยิ่งเร็ว แต่ใช้ RAM มากขึ้น
static const uint16_t LINES_PER_CHUNK = 20;

// ========================= กำหนดขาจอ JC3248W535EN =========================
#define GFX_BL 1

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  45, /* CS  */
  47, /* SCK */
  21, /* SDIO0 */
  48, /* SDIO1 */
  40, /* SDIO2 */
  39  /* SDIO3 */
);

Arduino_GFX *gfx = new Arduino_AXS15231B(
  bus,
  GFX_NOT_DEFINED /* RST */,
  0 /* rotation */,
  false /* IPS */,
  320 /* width */,
  480 /* height */
);

// ========================= ฟังก์ชันช่วยเหลือ =========================
void connectWiFi();
bool readAndDisplayRGB565Raw(const char* url, uint16_t imgW, uint16_t imgH);
void drawCenteredMessage(const char* msg, uint16_t color, uint8_t textSize);
void fillScreenColorBars();
void waitForWiFiReady();

// ========================= setup =========================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  if (!gfx->begin()) {
    Serial.println("ERROR: gfx->begin() failed");
    while (true) delay(1000);
  }

  gfx->setRotation(0);
  gfx->fillScreen(BLACK);

  drawCenteredMessage("RGB565 RAW DISPLAY", WHITE, 2);
  delay(1200);

  fillScreenColorBars();
  delay(1200);

  gfx->fillScreen(BLACK);
  drawCenteredMessage("Connecting WiFi...", YELLOW, 2);

  connectWiFi();
  waitForWiFiReady();

  gfx->fillScreen(BLACK);
  drawCenteredMessage("Loading RAW image...", CYAN, 2);

  bool ok = readAndDisplayRGB565Raw(RAW_IMAGE_URL, RAW_IMAGE_WIDTH, RAW_IMAGE_HEIGHT);

  if (ok) {
    Serial.println("RAW image displayed successfully.");
  } else {
    gfx->fillScreen(RED);
    drawCenteredMessage("RAW LOAD FAILED", WHITE, 2);
    Serial.println("RAW image display failed.");
  }
}

void loop() {
  delay(1000);
}

// ========================= ฟังก์ชันเชื่อมต่อ WiFi =========================
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting WiFi");
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000UL) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection timeout");
  }
}

void waitForWiFiReady() {
  if (WiFi.status() != WL_CONNECTED) {
    gfx->fillScreen(RED);
    drawCenteredMessage("WiFi FAILED", WHITE, 2);
    while (true) delay(1000);
  }
}

// ========================= ฟังก์ชันแสดงภาพ RGB565 Raw =========================
bool readAndDisplayRGB565Raw(const char* url, uint16_t imgW, uint16_t imgH) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  Serial.println("RAW URL:");
  Serial.println(url);

  if (!http.begin(client, url)) {
    Serial.println("http.begin() failed");
    return false;
  }

  int httpCode = http.GET();
  Serial.print("HTTP code = ");
  Serial.println(httpCode);

  if (httpCode != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  int contentLength = http.getSize();
  Serial.print("Content-Length = ");
  Serial.println(contentLength);

  const int expectedSize = imgW * imgH * 2;
  Serial.print("Expected Size  = ");
  Serial.println(expectedSize);

  if (contentLength > 0 && contentLength != expectedSize) {
    Serial.println("ERROR: file size does not match width x height x 2");
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  if (!stream) {
    Serial.println("ERROR: stream is null");
    http.end();
    return false;
  }

  static uint16_t lineBuffer[RAW_IMAGE_WIDTH * LINES_PER_CHUNK];
  uint8_t* rawBytes = (uint8_t*) lineBuffer;
  uint16_t y = 0;

  while (y < imgH) {
    uint16_t linesThisRound = LINES_PER_CHUNK;
    if (y + linesThisRound > imgH) {
      linesThisRound = imgH - y;
    }

    size_t bytesNeeded = imgW * linesThisRound * 2;
    size_t bytesRead = 0;

    while (bytesRead < bytesNeeded) {
      int availableBytes = stream->available();
      if (availableBytes > 0) {
        int n = stream->readBytes(rawBytes + bytesRead, bytesNeeded - bytesRead);
        if (n > 0) {
          bytesRead += n;
        }
      } else {
        delay(1);
      }
    }

    for (size_t i = 0; i < (bytesNeeded / 2); i++) {
      uint8_t hi = rawBytes[i * 2 + 0];
      uint8_t lo = rawBytes[i * 2 + 1];
      lineBuffer[i] = ((uint16_t)hi << 8) | lo;
    }

    // ถ้าสีเพี้ยน ให้ลองสลับเป็น:
    // lineBuffer[i] = ((uint16_t)lo << 8) | hi;

    gfx->draw16bitRGBBitmap(0, y, lineBuffer, imgW, linesThisRound);

    Serial.print("Draw lines: ");
    Serial.print(y);
    Serial.print(" - ");
    Serial.println(y + linesThisRound - 1);

    y += linesThisRound;
  }

  http.end();
  return true;
}

// ========================= ฟังก์ชันวาดข้อความกลางจอ =========================
void drawCenteredMessage(const char* msg, uint16_t color, uint8_t textSize) {
  gfx->fillScreen(BLACK);
  gfx->setTextSize(textSize);
  gfx->setTextColor(color);
  gfx->setCursor(20, 220);
  gfx->println(msg);
}

// ========================= ฟังก์ชันสีทดสอบเต็มจอ =========================
void fillScreenColorBars() {
  static const uint16_t colors[8] = {
    BLACK, BLUE, RED, GREEN, CYAN, MAGENTA, YELLOW, WHITE
  };

  for (int i = 0; i < 8; i++) {
    gfx->fillScreen(colors[i]);
    delay(250);
  }
}
