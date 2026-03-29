/*
  ================================================================================
  Program Name : dashboard_frame_viewer_V4.ino
  Version      : V1.1.0
  Date         : 2026-03-29
  Timezone     : Asia/Bangkok
  Author       : ChatGPT for Weerawat Poosommai
  Platform     : ESP32-S3 + JC3248W535EN LCD 320x480
  Display IC   : AXS15231B

  ------------------------------------------------------------------------------
  สรุปการทำงานของโปรแกรม
  ------------------------------------------------------------------------------
  โปรแกรมนี้ใช้สำหรับทดสอบการเชื่อมต่อ Wi-Fi และดาวน์โหลดภาพ JPEG จาก URL
  แบบ HTTP/HTTPS แล้วนำมาแสดงผลบนจอ LCD ขนาด 320x480 ของบอร์ด ESP32-S3
  โดยรองรับลิงก์ภาพจาก GitHub Raw URL หรือ URL ภาพบนเซิร์ฟเวอร์ทั่วไป

  ลำดับการทำงาน
  1. เริ่มต้น Serial และจอแสดงผล
  2. เปิดไฟ Backlight ของจอ
  3. แสดงข้อความเริ่มต้น
  4. เชื่อมต่อ Wi-Fi
  5. ดาวน์โหลดไฟล์ JPEG จาก URL ที่กำหนด
  6. ถอดรหัส JPEG ด้วย JPEGDecoder
  7. วาดภาพลงบนจอ LCD
  8. ค้างภาพไว้บนหน้าจอ

  ------------------------------------------------------------------------------
  คุณสมบัติโปรแกรม
  ------------------------------------------------------------------------------
  1. รองรับการเชื่อมต่อ Wi-Fi แบบ Station
  2. รองรับการดาวน์โหลดภาพผ่าน HTTP และ HTTPS
  3. รองรับการตาม Redirect ของ URL
  4. แสดงสถานะผ่าน Serial Monitor
  5. แสดงข้อความสถานะบนจอ LCD
  6. จัดตำแหน่งภาพให้อยู่กึ่งกลางกรณีภาพเล็กกว่าจอ
  7. เหมาะสำหรับใช้สอนเรื่อง Wi-Fi, HTTP Client, HTTPS Client, JPEG Decode และ LCD Graphics

  ------------------------------------------------------------------------------
  รูปแบบคำสั่งควบคุม / ข้อมูล Input
  ------------------------------------------------------------------------------
  โปรแกรมนี้เป็นตัวอย่างแบบกำหนดค่าคงที่ในโค้ด ยังไม่มีการรับ JSON จริงระหว่างรัน
  แต่กำหนดรูปแบบข้อมูลควบคุมไว้ในส่วนหัวเพื่อใช้พัฒนาต่อได้ง่าย

  ตัวแปรที่ต้องกำหนด
  - WIFI_SSID
  - WIFI_PASSWORD
  - RAW_IMAGE_URL

  ------------------------------------------------------------------------------
  รูปแบบ JSON ควบคุมที่รองรับในการพัฒนาต่อ
  ------------------------------------------------------------------------------
  ตัวอย่างแนวคิด JSON:
  {
    "cmd": "show_image",
    "url": "https://raw.githubusercontent.com/user/repo/main/image.jpg",
    "rotation": 0,
    "clear_screen": true
  }

  ความหมายคำสั่ง
  - cmd           : คำสั่งหลัก เช่น "show_image"
  - url           : URL ของภาพ JPEG
  - rotation      : มุมหมุนจอ 0..3
  - clear_screen  : true = ล้างจอก่อนวาดภาพ

  ------------------------------------------------------------------------------
  Libraries Required
  ------------------------------------------------------------------------------
  1. WiFi
  2. HTTPClient
  3. WiFiClientSecure
  4. Arduino_GFX_Library
  5. JPEGDecoder

  ------------------------------------------------------------------------------
  หมายเหตุสำคัญ
  ------------------------------------------------------------------------------
  1. ถ้าใช้ GitHub ต้องใช้ลิงก์ RAW โดยตรง
     ตัวอย่างที่ถูกต้อง:
     https://raw.githubusercontent.com/user/repo/branch/path/image.jpg

  2. ห้ามใช้ลิงก์ GitHub แบบหน้าเว็บ:
     https://github.com/user/repo/blob/main/image.jpg
     เพราะจะได้ HTML แทนไฟล์ภาพ

  3. สำหรับ HTTPS ในตัวอย่างนี้ใช้ client.setInsecure()
     เพื่อความสะดวกในการทดสอบ
     หากนำไปใช้งานจริงเชิงความปลอดภัย ควรตรวจสอบ certificate ให้ถูกต้อง

  ------------------------------------------------------------------------------
  Revision History
  ------------------------------------------------------------------------------
  V1.0.0 : เวอร์ชันเริ่มต้น แสดงภาพ JPEG จาก GitHub RAW URL
  V1.1.0 : เพิ่มรองรับ HTTPS ด้วย WiFiClientSecure และปรับคอมเมนต์เพื่อการเรียนการสอน
  ================================================================================
*/

