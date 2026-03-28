/*
  ============================================================================
  Program: GitHub_Image_Display_V1.0.0.ino
  Version: V1.0.0
  Date: 2026-03-28
  Time: Asia/Bangkok
  Purpose:
    โปรแกรมทดสอบแสดงภาพ JPEG จาก GitHub แบบ RAW URL บนจอ LCD 320x480
    สำหรับบอร์ด ESP32-S3 + จอ JC3248W535EN

  Program Features:
    1) เชื่อมต่อ Wi-Fi
    2) ดาวน์โหลดภาพ JPEG จาก GitHub RAW URL ผ่าน HTTP
    3) แสดงภาพเต็มจอ โดยใช้ JPEGDecoder
    4) แสดงสถานะผ่าน Serial Monitor

  Control / Input:
    - กำหนด WiFi SSID/PASSWORD
    - กำหนด RAW_IMAGE_URL เป็นลิงก์แบบ raw.githubusercontent.com

  Important Notes:
    1) ห้ามใช้ลิงก์ GitHub แบบหน้าเว็บทั่วไป เช่น
       https://github.com/user/repo/blob/main/picture.jpg
       เพราะ ESP32 จะอ่านหน้า HTML ไม่ใช่ไฟล์ภาพจริง

    2) ต้องใช้ลิงก์ RAW เช่น
       https://raw.githubusercontent.com/user/repo/main/picture.jpg

    3) รองรับไฟล์ JPEG เป็นหลัก

  Example RAW URL:
    https://raw.githubusercontent.com/<user>/<repo>/<branch>/<path_to_file>.jpg

  Libraries Required:
    - Arduino_GFX_Library
    - JPEGDecoder
    - WiFi
    - HTTPClient

  Hardware Summary:
    - Board : ESP32-S3
    - LCD   : 320x480
    - Driver: AXS15231B

  Revision History:
    - V1.0.0 : Initial version for GitHub RAW JPEG display test
  ============================================================================
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_GFX_Library.h>
#include <JPEGDecoder.h>

// ========================= USER CONFIG =========================
const char* WIFI_SSID     = "SmartAgritronics";
const char* WIFI_PASSWORD = "99999999";
 
// ใช้ลิงก์ RAW จาก GitHub เท่านั้น
const char* RAW_IMAGE_URL = "https://raw.githubusercontent.com/wpoosommai/JC3248W535-Without-LVGL/refs/heads/main/extras/images/dashboard_frame.jpg";
// ===============================================================

// กำหนดค่าสี RGB565
static const uint16_t BLACK   = 0x0000;
static const uint16_t BLUE    = 0x001F;
static const uint16_t RED     = 0xF800;
static const uint16_t GREEN   = 0x07E0;
static const uint16_t CYAN    = 0x07FF;
static const uint16_t MAGENTA = 0xF81F;
static const uint16_t YELLOW  = 0xFFE0;
static const uint16_t WHITE   = 0xFFFF;

// Backlight pin
#ifndef GFX_BL
#define GFX_BL 1
#endif

// ====================== DISPLAY CONFIG =========================
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  45 /* CS */, 47 /* SCK */, 21 /* D0 */, 48 /* D1 */, 40 /* D2 */, 39 /* D3 */
);

Arduino_GFX *gfx = new Arduino_AXS15231B(
  bus,
  GFX_NOT_DEFINED /* RST */,
  0 /* rotation */,
  false /* IPS */,
  320 /* width */,
  480 /* height */
);
// ===============================================================

// บัฟเฟอร์เก็บไฟล์ JPEG ที่ดาวน์โหลดมา
uint8_t *jpgBuffer = nullptr;
size_t jpgSize = 0;

void drawCenteredText(const char *msg, int y, uint16_t color, uint8_t textSize)
{
  int16_t x1, y1;
  uint16_t w, h;
  gfx->setTextSize(textSize);
  gfx->setTextColor(color, BLACK);
  gfx->getTextBounds((char*)msg, 0, y, &x1, &y1, &w, &h);
  int x = (gfx->width() - w) / 2;
  gfx->setCursor(x, y);
  gfx->print(msg);
}

void connectWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  gfx->fillScreen(BLACK);
  drawCenteredText("Connecting WiFi...", 220, YELLOW, 2);
  Serial.print("Connecting WiFi");

  uint32_t startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < 20000)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("WiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    gfx->fillScreen(BLACK);
    drawCenteredText("WiFi Connected", 210, GREEN, 2);
    drawCenteredText(WiFi.localIP().toString().c_str(), 245, CYAN, 2);
  }
  else
  {
    Serial.println("WiFi connection failed");
    gfx->fillScreen(BLACK);
    drawCenteredText("WiFi Failed", 220, RED, 3);
  }
}

