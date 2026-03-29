/*
  ======================================================================
  Program : dashboard_frame_viewer_V3.ino
  Board   : ESP32-S3 + JC3248W535
  Display : 3.5" LCD 320x480
  Library : WiFi, WiFiClient, WiFiClientSecure, HTTPClient,
            Arduino_GFX, JPEGDecoder
  Version : V3
  Date    : 2026-03-29 22:10 ICT

  สรุปการทำงานของโปรแกรม
  1) เริ่มต้นจอ LCD ของบอร์ด JC3248W535
  2) เปิดไฟ backlight ของจอ
  3) เชื่อมต่อ WiFi ตามค่า SSID และ PASSWORD ที่กำหนด
  4) ตรวจชนิด URL อัตโนมัติ
     - ถ้าเป็น http://  ใช้ WiFiClient
     - ถ้าเป็น https:// ใช้ WiFiClientSecure
  5) ดาวน์โหลดภาพ JPEG จาก URL
     https://raw.githubusercontent.com/wpoosommai/JC3248W535-Without-LVGL/main/extras/images/dashboard_frame.jpg
  6) ถอดรหัสภาพด้วย JPEGDecoder
  7) แสดงภาพเต็มจอ 320x480
     - ถ้าภาพเล็กกว่าจอ จะจัดกึ่งกลาง
     - ถ้าภาพใหญ่กว่าจอ จะ crop เฉพาะส่วนเกิน
  8) แสดงผลข้อความสถานะผ่าน Serial Monitor และหน้าจอ

  คุณสมบัติโปรแกรม
  1) คัดออกมาเฉพาะส่วนแสดงผลภาพจาก URL เท่านั้น
  2) รองรับทั้ง HTTP และ HTTPS
  3) รองรับการ redirect ของ HTTP
  4) ใช้หน่วยความจำ PSRAM ก่อน ถ้ามี
  5) มี retry ดาวน์โหลดภาพอัตโนมัติ 3 รอบ
  6) มี comment อธิบายทุกส่วนเพื่อใช้ประกอบการเรียนการสอน

  ข้อมูลนำเข้า (Input)
  1) WiFi SSID
  2) WiFi PASSWORD
  3) IMAGE_URL

  รูปแบบคำสั่งควบคุม
  1) โปรแกรมนี้ไม่มีคำสั่ง JSON ควบคุม
  2) เมื่อเปิดเครื่อง โปรแกรมจะเชื่อมต่อ WiFi และแสดงภาพอัตโนมัติ

  ตัวอย่าง JSON ควบคุม (บันทึกไว้เพื่อมาตรฐานงานพัฒนา)
  {
    "cmd": "none",
    "note": "This viewer has no JSON runtime control"
  }

  หมายเหตุ
  1) ถ้าภาพสีเพี้ยน ให้ลองเปลี่ยน JPEG_USE_SWAPPED_BYTES ระหว่าง false/true
  2) ถ้า RSSI ต่ำกว่า -85 dBm การโหลด HTTPS อาจไม่เสถียร
  3) เวอร์ชันนี้แก้ไขให้รองรับ GitHub Raw HTTPS โดยตรง
  4) เวอร์ชัน V3 แก้ปัญหาชนกับ macro สี COLOR_* ของไลบรารี
  ======================================================================
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Arduino_GFX_Library.h>
#include <JPEGDecoder.h>
#include <JC3248W535.h>
#include <Wire.h>

// ======================================================================
// การตั้งค่าพื้นฐานของจอ
// ======================================================================
#define GFX_BL               1
#define SCREEN_ROTATION      0
#define LCD_NATIVE_WIDTH     320
#define LCD_NATIVE_HEIGHT    480
#define IMAGE_AREA_X         0
#define IMAGE_AREA_Y         0
#define IMAGE_AREA_W         320
#define IMAGE_AREA_H         480

// ======================================================================
// ถ้าภาพสีเพี้ยนให้สลับค่า false/true เพื่อทดสอบลำดับ byte สี RGB565
// false = JpegDec.read()
// true  = JpegDec.readSwappedBytes()
// ======================================================================
#define JPEG_USE_SWAPPED_BYTES false

// ======================================================================
// ข้อมูลเครือข่าย
// ======================================================================
const char* ssid     = "SmartAgritronics";
const char* password = "99999999";

// ======================================================================
// URL ของภาพที่ต้องการแสดง
// รองรับทั้ง HTTP และ HTTPS
// ======================================================================
const char* IMAGE_URL = "https://raw.githubusercontent.com/wpoosommai/JC3248W535-Without-LVGL/main/extras/images/JapanGirl.jpg";

// ======================================================================
// ค่าควบคุมการเชื่อมต่อ
// ======================================================================
const uint32_t WIFI_TIMEOUT_MS       = 20000;
const uint16_t HTTP_CONNECT_TIMEOUT  = 10000;
const uint16_t HTTP_READ_TIMEOUT     = 20000;
const uint8_t  DOWNLOAD_RETRY_COUNT  = 3;

// ======================================================================
// สีมาตรฐาน
// ใช้ macro COLOR_* จากไลบรารี JC3248W535 โดยตรง
// ห้ามประกาศซ้ำ เพราะในไฟล์ JC3248W535_Colors.h มี #define ไว้อยู่แล้ว
// ======================================================================

// ======================================================================
// สร้างบัส QSPI ของจอ JC3248W535
// ======================================================================
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

// ======================================================================
// ตัวแปรควบคุมตำแหน่งและพื้นที่ clipping ของภาพ JPEG
// ======================================================================
int16_t jpgDrawX = 0;
int16_t jpgDrawY = 0;
int16_t jpgClipX1 = 0;
int16_t jpgClipY1 = 0;
int16_t jpgClipX2 = IMAGE_AREA_W - 1;
int16_t jpgClipY2 = IMAGE_AREA_H - 1;

// line buffer สำหรับวาดภาพทีละ 1 แถว
uint16_t g_scaledLineBuffer[IMAGE_AREA_W];

// ตัวแปร debug HTTP
String g_lastContentType = "";
int    g_lastHttpCode = -1;

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

// ======================================================================
// ตรวจสอบชนิด URL
// ======================================================================
bool isHttpsUrl(const char* url) {
  if (url == nullptr) return false;
  return (strncmp(url, "https://", 8) == 0);
}

// ======================================================================
// จองหน่วยความจำสำหรับเก็บภาพ โดยใช้ PSRAM ก่อนถ้ามี
// ======================================================================
uint8_t* allocateImageBuffer(size_t len) {
  uint8_t* buf = nullptr;

  if (psramFound()) {
    buf = (uint8_t*)ps_malloc(len);
  }
  if (buf == nullptr) {
    buf = (uint8_t*)malloc(len);
  }

  return buf;
}

// ======================================================================
// จองหน่วยความจำใหม่ขนาดใหญ่ขึ้น แล้วคัดลอกข้อมูลเดิม
// ======================================================================
uint8_t* reallocateImageBuffer(uint8_t* oldBuf, size_t newSize, size_t oldDataSize) {
  uint8_t* newBuf = allocateImageBuffer(newSize);
  if (!newBuf) return nullptr;

  if (oldBuf && oldDataSize > 0) {
    memcpy(newBuf, oldBuf, oldDataSize);
    free(oldBuf);
  }

  return newBuf;
}

// ======================================================================
// ตรวจว่า buffer เป็นข้อมูล JPEG หรือไม่
// ======================================================================
bool isJpegBuffer(const uint8_t* buf, size_t len) {
  if (buf == nullptr || len < 4) return false;
  return (buf[0] == 0xFF && buf[1] == 0xD8);
}

// ======================================================================
// เชื่อมต่อ WiFi แบบรอจนกว่าจะสำเร็จหรือครบเวลาที่กำหนด
// ======================================================================
bool connectWiFi(uint32_t timeoutMs = WIFI_TIMEOUT_MS) {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");
  uint32_t startMs = millis();

  while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < timeoutMs) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.print("IP Address : ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI       : ");
    Serial.println(WiFi.RSSI());
    return true;
  }

  Serial.println("WiFi connect failed");
  return false;
}

// ======================================================================
// ดาวน์โหลดข้อมูลจาก stream เข้า memory buffer
// ฟังก์ชันนี้ใช้ร่วมกันทั้ง HTTP และ HTTPS
// ======================================================================
bool readHttpStreamToMemory(HTTPClient& http, uint8_t** outBuf, size_t* outLen) {
  *outBuf = nullptr;
  *outLen = 0;

  WiFiClient* stream = http.getStreamPtr();
  int len = http.getSize();

  if (stream == nullptr) {
    Serial.println("HTTP stream is null");
    return false;
  }

  if (len > 0) {
    uint8_t* buf = allocateImageBuffer((size_t)len);
    if (!buf) {
      Serial.println("Memory allocation failed");
      return false;
    }

    size_t totalRead = 0;
    uint32_t lastDataMs = millis();

    while (http.connected() && totalRead < (size_t)len) {
      size_t avail = stream->available();

      if (avail > 0) {
        int n = stream->readBytes(buf + totalRead, min((size_t)avail, (size_t)(len - totalRead)));
        if (n > 0) {
          totalRead += (size_t)n;
          lastDataMs = millis();
        }
      } else {
        if ((millis() - lastDataMs) > HTTP_READ_TIMEOUT) {
          Serial.println("HTTP read timeout while receiving fixed-length body");
          break;
        }
        delay(1);
      }
    }

    if (totalRead == (size_t)len) {
      *outBuf = buf;
      *outLen = totalRead;
      return true;
    }

    Serial.print("Read incomplete. expected=");
    Serial.print(len);
    Serial.print(" actual=");
    Serial.println(totalRead);
    free(buf);
    return false;
  }

  size_t capacity = 16384;
  size_t totalRead = 0;
  uint8_t* buf = allocateImageBuffer(capacity);
  if (!buf) {
    Serial.println("Initial allocation failed");
    return false;
  }

  uint32_t lastDataMs = millis();

  while (http.connected() || stream->available()) {
    size_t avail = stream->available();

    if (avail > 0) {
      if (totalRead + avail > capacity) {
        size_t newCapacity = capacity * 2;
        while (newCapacity < totalRead + avail) newCapacity *= 2;

        uint8_t* newBuf = reallocateImageBuffer(buf, newCapacity, totalRead);
        if (!newBuf) {
          free(buf);
          Serial.println("Reallocation failed");
          return false;
        }

        buf = newBuf;
        capacity = newCapacity;
      }

      int n = stream->readBytes(buf + totalRead, avail);
      if (n > 0) {
        totalRead += (size_t)n;
        lastDataMs = millis();
      }
    } else {
      if ((millis() - lastDataMs) > HTTP_READ_TIMEOUT) {
        break;
      }
      delay(1);
    }
  }

  if (totalRead > 0) {
    *outBuf = buf;
    *outLen = totalRead;
    return true;
  }

  free(buf);
  return false;
}

// ======================================================================
// ดาวน์โหลดไฟล์ภาพจาก URL มาเก็บในหน่วยความจำ
// รองรับทั้ง HTTP และ HTTPS
// ======================================================================
bool downloadImageToMemory(const char* url, uint8_t** outBuf, size_t* outLen) {
  *outBuf = nullptr;
  *outLen = 0;
  g_lastHttpCode = -1;
  g_lastContentType = "";

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("HTTP skipped: WiFi not connected");
    return false;
  }

  bool httpsMode = isHttpsUrl(url);
  Serial.print("URL mode    = ");
  Serial.println(httpsMode ? "HTTPS" : "HTTP");

  HTTPClient http;
  bool okBegin = false;

  if (httpsMode) {
    static WiFiClientSecure secureClient;
    secureClient.stop();
    secureClient.setInsecure();
    secureClient.setTimeout(HTTP_READ_TIMEOUT / 1000);
    okBegin = http.begin(secureClient, url);
  } else {
    static WiFiClient plainClient;
    plainClient.stop();
    plainClient.setTimeout(HTTP_READ_TIMEOUT / 1000);
    okBegin = http.begin(plainClient, url);
  }

  if (!okBegin) {
    Serial.println("HTTP begin failed");
    return false;
  }

  http.setConnectTimeout(HTTP_CONNECT_TIMEOUT);
  http.setTimeout(HTTP_READ_TIMEOUT);
  http.useHTTP10(true);
  http.setReuse(false);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.addHeader("Accept", "image/jpeg,image/*,*/*");
  http.addHeader("User-Agent", "ESP32-JPEG-Viewer-V2");
  http.addHeader("Connection", "close");

  Serial.print("HTTP GET URL = ");
  Serial.println(url);

  int httpCode = http.GET();
  g_lastHttpCode = httpCode;

  Serial.print("HTTP code = ");
  Serial.println(httpCode);

  if (httpCode <= 0) {
    Serial.print("HTTP error = ");
    Serial.println(http.errorToString(httpCode));
    http.end();
    return false;
  }

  if (httpCode != HTTP_CODE_OK) {
    Serial.print("Unexpected HTTP code: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }

  g_lastContentType = http.header("Content-Type");
  Serial.print("Content-Type   = ");
  Serial.println(g_lastContentType);
  Serial.print("Content-Length = ");
  Serial.println(http.getSize());

  bool readOk = readHttpStreamToMemory(http, outBuf, outLen);
  http.end();

  if (!readOk) {
    Serial.println("Image stream read failed");
    return false;
  }

  Serial.print("Image bytes    = ");
  Serial.println(*outLen);
  return true;
}

// ======================================================================
// แสดงข้อมูล JPEG เพื่อ debug
// ======================================================================
void jpegInfo() {
  Serial.println(F("==============="));
  Serial.println(F("JPEG image info"));
  Serial.println(F("==============="));
  Serial.print(F("Width      : ")); Serial.println(JpegDec.width);
  Serial.print(F("Height     : ")); Serial.println(JpegDec.height);
  Serial.print(F("Components : ")); Serial.println(JpegDec.comps);
  Serial.print(F("MCU / row  : ")); Serial.println(JpegDec.MCUSPerRow);
  Serial.print(F("MCU / col  : ")); Serial.println(JpegDec.MCUSPerCol);
  Serial.print(F("Scan type  : ")); Serial.println(JpegDec.scanType);
  Serial.print(F("MCU width  : ")); Serial.println(JpegDec.MCUWidth);
  Serial.print(F("MCU height : ")); Serial.println(JpegDec.MCUHeight);
  Serial.println(F(""));
}

// ======================================================================
// วาด block ย่อยของ JPEG แบบ crop ไม่ให้เกินขอบจอ
// ======================================================================
void drawScaledClippedBlock(
  int32_t srcBlockX,
  int32_t srcBlockY,
  uint16_t srcBlockW,
  uint16_t srcBlockH,
  uint16_t *srcPixels
) {
  if (srcPixels == nullptr || srcBlockW == 0 || srcBlockH == 0) return;

  int32_t dstX0 = jpgDrawX + srcBlockX;
  int32_t dstY0 = jpgDrawY + srcBlockY;
  int32_t dstX1 = dstX0 + srcBlockW - 1;
  int32_t dstY1 = dstY0 + srcBlockH - 1;

  if (dstX0 > jpgClipX2 || dstY0 > jpgClipY2 || dstX1 < jpgClipX1 || dstY1 < jpgClipY1) {
    return;
  }

  int32_t clipX0 = max((int32_t)jpgClipX1, dstX0);
  int32_t clipY0 = max((int32_t)jpgClipY1, dstY0);
  int32_t clipX1 = min((int32_t)jpgClipX2, dstX1);
  int32_t clipY1 = min((int32_t)jpgClipY2, dstY1);

  for (int32_t dy = clipY0; dy <= clipY1; dy++) {
    int32_t srcY = dy - dstY0;
    if (srcY >= srcBlockH) srcY = srcBlockH - 1;

    uint16_t drawW = 0;

    for (int32_t dx = clipX0; dx <= clipX1; dx++) {
      int32_t srcX = dx - dstX0;
      if (srcX >= srcBlockW) srcX = srcBlockW - 1;

      g_scaledLineBuffer[drawW++] = srcPixels[srcY * srcBlockW + srcX];
    }

    if (drawW > 0) {
      gfx->draw16bitRGBBitmap(clipX0, dy, g_scaledLineBuffer, drawW, 1);
    }
  }
}

// ======================================================================
// อ่าน MCU จาก JPEGDecoder แล้ววาดทีละ block
// ======================================================================
bool renderJPEGFromDecoder() {
  uint16_t mcuW = JpegDec.MCUWidth;
  uint16_t mcuH = JpegDec.MCUHeight;
  uint32_t imgW = JpegDec.width;
  uint32_t imgH = JpegDec.height;

  if (mcuW == 0 || mcuH == 0 || imgW == 0 || imgH == 0) return false;

  uint16_t minW = (imgW % mcuW == 0) ? mcuW : (imgW % mcuW);
  uint16_t minH = (imgH % mcuH == 0) ? mcuH : (imgH % mcuH);

  while (JPEG_USE_SWAPPED_BYTES ? JpegDec.readSwappedBytes() : JpegDec.read()) {
    uint16_t *pImg = JpegDec.pImage;
    if (pImg == nullptr) return false;

    int32_t mcuX = JpegDec.MCUx * mcuW;
    int32_t mcuY = JpegDec.MCUy * mcuH;

    uint16_t blockW = ((mcuX + mcuW) <= (int32_t)imgW) ? mcuW : minW;
    uint16_t blockH = ((mcuY + mcuH) <= (int32_t)imgH) ? mcuH : minH;

    drawScaledClippedBlock(mcuX, mcuY, blockW, blockH, pImg);
  }

  return true;
}

// ======================================================================
// ดาวน์โหลดภาพ JPEG จาก URL และวาดเต็มจอ
// ======================================================================
bool showImageFromUrl(const char* url) {
  uint8_t* jpgBuf = nullptr;
  size_t jpgLen = 0;

  for (uint8_t attempt = 1; attempt <= DOWNLOAD_RETRY_COUNT; attempt++) {
    Serial.println();
    Serial.print("Download attempt ");
    Serial.print(attempt);
    Serial.print("/");
    Serial.println(DOWNLOAD_RETRY_COUNT);

    if (attempt == 1) {
      drawStatusScreen("Loading image...", "Attempt 1", url, COLOR_YELLOW, COLOR_CYAN, COLOR_WHITE);
    } else {
      char line2[24];
      snprintf(line2, sizeof(line2), "Attempt %u", attempt);
      drawStatusScreen("Retry loading...", line2, url, COLOR_YELLOW, COLOR_CYAN, COLOR_WHITE);
    }

    if (downloadImageToMemory(url, &jpgBuf, &jpgLen)) {
      break;
    }

    delay(1000);
  }

  if (jpgBuf == nullptr || jpgLen == 0) {
    Serial.println("Image download failed");
    return false;
  }

  if (!isJpegBuffer(jpgBuf, jpgLen)) {
    Serial.println("Downloaded data is not JPEG");
    free(jpgBuf);
    return false;
  }

  int rc = JpegDec.decodeArray(jpgBuf, jpgLen);
  if (rc == 0) {
    Serial.println("JPEG decode start failed");
    free(jpgBuf);
    return false;
  }

  jpegInfo();

  uint16_t jpgW = JpegDec.width;
  uint16_t jpgH = JpegDec.height;

  if (jpgW == 0 || jpgH == 0) {
    Serial.println("Invalid JPEG size");
    free(jpgBuf);
    return false;
  }

  // จัดตำแหน่งให้ภาพอยู่กึ่งกลางจอ
  jpgDrawX = IMAGE_AREA_X + ((int32_t)IMAGE_AREA_W - (int32_t)jpgW) / 2;
  jpgDrawY = IMAGE_AREA_Y + ((int32_t)IMAGE_AREA_H - (int32_t)jpgH) / 2;

  // กำหนดขอบเขตการวาดภาพไม่ให้เกินจอ
  jpgClipX1 = IMAGE_AREA_X;
  jpgClipY1 = IMAGE_AREA_Y;
  jpgClipX2 = IMAGE_AREA_X + IMAGE_AREA_W - 1;
  jpgClipY2 = IMAGE_AREA_Y + IMAGE_AREA_H - 1;

  // ล้างจอก่อนวาดภาพใหม่
  gfx->fillScreen(COLOR_BLACK);

  Serial.print("Draw at X=");
  Serial.print(jpgDrawX);
  Serial.print(" Y=");
  Serial.println(jpgDrawY);

  bool ok = renderJPEGFromDecoder();
  free(jpgBuf);

  if (ok) {
    gfx->flush();
    Serial.println("Image draw success");
  }

  return ok;
}

// ======================================================================
// setup : เริ่มต้นระบบเพียงครั้งเดียว
// ======================================================================
void setup() {
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

  if (!connectWiFi()) {
    drawStatusScreen("WiFi connect failed", "Check SSID / signal", "Move closer to AP", COLOR_RED, COLOR_YELLOW, COLOR_WHITE);
    return;
  }

  if (WiFi.RSSI() < -85) {
    Serial.println("Warning: WiFi signal is weak for HTTPS");
    drawStatusScreen("WiFi connected", "Signal is weak", "HTTPS may fail", COLOR_GREEN, COLOR_YELLOW, COLOR_WHITE);
    delay(1500);
  }

  if (!showImageFromUrl(IMAGE_URL)) {
    char codeLine[32];
    snprintf(codeLine, sizeof(codeLine), "HTTP code = %d", g_lastHttpCode);
    drawStatusScreen("Image load failed", codeLine, "See Serial Monitor", COLOR_RED, COLOR_YELLOW, COLOR_WHITE);
  }
}

// ======================================================================
// loop : เวอร์ชันย่อนี้ไม่ต้องทำงานวนซ้ำเพิ่มเติม
// ======================================================================
void loop() {
  delay(1000);
}