#include <WiFi.h>                 // ไลบรารี Wi-Fi สำหรับ ESP32
#include <HTTPClient.h>           // ไลบรารี HTTP Client
#include <WiFiClientSecure.h>     // ไลบรารีสำหรับ HTTPS
#include <Arduino_GFX_Library.h>  // ไลบรารีกราฟิกสำหรับจอ
#include <JPEGDecoder.h>          // ไลบรารีถอดรหัส JPEG

// ========================= USER CONFIG =========================
// กำหนดชื่อเครือข่าย Wi-Fi
const char* WIFI_SSID     = "SmartAgritronics";

// กำหนดรหัสผ่าน Wi-Fi
const char* WIFI_PASSWORD = "99999999";

// กำหนด URL ของภาพ JPEG
// รองรับทั้ง http:// และ https://
// ตัวอย่างลิงก์ GitHub RAW:
const char* RAW_IMAGE_URL =
  "https://raw.githubusercontent.com/wpoosommai/JC3248W535-Without-LVGL/main/extras/images/dashboard_frame.jpg";
// ===============================================================

// ========================= COLOR CONFIG ========================
static const uint16_t BLACK   = 0x0000;
static const uint16_t BLUE    = 0x001F;
static const uint16_t RED     = 0xF800;
static const uint16_t GREEN   = 0x07E0;
static const uint16_t CYAN    = 0x07FF;
static const uint16_t MAGENTA = 0xF81F;
static const uint16_t YELLOW  = 0xFFE0;
static const uint16_t WHITE   = 0xFFFF;
// ===============================================================

// ======================== BACKLIGHT PIN =========================
#ifndef GFX_BL
#define GFX_BL 1
#endif
// ===============================================================

// ======================= DISPLAY CONFIG =========================
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  45 /* CS */,
  47 /* SCK */,
  21 /* D0 */,
  48 /* D1 */,
  40 /* D2 */,
  39 /* D3 */
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

// ======================= JPEG BUFFER ============================
uint8_t *jpgBuffer = nullptr;
size_t jpgSize = 0;
// ===============================================================

bool isHttpsUrl(const char *url)
{
  if (url == nullptr) return false;
  return (strncmp(url, "https://", 8) == 0);
}

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

void showStatusScreen(const char *line1, const char *line2, uint16_t color1, uint16_t color2)
{
  gfx->fillScreen(BLACK);
  drawCenteredText(line1, 210, color1, 2);
  if (line2 != nullptr && strlen(line2) > 0)
  {
    drawCenteredText(line2, 245, color2, 2);
  }
}

void connectWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  showStatusScreen("Connecting WiFi...", "", YELLOW, WHITE);

  Serial.println("========================================");
  Serial.println("WiFi connect start");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("Connecting");

  uint32_t startMs = millis();

  while (WiFi.status() != WL_CONNECTED && (millis() - startMs < 20000))
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("WiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());

    showStatusScreen("WiFi Connected", WiFi.localIP().toString().c_str(), GREEN, CYAN);
  }
  else
  {
    Serial.println("WiFi connection failed");
    showStatusScreen("WiFi Failed", "Check SSID/Password", RED, YELLOW);
  }
}

void freeJpegBuffer()
{
  if (jpgBuffer != nullptr)
  {
    free(jpgBuffer);
    jpgBuffer = nullptr;
    jpgSize = 0;
  }
}