bool downloadJpegToBuffer(const char *url)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi not connected");
    return false;
  }

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(url);

  Serial.print("HTTP GET: ");
  Serial.println(url);

  int httpCode = http.GET();
  Serial.print("HTTP code = ");
  Serial.println(httpCode);

  if (httpCode != HTTP_CODE_OK)
  {
    http.end();
    return false;
  }

  int len = http.getSize();
  WiFiClient *stream = http.getStreamPtr();

  if (len <= 0)
  {
    Serial.println("Invalid content length");
    http.end();
    return false;
  }

  if (jpgBuffer)
  {
    free(jpgBuffer);
    jpgBuffer = nullptr;
    jpgSize = 0;
  }

  jpgBuffer = (uint8_t *)malloc(len);
  if (!jpgBuffer)
  {
    Serial.println("Memory allocation failed");
    http.end();
    return false;
  }

  int offset = 0;
  uint32_t timeoutMs = millis();
  while (http.connected() && offset < len)
  {
    size_t availableBytes = stream->available();
    if (availableBytes)
    {
      int readLen = stream->readBytes(jpgBuffer + offset, availableBytes);
      if (readLen > 0)
      {
        offset += readLen;
        timeoutMs = millis();
      }
    }

    if (millis() - timeoutMs > 5000)
    {
      Serial.println("Download timeout");
      break;
    }
    delay(1);
  }

  http.end();

  if (offset != len)
  {
    Serial.println("Downloaded size mismatch");
    free(jpgBuffer);
    jpgBuffer = nullptr;
    jpgSize = 0;
    return false;
  }

  jpgSize = offset;
  Serial.print("Downloaded bytes: ");
  Serial.println(jpgSize);
  return true;
}

void drawJpegFromBuffer()
{
  if (!jpgBuffer || jpgSize == 0)
  {
    Serial.println("No JPEG buffer");
    return;
  }

  JpegDec.decodeArray(jpgBuffer, jpgSize);

  int jpgW = JpegDec.width;
  int jpgH = JpegDec.height;
  int mcuW = JpegDec.MCUWidth;
  int mcuH = JpegDec.MCUHeight;

  Serial.printf("JPEG: %dx%d\n", jpgW, jpgH);

  int drawX = 0;
  int drawY = 0;

  if (jpgW < gfx->width())  drawX = (gfx->width()  - jpgW) / 2;
  if (jpgH < gfx->height()) drawY = (gfx->height() - jpgH) / 2;

  gfx->fillScreen(BLACK);

  while (JpegDec.read())
  {
    uint16_t *pImg = JpegDec.pImage;
    int mcuX = JpegDec.MCUx * mcuW + drawX;
    int mcuY = JpegDec.MCUy * mcuH + drawY;

    int winW = mcuW;
    int winH = mcuH;

    if (mcuX + winW > gfx->width())  winW = gfx->width()  - mcuX;
    if (mcuY + winH > gfx->height()) winH = gfx->height() - mcuY;

    if (winW > 0 && winH > 0)
    {
      gfx->draw16bitRGBBitmap(mcuX, mcuY, pImg, winW, winH);
    }
  }
}

void showErrorScreen(const char *msg)
{
  gfx->fillScreen(BLACK);
  drawCenteredText("Display GitHub Image", 180, WHITE, 2);
  drawCenteredText(msg, 230, RED, 2);
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("=== GitHub RAW Image Display Test ===");

  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  gfx->begin();
  gfx->fillScreen(BLACK);
  gfx->setRotation(0);

  drawCenteredText("GitHub RAW Image Test", 200, CYAN, 2);
  delay(1000);

  connectWiFi();
  delay(1000);

  if (WiFi.status() != WL_CONNECTED)
  {
    showErrorScreen("WiFi Failed");
    return;
  }

  gfx->fillScreen(BLACK);
  drawCenteredText("Downloading JPEG...", 220, YELLOW, 2);

  if (!downloadJpegToBuffer(RAW_IMAGE_URL))
  {
    Serial.println("Download failed");
    showErrorScreen("Download Failed");
    return;
  }

  drawJpegFromBuffer();
}

void loop()
{
  // วนค้างภาพไว้ ไม่ต้องทำอะไรเพิ่ม
  delay(1000);
}