bool downloadJpegToBuffer(const char *url)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi not connected");
    return false;
  }

  if (url == nullptr || strlen(url) == 0)
  {
    Serial.println("Invalid URL");
    return false;
  }

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  bool beginOk = false;

  Serial.println("========================================");
  Serial.print("HTTP GET URL: ");
  Serial.println(url);

  if (isHttpsUrl(url))
  {
    static WiFiClientSecure secureClient;
    secureClient.setInsecure();
    beginOk = http.begin(secureClient, url);
    Serial.println("Protocol: HTTPS");
  }
  else
  {
    beginOk = http.begin(url);
    Serial.println("Protocol: HTTP");
  }

  if (!beginOk)
  {
    Serial.println("HTTP begin failed");
    return false;
  }

  int httpCode = http.GET();
  Serial.print("HTTP code = ");
  Serial.println(httpCode);

  if (httpCode != HTTP_CODE_OK)
  {
    Serial.println("HTTP request failed");
    http.end();
    return false;
  }

  int len = http.getSize();
  Serial.print("Content-Length = ");
  Serial.println(len);

  if (len <= 0)
  {
    Serial.println("Invalid content length");
    http.end();
    return false;
  }

  WiFiClient *stream = http.getStreamPtr();

  freeJpegBuffer();

  jpgBuffer = (uint8_t *)malloc(len);
  if (jpgBuffer == nullptr)
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

    if (availableBytes > 0)
    {
      int remain = len - offset;
      int toRead = (availableBytes > (size_t)remain) ? remain : (int)availableBytes;

      int readLen = stream->readBytes(jpgBuffer + offset, toRead);
      if (readLen > 0)
      {
        offset += readLen;
        timeoutMs = millis();

        Serial.print("Downloaded: ");
        Serial.print(offset);
        Serial.print(" / ");
        Serial.println(len);
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
    freeJpegBuffer();
    return false;
  }

  jpgSize = offset;

  Serial.print("Downloaded bytes: ");
  Serial.println(jpgSize);

  return true;
}

void drawJpegFromBuffer()
{
  if (jpgBuffer == nullptr || jpgSize == 0)
  {
    Serial.println("No JPEG buffer");
    return;
  }

  JpegDec.decodeArray(jpgBuffer, jpgSize);

  int jpgW = JpegDec.width;
  int jpgH = JpegDec.height;
  int mcuW = JpegDec.MCUWidth;
  int mcuH = JpegDec.MCUHeight;

  Serial.println("========================================");
  Serial.print("JPEG Width  : ");
  Serial.println(jpgW);
  Serial.print("JPEG Height : ");
  Serial.println(jpgH);
  Serial.print("MCU Width   : ");
  Serial.println(mcuW);
  Serial.print("MCU Height  : ");
  Serial.println(mcuH);

  int drawX = 0;
  int drawY = 0;

  if (jpgW < gfx->width())
  {
    drawX = (gfx->width() - jpgW) / 2;
  }

  if (jpgH < gfx->height())
  {
    drawY = (gfx->height() - jpgH) / 2;
  }

  gfx->fillScreen(BLACK);

  while (JpegDec.read())
  {
    uint16_t *pImg = JpegDec.pImage;

    int mcuX = JpegDec.MCUx * mcuW + drawX;
    int mcuY = JpegDec.MCUy * mcuH + drawY;

    int winW = mcuW;
    int winH = mcuH;

    if (mcuX + winW > gfx->width())
    {
      winW = gfx->width() - mcuX;
    }

    if (mcuY + winH > gfx->height())
    {
      winH = gfx->height() - mcuY;
    }

    if (winW > 0 && winH > 0)
    {
      gfx->draw16bitRGBBitmap(mcuX, mcuY, pImg, winW, winH);
    }
  }

  Serial.println("JPEG draw complete");
}

void showErrorScreen(const char *msg)
{
  gfx->fillScreen(BLACK);
  drawCenteredText("Image Viewer Error", 180, WHITE, 2);
  drawCenteredText(msg, 230, RED, 2);
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println("GitHub / HTTP / HTTPS JPEG Viewer");
  Serial.println("Version: V1.1.0");
  Serial.println("Board  : ESP32-S3 + JC3248W535EN");
  Serial.println("========================================");

  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  gfx->begin();
  gfx->setRotation(0);
  gfx->fillScreen(BLACK);

  drawCenteredText("JPEG Viewer V1.1.0", 200, CYAN, 2);
  drawCenteredText("ESP32-S3 LCD 320x480", 235, WHITE, 2);
  delay(1200);

  connectWiFi();
  delay(1000);

  if (WiFi.status() != WL_CONNECTED)
  {
    showErrorScreen("WiFi Failed");
    return;
  }

  showStatusScreen("Downloading JPEG...", "", YELLOW, WHITE);

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
  delay(1000);
}
