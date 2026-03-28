/*
  ======================================================================
  Program : IoTAGIAppV50.ino
  Board   : ESP32-S3 + JC3248W535
  Display : 3.5" LCD 320x480
  Library : Arduino_GFX, JPEGDecoder, WiFi, HTTPClient, PubSubClient
  Version : V50
  Date    : 2026-03-28 23:45 ICT

  สรุปการทำงานของโปรแกรม
  1) เริ่มต้นจอ LCD, ระบบสัมผัส และแสดงหน้า Welcome Screen
  2) เมื่อแตะหน้าจอครั้งแรก จะเข้าสู่หน้า Dashboard ระบบควบคุมโรงเรือนปลูกผัก
  3) Dashboard แสดงค่าอุณหภูมิ ความชื้นอากาศ ความชื้นดิน แสง ระดับน้ำ pH และ EC
  4) Dashboard มีปุ่มควบคุม AUTO/MANUAL และสั่งงาน FAN, PUMP, MIST, LIGHT, VENT ได้จากหน้าจอ
  5) ทุกการสั่งงานจากหน้าจอจะส่งสถานะ JSON กลับทาง MQTT เพื่อให้ App ภายนอกรับทราบผล
  6) รับคำสั่ง JSON ผ่าน MQTT เพื่อควบคุมรีเลย์ โหมดทำงาน อัปเดตค่าจากเซนเซอร์ และสลับหน้าจอ
  7) ยังรองรับโหมดแสดงภาพ JPEG จาก URL เดิม เพื่อใช้เป็นหน้ากล้อง/แกลเลอรี/เมนูภาพประกอบได้
  8) ไอคอนคำสั่ง FAN/PUMP/MIST/LIGHT/GALLERY บน Dashboard ใช้ข้อมูลภาพสี RGB565 จากไฟล์ icons_rgb565_V1.h
  9) วาดไอคอน RGB565 จากข้อมูล PROGMEM โดยตรงโดยไม่ต้องโหลดจากอินเทอร์เน็ต
 10) หลังแสดงภาพหลักแล้ว จะโหลดภาพ MainMenu.jpg มาวาดทับบนภาพเดิมที่ตำแหน่ง (0,430)
 11) โหมด GALLERY รองรับ Slide Show อัตโนมัติ เมื่อไม่มีการแตะเปลี่ยนภาพตามเวลาที่กำหนด
 12) เพิ่มเอฟเฟกต์เปลี่ยนภาพแบบ Curtain/Wipe เพื่อให้การสลับภาพดูนุ่มนวลขึ้น
 13) โหมด Slide Show ใช้รายการ URL ภาพจาก API slideshow_list_api.php แทนการอ้างชื่อไฟล์ตรงในโค้ด
 14) ขณะเปลี่ยนภาพใน Slide Show จะแสดงข้อความ Loading image... บนหน้าจอ
 15) ภาพพื้นหลัง Dashboard (dashboardFrameFile) โหลดจาก URL ตรง http://192.168.1.201/IoTServer/dashboard_frame.jpg

  หมายเหตุสำคัญของเวอร์ชันนี้
  1) เปลี่ยนจาก TJpg_Decoder ไปใช้ JPEGDecoder
  2) JPEGDecoder ไม่มีฟังก์ชัน setJpgScale() แบบ TJpg_Decoder
  3) เวอร์ชันนี้แสดงภาพเต็มจอ ไม่มีกรอบและไม่มีแถบคำสั่งด้านล่าง
  4) ถ้าภาพเล็กกว่าจอจะจัดกึ่งกลาง ถ้าภาพใหญ่กว่าจะ crop เฉพาะส่วนเกิน
  5) โค้ดถูกใส่คำอธิบายเพื่อใช้ประกอบการเรียนการสอน
  6) ปรับ Welcome Screen ให้เป็น Dark Theme อ่านง่ายและลดความสว่างจ้า
  7) เพิ่มระบบรักษาเสถียรภาพ WiFi: ปิดโหมดประหยัดพลังงาน, เปิด Auto Reconnect,
     ตรวจจับหลุด และสั่งเชื่อมต่อใหม่แบบเป็นช่วงเวลาเพื่อลดอาการ WiFi หลุดบ่อย
  8) เพิ่มระบบ Slide Show อัตโนมัติในหน้า Gallery พร้อมเอฟเฟกต์ก่อนเปลี่ยนภาพ
  9) ใส่คำสั่งควบคุม JSON ไว้ในส่วนหัวโปรแกรม เพื่อใช้เป็นเอกสารอ้างอิง
 10) ปรับฟังก์ชันโหลดภาพ Overlay ให้ตั้งเวลา timeout ชัดเจนและพิมพ์ข้อความ error รายละเอียด
 11) ปรับการส่ง MQTT control state ให้พยายามเชื่อมต่อใหม่ก่อนส่ง และรายงานสาเหตุเมื่อส่งไม่สำเร็จ
 12) ปรับหน้า System Ready ให้แสดงรายละเอียด Hardware และสถานะระบบทั้งหมดแบบสรุปในหน้าเดียว

  รูปแบบคำสั่งควบคุม JSON และชื่อ TOPIC ที่ใช้
  1) Topic: IoTAGIApp/Command
     JSON : {"cmd":"home"}
  2) Topic: IoTAGIApp/Command
     JSON : {"cmd":"dashboard"}
  3) Topic: IoTAGIApp/Command
     JSON : {"cmd":"gallery"}
  4) Topic: IoTAGIApp/Command
     JSON : {"cmd":"next"}
  5) Topic: IoTAGIApp/Command
     JSON : {"cmd":"prev"}
  6) Topic: IoTAGIApp/Command
     JSON : {"cmd":"show","index":0}
  7) Topic: IoTAGIApp/Command
     JSON : {"cmd":"show","file":"TADSANEE8.jpg"}
  8) Topic: IoTAGIApp/Command
     JSON : {"cmd":"slideshow","state":"on","interval":10000}
  9) Topic: IoTAGIApp/Command
     JSON : {"cmd":"slideshow","state":"off"}
 10) Topic: IoTAGIApp/Command
     JSON : {"cmd":"refreshSlideshow"}
 11) Topic: IoTAGIApp/Command
     JSON : {"cmd":"fan","state":"on"}
 12) Topic: IoTAGIApp/Command
     JSON : {"cmd":"pump","state":"off"}
 13) Topic: IoTAGIApp/Command
     JSON : {"cmd":"mist","state":"on"}
 14) Topic: IoTAGIApp/Command
     JSON : {"cmd":"light","state":"off"}
 15) Topic: IoTAGIApp/Command
     JSON : {"cmd":"mode","state":"auto"}
 16) Topic: IoTAGIApp/Status
     JSON : {"temp":27.5,"hum":68.2,"soil":55.0}

  สรุป TOPIC MQTT ที่เกี่ยวข้อง
  1) Subscribe รับคำสั่งควบคุม : IoTAGIApp/Command
  2) Publish ส่งสถานะระบบ     : IoTAGIApp/Status
  3) Publish ส่งพิกัดสัมผัส    : IoTAGIApp/Touch
  4) Subscribe รับข้อมูลเกษตร  : AGRI/#
  ======================================================================
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <JPEGDecoder.h>
#include <math.h>
#include <JC3248W535.h>
#include <pgmspace.h>

// ======================================================================
// Embedded RGB565 icons ถูกเรียกใช้ผ่าน JC3248W535 library แล้ว
// ======================================================================

// ======================================================================
// การตั้งค่าการสลับ byte สีของ JPEG
// หมายเหตุ:
// 1) ถ้าภาพสีเพี้ยน สีสลับ หรือออกแนวม่วง/เขียวผิดธรรมชาติ
//    มักเกิดจากลำดับ byte ของค่าสี RGB565 ไม่ตรงกับฟังก์ชันวาดภาพ
// 2) ค่า false = ใช้ JpegDec.read()      (ไม่สลับ byte)
// 3) ค่า true  = ใช้ JpegDec.readSwappedBytes()
// 4) สำหรับจอ/ไลบรารีชุดนี้ แนะนำเริ่มจาก false ก่อน
// ======================================================================
#define JPEG_USE_SWAPPED_BYTES false

// ======================================================================
// ===== WiFi =====
// ======================================================================
const char* ssid     = "SmartAgritronics";
const char* password = "99999999";


// ======================================================================
// ===== MQTT =====
// หน้าที่:
// 1) ใช้ส่งค่าพิกัด Touch X,Y ไปยัง MQTT Broker
// 2) Broker : 202.29.231.205
// 3) Port   : 1883
// 4) Topic  : IoTAGIApp/Touch
// ======================================================================
const char* mqttServer = "202.29.231.205";
const uint16_t mqttPort = 1883;
const char* mqttTopicTouch   = "IoTAGIApp/Touch";
const char* mqttTopicCommand = "IoTAGIApp/Command";
const char* mqttTopicStatus  = "IoTAGIApp/Status";
const char* mqttTopicAgriAll = "AGRI/#";

WiFiClient mqttNetClient;
PubSubClient mqttClient(mqttNetClient);

// ======================================================================
// ===== WiFi Reliability Control =====
// หน้าที่:
// 1) ควบคุมช่วงเวลาการ reconnect WiFi ไม่ให้ยิงถี่เกินไป
// 2) ตรวจสอบว่าหลุดต่อเนื่องเกินเวลาที่กำหนดหรือไม่
// 3) ใช้ร่วมกับ loop() เพื่อฟื้นการเชื่อมต่ออัตโนมัติ
// ======================================================================
const unsigned long WIFI_CONNECT_TIMEOUT_MS    = 15000UL;
const unsigned long WIFI_RETRY_INTERVAL_MS     = 10000UL;
const unsigned long WIFI_DROP_RESTART_WAIT_MS  = 30000UL;
unsigned long g_lastWiFiConnectAttemptMs       = 0;
unsigned long g_wifiDisconnectedSinceMs        = 0;

// ======================================================================
// ส่วนประกาศฟังก์ชันล่วงหน้า (Forward Declarations)
// หน้าที่:
// 1) ประกาศฟังก์ชันที่ถูกเรียกใช้งานก่อนตำแหน่งที่นิยามจริง
// 2) ทำให้ callback ของ MQTT สามารถเรียกฟังก์ชันควบคุมภาพได้
// ======================================================================
void showCurrentImage();
void showStartHomeScreen();
void showDashboardScreen();
void drawDashboardScreen();
void drawWelcomeScreen();
void showNextImage();
void showPrevImage();
void markGalleryInteraction();
void updateGallerySlideshow();
void playGalleryTransitionEffect();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void configureWiFiReliability();
bool ensureWiFiConnected(bool forceRetry = false);
void maintainNetwork();
void publishSystemStatus(const char* eventName, const char* detail);
void publishControlState(const char* source);
bool handleMqttJsonCommand(const String &json);
bool drawJpegFromUrlAt(const char* url, int16_t posX, int16_t posY);
bool drawRemoteIconByFile(const char* fileName, int16_t x, int16_t y);
bool drawRgb888ImageAt(const uint8_t* imageData, size_t imageLen, uint16_t imageW, uint16_t imageH, int16_t posX, int16_t posY);
bool drawDashboardIconFromArray(const uint16_t* imageData, uint16_t imageW, uint16_t imageH, int16_t x, int16_t y);
void drawDashboardIconsFromUrl();
void refreshDashboardInteractiveArea();
bool probeI2CDevice(uint8_t addr);
String formatBoolOnOff(bool value);
String formatBoolYesNo(bool value);
String getTouchStatusText();
String getRssiText();
String getMacText();
String getMqttStatusText();
void drawSystemReadyLine(int16_t x, int16_t y, const String &label, const String &value, uint16_t valueColor);
void requestDashboardRefresh();
bool drawDashboardFrameFromUrl();
void drawDashboardOverlayValues();
void drawDashboardIconsFromUrl();

// ======================================================================
// ตัวแปรฐาน URL ของโฟลเดอร์ภาพ
// หน้าที่:
// 1) กำหนด path หลักของรูปภาพไว้จุดเดียว
// 2) ผู้ใช้สามารถเปลี่ยน IP หรือชื่อโฟลเดอร์ได้จากบรรทัดนี้โดยตรง
// 3) ต้องลงท้ายด้วย / เพื่อให้ต่อชื่อไฟล์ได้ถูกต้อง
// ตัวอย่าง:
//    strcpy(baseUrl, "http://192.168.1.240/IoTServer/");
// ======================================================================
//char baseUrl[128] = "http://agriculture.3bbddns.com:37473/IoTServer/";
char baseUrl[128] = "http://192.168.1.200/IoTServer/";
// ======================================================================
// รายการภาพสำรองแบบกำหนดตรงในโปรแกรม
// หมายเหตุ:
// 1) ใช้เป็น fallback เมื่อ API ของ Slide Show โหลดไม่สำเร็จ
// 2) เก็บเฉพาะชื่อไฟล์ แล้วนำ baseUrl มาต่อภายหลัง
// ======================================================================
const char* fallbackImageFiles[] = {
  "TADSANEE1.jpg",
  "TADSANEE2.jpg",
  "TADSANEE3.jpg",
  "TADSANEE4.jpg",
  "TADSANEE5.jpg",
  "TADSANEE6.jpg",
  "TADSANEE7.jpg",
  "TADSANEE8.jpg",
  "TADSANEE9.jpg",
  "TADSANEE10.jpg",
  "TADSANEE11.jpg",
  "TADSANEE12.jpg",
  "TADSANEE13.jpg",
  "TADSANEE14.jpg",
  "musicabusulu.jpg",
  "musicabusulu1.jpg",
  "musicabusulu2.jpg",
  "musicabusulu3.jpg",
  "TADSANEE480X320.jpg"
};

const uint8_t FALLBACK_IMAGE_COUNT = sizeof(fallbackImageFiles) / sizeof(fallbackImageFiles[0]);

// ======================================================================
// URL ของ API สำหรับดึงรายการภาพ Slide Show
// หน้าที่:
// 1) ให้โปรแกรมอ่านรายชื่อภาพและ URL จาก JSON ที่ Server ส่งกลับ
// 2) ลดการแก้รายชื่อภาพใน source code ทุกครั้งที่มีการเพิ่ม/ลบรูป
// 3) หาก API ล้มเหลว โปรแกรมจะย้อนกลับไปใช้ fallbackImageFiles
// ======================================================================
char slideshowApiUrl[160] = "http://192.168.1.201/IoTServer/slideshow_list_api.php";
const uint8_t MAX_SLIDESHOW_IMAGES = 40;
String slideshowImageNames[MAX_SLIDESHOW_IMAGES];
String slideshowImageUrls[MAX_SLIDESHOW_IMAGES];
uint8_t slideshowImageCount = 0;
bool slideshowListLoaded = false;
unsigned long g_lastSlideshowFetchMs = 0;
const unsigned long SLIDESHOW_LIST_REFRESH_MS = 60000UL;

// ======================================================================
// ชื่อไฟล์ Overlay ที่ต้องการวาดทับหลังจากแสดงภาพหลัก
// หมายเหตุ:
// 1) เวอร์ชันนี้ใช้การคำนวณตำแหน่งกึ่งกลางจออัตโนมัติ
// 2) ค่าพิกัด OVERLAY_MENU_X และ OVERLAY_MENU_Y ถูกเก็บไว้เพื่อใช้อ้างอิงเท่านั้น
// ======================================================================
const char* overlayMenuFile = "MainMenu.jpg";
#define OVERLAY_MENU_X 0
#define OVERLAY_MENU_Y 430

// ======================================================================
// ชื่อไฟล์ไอคอน Dashboard แบบเดิมจาก URL
// หน้าที่:
// 1) เก็บชื่อไฟล์เดิมไว้เป็นข้อมูลอ้างอิงของโปรเจกต์
// 2) เวอร์ชันนี้ไม่ได้ใช้โหลดไอคอนคำสั่งจากอินเทอร์เน็ตแล้ว
// 3) ไอคอนคำสั่งจะวาดจาก array image_data_... ที่ฝังในโปรแกรมแทน
// 4) หากต้องการกลับไปใช้ Web Server ภายหลัง ยังมีชื่อไฟล์เดิมให้ใช้อ้างอิง
// ======================================================================
const char* iconFanOnFile     = "fan_on.jpg";
const char* iconFanOffFile    = "fan_off.jpg";
const char* iconPumpOnFile    = "pump_on.jpg";
const char* iconPumpOffFile   = "pump_off.jpg";
const char* iconMistOnFile    = "mist_on.jpg";
const char* iconMistOffFile   = "mist_off.jpg";
const char* iconLightOnFile   = "light_on.jpg";
const char* iconLightOffFile  = "light_off.jpg";
const char* iconGalleryFile   = "gallery.jpg";
const char* dashboardFrameFile = "http://192.168.1.201/IoTServer/dashboard_frame.jpg";

// พิกัดวางไอคอนบนหน้า Dashboard แบบเฟรมล่าสุด
// 1) พื้นที่ CONTROL & MENU : มุมบนซ้าย (20,350) ถึง มุมล่างขวา (300,391)
// 2) ใช้แถวเดียว 5 ปุ่ม: FAN / PUMP / MIST / LIGHT / GALLERY
// 3) แบ่งพื้นที่กว้างประมาณ 281 px เป็น 5 ช่อง เท่ากับช่องละประมาณ 56 px
#define DASH_ICON_Y1 360
#define DASH_ICON_H  50
#define DASH_ICON_W  50
#define DASH_ICON_X1 10
#define DASH_ICON_X2 75
#define DASH_ICON_X3 140
#define DASH_ICON_X4 205
#define DASH_ICON_X5 260

// ======================================================================
// พิกัดโซนสัมผัสหน้า Dashboard
// หมายเหตุ:
// 1) แยกโซน AUTO/MANUAL ออกจากแถวไอคอนล่างอย่างชัดเจน
// 2) ขยายพื้นที่สัมผัสให้กว้างขึ้นเพื่อกดได้ง่ายขึ้นบนจอจริง
// 3) ปรับตามเลย์เอาต์ Dashboard Dark Theme รุ่นล่าสุด
// ======================================================================
#define DASH_MODE_TOUCH_Y1   320
#define DASH_MODE_TOUCH_Y2   364
#define DASH_AUTO_TOUCH_X1    70
#define DASH_AUTO_TOUCH_X2   160
#define DASH_MANUAL_TOUCH_X1 160
#define DASH_MANUAL_TOUCH_X2 252

#define DASH_ICON_TOUCH_Y1   350
#define DASH_ICON_TOUCH_Y2   430
#define DASH_FAN_TOUCH_X1     20
#define DASH_FAN_TOUCH_X2     78
#define DASH_PUMP_TOUCH_X1    79
#define DASH_PUMP_TOUCH_X2   132
#define DASH_MIST_TOUCH_X1   133
#define DASH_MIST_TOUCH_X2   186
#define DASH_LIGHT_TOUCH_X1  187
#define DASH_LIGHT_TOUCH_X2  248
#define DASH_GALLERY_TOUCH_X1 249
#define DASH_GALLERY_TOUCH_X2 319

// ======================================================================
// ค่าสี RGB565
// ======================================================================
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define YELLOW  0xFFE0
#define MAGENTA 0xF81F
#define CYAN    0x07FF
#define GRAY    0x8410

// ======================================================================
// กำหนดค่าจอ
// ======================================================================
#define GFX_BL 1
#define GFX_USE_PSRAM true
#define CANVAS

// ======================================================================
// กำหนดค่า Touch Controller
// ======================================================================
#define TOUCH_ADDR             0x3B
#define TOUCH_SDA              4
#define TOUCH_SCL              8
#define TOUCH_RST_PIN          12
#define TOUCH_INT_PIN          11
#define TOUCH_I2C_CLOCK        400000
#define AXS_MAX_TOUCH_NUMBER   1

// ======================================================================
// หมายเหตุสำคัญเรื่องสีภาพเพี้ยนใน IoTAGIAppV3 เดิม
// 1) เวอร์ชันเดิมนำ storage ไปใช้ GPIO11 และ GPIO12
// 2) แต่บอร์ดนี้ใช้ GPIO11 = TOUCH_INT และ GPIO12 = TOUCH_RST อยู่แล้ว
// 3) การใช้ขาซ้ำทำให้สัญญาณชนกันและทำให้ระบบแสดงผลทำงานผิดปกติ
// 4) เวอร์ชันแก้ไขนี้ปิด storage ชั่วคราวเพื่อให้การแสดงผล JPEG กลับมาถูกต้องเหมือน IoTAGIApp.ino
// ======================================================================
/*
สรุปการชนกัน
มีการชนกัน 2 จุดชัดเจน
IO11 storage ใช้เป็น MOSI
Touch ใช้เป็น INT
IO12 storage ใช้เป็น CLK
Touch ใช้เป็น RST
ดังนั้น storage กับ Touch ใช้งานพร้อมกันบนขาเดิมไม่ได้
*/
// ======================================================================
// กำหนดการหมุนจอ
// ======================================================================
#define SCREEN_ROTATION 0

// ======================================================================
// ขนาดจอจริงก่อนหมุน
// ======================================================================
#define LCD_NATIVE_WIDTH   320
#define LCD_NATIVE_HEIGHT  480

// ======================================================================
// ค่าคาลิเบรต Touch
// หมายเหตุ:
// ค่า raw ของ touch controller ควรเป็นช่วง ADC จริงของแผงสัมผัส
// ถ้าจอแตะคลาดเคลื่อน ให้คาลิเบรตใหม่ภายหลัง
// ======================================================================
#define TOUCH_RAW_X_MIN  0
#define TOUCH_RAW_X_MAX  319
#define TOUCH_RAW_Y_MIN  0
#define TOUCH_RAW_Y_MAX  479

// ======================================================================
// กลับแกน Touch กรณีติดตั้งจริงกลับด้าน
// ======================================================================
#define TOUCH_INVERT_X   false
#define TOUCH_INVERT_Y   false

// ======================================================================
// พื้นที่แสดงภาพแบบเต็มจอ
// หมายเหตุ:
// 1) ใช้จอทั้ง 320x480 สำหรับแสดงภาพ
// 2) ไม่มีกรอบและไม่มีแถบคำสั่งด้านล่าง
// 3) แตะขอบซ้ายของจอเพื่อภาพก่อนหน้า
// 4) แตะขอบขวาของจอเพื่อภาพถัดไป
// ======================================================================
#define IMAGE_AREA_X      0
#define IMAGE_AREA_Y      0
#define IMAGE_AREA_W      320
#define IMAGE_AREA_H      480

#define PREV_TOUCH_X_MAX  20
#define NEXT_TOUCH_X_MIN  300

// ======================================================================
// โซนสัมผัสสำหรับแถบไอคอนด้านล่างที่วางทับที่ตำแหน่ง (0,430)
// หมายเหตุ:
// 1) แถบไอคอนสูง 50 พิกเซล ครอบคลุม y = 430 ถึง 479
// 2) แบ่งพื้นที่กว้าง 320 พิกเซล ออกเป็น 5 ปุ่ม ปุ่มละ 64 พิกเซล
// 3) ลำดับปุ่มจากซ้ายไปขวา: ย้อนกลับ, ตั้งค่า, Home, ค้นหา, ถัดไป
// ======================================================================
#define ICON_BAR_X          0
#define ICON_BAR_Y          430
#define ICON_BAR_W          320
#define ICON_BAR_H          50
#define ICON_SLOT_W         64

#define ICON_PREV_X1        0
#define ICON_PREV_X2        63
#define ICON_SETTINGS_X1    64
#define ICON_SETTINGS_X2    127
#define ICON_HOME_X1        128
#define ICON_HOME_X2        191
#define ICON_SEARCH_X1      192
#define ICON_SEARCH_X2      255
#define ICON_NEXT_X1        256
#define ICON_NEXT_X2        319

// ======================================================================
// สร้างบัสจอ QSPI
// ======================================================================
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  45,
  47,
  21,
  48,
  40,
  39
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
// ใช้ Canvas ลดการกระพริบระหว่างเปลี่ยนหน้า/วาดข้อความ
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
// ตัวแปรสถานะของระบบ
// ======================================================================
uint16_t touchX = 0, touchY = 0;
bool lastTouchState = false;
bool imageMode = false;

// ======================================================================
// ตัวแปรควบคุม Slide Show ของหน้า Gallery
// หน้าที่:
// 1) ถ้าไม่มีการแตะเปลี่ยนภาพตามเวลาที่กำหนด ให้เลื่อนไปภาพถัดไปอัตโนมัติ
// 2) หน่วงหลังการแตะครั้งล่าสุด เพื่อไม่ให้ Slide Show เปลี่ยนภาพเร็วเกินไป
// 3) ใช้ร่วมกับเอฟเฟกต์ Curtain/Wipe ก่อนโหลดภาพถัดไป
// 4) รายการภาพของ Slide Show จะพยายามดึงจาก API ก่อน แล้วค่อย fallback
// ======================================================================
const unsigned long GALLERY_SLIDESHOW_INTERVAL_MS = 10000UL;
const unsigned long GALLERY_USER_IDLE_DELAY_MS    = 12000UL;
const uint8_t GALLERY_EFFECT_STEPS                = 12;
const uint16_t GALLERY_EFFECT_STEP_DELAY_MS       = 18;
unsigned long g_lastGalleryInteractionMs          = 0;
unsigned long g_lastGallerySlideMs                = 0;
bool wifiReady = false;
bool g_displayReady = false;
bool g_touchControllerDetected = false;
int currentImageIndex = 0;

enum ScreenMode {
  SCREEN_WELCOME = 0,
  SCREEN_DASHBOARD = 1,
  SCREEN_IMAGE = 2
};

ScreenMode currentScreen = SCREEN_WELCOME;

// ======================================================================
// ตัวแปรสถานะระบบโรงเรือนปลูกผัก
// หน้าที่:
// 1) เก็บค่าจากเซนเซอร์เพื่อแสดงบน Dashboard
// 2) เก็บสถานะเอาต์พุตที่สั่งจากจอสัมผัสหรือจาก MQTT
// 3) ใช้เป็นต้นแบบก่อนเชื่อมต่อฮาร์ดแวร์จริงในภายหลัง
// ======================================================================
float ghTempC       = 28.5f;
float ghExtTempC    = 35.2f;  // อุณหภูมิภายนอกโรงเรือน
float ghHumRH       = 72.0f;
float ghSoilMoist   = 58.0f;
float ghLightLux    = 18500.0f;
float ghWaterLevel  = 76.0f;
float ghPh          = 6.4f;
float ghEc          = 1.8f;

bool ghAutoMode     = true;
bool ghFanOn        = false;
bool ghPumpOn       = false;
bool ghMistOn       = false;
bool ghLightOn      = false;
bool ghVentOn       = false;

// ======================================================================
// ตัวแปรสำหรับวาดภาพ JPEG ด้วย JPEGDecoder
// ======================================================================
int16_t jpgDrawX = 0;
int16_t jpgDrawY = 0;
int16_t jpgClipX1 = 0;
int16_t jpgClipY1 = 0;
int16_t jpgClipX2 = IMAGE_AREA_W - 1;
int16_t jpgClipY2 = IMAGE_AREA_H - 1;
float   jpgScale  = 1.0f;

// ======================================================================
// line buffer สำหรับวาดภาพแบบย่อทีละแถว
// ใช้สูงสุดตามความกว้างพื้นที่ภาพ
// ======================================================================
uint16_t g_scaledLineBuffer[IMAGE_AREA_W];

// ======================================================================
// ตัวแปร debug HTTP/JPEG
// ======================================================================
String g_lastContentType = "";
int    g_lastHttpCode = -1;

// ======================================================================
// ตัวแปรคิวรีเฟรช Dashboard
// หน้าที่:
// 1) หลีกเลี่ยงการวาดจอ/โหลด HTTP ภายใน MQTT callback โดยตรง
// 2) ให้ loop() เป็นผู้เรียก refreshDashboardInteractiveArea() ภายหลัง
// 3) ลดโอกาสเกิด Guru Meditation จากการใช้งาน network ซ้อนใน callback
// ======================================================================
volatile bool g_dashboardRefreshPending = false;
unsigned long g_lastDashboardRefreshMs = 0;
unsigned long g_lastHttpFailMs = 0;


// ======================================================================
// ฟังก์ชัน mapTouchRange
// หน้าที่:
// 1) แปลงค่าดิบจาก touch ให้เป็นพิกัดบนหน้าจอ
// 2) จำกัดค่าไม่ให้หลุดนอกขอบเขต
// ======================================================================
uint16_t mapTouchRange(uint16_t value, uint16_t inMin, uint16_t inMax, uint16_t screenMax) {
  if (value < inMin) value = inMin;
  if (value > inMax) value = inMax;

  long out = map((long)value, (long)inMin, (long)inMax, 0L, (long)screenMax - 1L);

  if (out < 0) out = 0;
  if (out >= screenMax) out = screenMax - 1;

  return (uint16_t)out;
}

// ======================================================================
// ฟังก์ชัน getTouchPointRaw
// หน้าที่:
// 1) อ่านข้อมูลดิบจาก touch controller ผ่าน I2C
// 2) แปลงเป็นพิกัด pixel ตามขนาดจอ
// 3) คืนค่า true เมื่อมีการแตะจริง
// ======================================================================
bool getTouchPointRaw(uint16_t &x, uint16_t &y) {
  uint8_t data[AXS_MAX_TOUCH_NUMBER * 6 + 2] = {0};

  const uint8_t read_cmd[11] = {
    0xB5, 0xAB, 0xA5, 0x5A, 0x00, 0x00,
    (uint8_t)((AXS_MAX_TOUCH_NUMBER * 6 + 2) >> 8),
    (uint8_t)((AXS_MAX_TOUCH_NUMBER * 6 + 2) & 0xFF),
    0x00, 0x00, 0x00
  };

  Wire.beginTransmission(TOUCH_ADDR);
  Wire.write(read_cmd, 11);
  if (Wire.endTransmission() != 0) return false;

  if (Wire.requestFrom(TOUCH_ADDR, (uint8_t)sizeof(data)) != sizeof(data)) return false;

  for (int i = 0; i < (int)sizeof(data); i++) {
    data[i] = Wire.read();
  }

  if (data[1] == 0 || data[1] > AXS_MAX_TOUCH_NUMBER) return false;

  uint16_t rawX = ((data[2] & 0x0F) << 8) | data[3];
  uint16_t rawY = ((data[4] & 0x0F) << 8) | data[5];

  if (rawX > 4095 || rawY > 4095) return false;

  uint16_t px = mapTouchRange(rawX, TOUCH_RAW_X_MIN, TOUCH_RAW_X_MAX, LCD_NATIVE_WIDTH);
  uint16_t py = mapTouchRange(rawY, TOUCH_RAW_Y_MIN, TOUCH_RAW_Y_MAX, LCD_NATIVE_HEIGHT);

  if (TOUCH_INVERT_X) px = (LCD_NATIVE_WIDTH - 1) - px;
  if (TOUCH_INVERT_Y) py = (LCD_NATIVE_HEIGHT - 1) - py;

  x = px;
  y = py;
  return true;
}

// ======================================================================
// ฟังก์ชัน getTouchPoint
// หน้าที่:
// 1) นำค่าจาก getTouchPointRaw มาปรับตามการหมุนจอ
// ======================================================================
bool getTouchPoint(uint16_t &x, uint16_t &y) {
  uint16_t tx, ty;

  if (!getTouchPointRaw(tx, ty)) return false;

  switch (SCREEN_ROTATION) {
    case 0:
      x = tx; y = ty;
      break;
    case 1:
      x = ty; y = (LCD_NATIVE_WIDTH - 1) - tx;
      break;
    case 2:
      x = (LCD_NATIVE_WIDTH - 1) - tx;
      y = (LCD_NATIVE_HEIGHT - 1) - ty;
      break;
    case 3:
      x = (LCD_NATIVE_HEIGHT - 1) - ty;
      y = tx;
      break;
    default:
      x = tx; y = ty;
      break;
  }

  if (x >= gfx->width())  x = gfx->width() - 1;
  if (y >= gfx->height()) y = gfx->height() - 1;

  return true;
}

// ======================================================================
// ฟังก์ชัน drawCenteredText
// หน้าที่:
// 1) วาดข้อความให้อยู่กึ่งกลางแนวนอน
// ======================================================================
void drawCenteredText(const String &text, int y, uint16_t color, uint8_t size) {
  int16_t x1, y1;
  uint16_t w, h;

  gfx->setFont();
  gfx->setTextSize(size);
  gfx->getTextBounds(text, 0, y, &x1, &y1, &w, &h);

  int x = (gfx->width() - w) / 2;
  if (x < 0) x = 0;

  gfx->setCursor(x, y);
  gfx->setTextColor(color);
  gfx->print(text);
}

// ======================================================================
// ฟังก์ชัน getWiFiStatusText
// หน้าที่:
// 1) คืนข้อความสถานะ WiFi เพื่อใช้แสดงบนหน้า Dashboard
// ======================================================================
String getWiFiStatusText() {
  if (WiFi.status() == WL_CONNECTED) return "Connected";
  return "Disconnected";
}

// ======================================================================
// ฟังก์ชัน getIpText
// หน้าที่:
// 1) คืนค่า IP address ในรูปแบบข้อความ
// 2) ถ้ายังไม่เชื่อมต่อ WiFi ให้แสดงขีดแทน
// ======================================================================
String getIpText() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  return "-";
}

// ======================================================================
// ฟังก์ชัน probeI2CDevice
// หน้าที่:
// 1) ตรวจสอบว่ามีอุปกรณ์ I2C ตอบสนองที่ address ที่กำหนดหรือไม่
// 2) ใช้ยืนยันการมองเห็น touch controller ตอนเริ่มต้นระบบ
// ======================================================================
bool probeI2CDevice(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

// ======================================================================
// ฟังก์ชัน formatBoolOnOff
// หน้าที่:
// 1) แปลงค่า boolean เป็นข้อความ ON / OFF
// ======================================================================
String formatBoolOnOff(bool value) {
  return value ? "ON" : "OFF";
}

// ======================================================================
// ฟังก์ชัน formatBoolYesNo
// หน้าที่:
// 1) แปลงค่า boolean เป็นข้อความ YES / NO
// ======================================================================
String formatBoolYesNo(bool value) {
  return value ? "YES" : "NO";
}

// ======================================================================
// ฟังก์ชัน getTouchStatusText
// หน้าที่:
// 1) คืนค่าสถานะของ touch controller เพื่อแสดงบนหน้า Welcome
// ======================================================================
String getTouchStatusText() {
  return g_touchControllerDetected ? "Detected" : "Not found";
}

// ======================================================================
// ฟังก์ชัน getRssiText
// หน้าที่:
// 1) คืนค่าความแรงสัญญาณ WiFi เป็น dBm
// ======================================================================
String getRssiText() {
  if (WiFi.status() == WL_CONNECTED) {
    return String(WiFi.RSSI()) + " dBm";
  }
  return "-";
}

// ======================================================================
// ฟังก์ชัน getMacText
// หน้าที่:
// 1) คืนค่า MAC address เมื่อเชื่อมต่อ WiFi แล้ว
// ======================================================================
String getMacText() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.macAddress();
  }
  return "-";
}

// ======================================================================
// ฟังก์ชัน getMqttStatusText
// หน้าที่:
// 1) คืนค่าสถานะการเชื่อมต่อ MQTT เพื่อใช้แสดงบนหน้า Welcome
// ======================================================================
String getMqttStatusText() {
  return mqttClient.connected() ? "Connected" : "Disconnected";
}

// ======================================================================
// ฟังก์ชัน drawSystemReadyLine
// หน้าที่:
// 1) วาดข้อมูล 1 บรรทัดในรูปแบบ Label : Value
// 2) ใช้ตัวอักษรขนาดเล็กเพื่อแสดงข้อมูลจำนวนมากในหน้าเดียว
// ======================================================================
void drawSystemReadyLine(int16_t x, int16_t y, const String &label, const String &value, uint16_t valueColor) {
  gfx->setFont();
  gfx->setTextSize(1);
  gfx->setTextColor(gfx->color565(190, 205, 220));
  gfx->setCursor(x, y);
  gfx->print(label);

  gfx->setTextColor(valueColor);
  gfx->setCursor(x + 96, y);
  gfx->print(value);
}

// ======================================================================
// ฟังก์ชัน getMemoryTextKB
// หน้าที่:
// 1) คืนค่าหน่วยความจำในหน่วย KB เพื่อให้อ่านง่ายบนหน้า Dashboard
// ======================================================================
String getMemoryTextKB(uint32_t bytesValue) {
  return String(bytesValue / 1024) + " KB";
}

// ======================================================================
// ฟังก์ชัน getStorageTextMB
// หน้าที่:
// 1) คืนค่าขนาดหน่วยเก็บข้อมูลเป็น MB
// ======================================================================
String getStorageTextMB(uint64_t bytesValue) {
  return String((uint32_t)(bytesValue / (1024ULL * 1024ULL))) + " MB";
}

// ======================================================================
// ฟังก์ชัน getStorageSystemText
// หน้าที่:
// 1) แจ้งสถานะระบบจัดเก็บข้อมูลของโปรแกรมเวอร์ชันนี้
// ======================================================================
String getStorageSystemText() {
  return "Web image mode";
}

// ======================================================================
// ฟังก์ชัน buildFullUrl
// หน้าที่:
// 1) นำ baseUrl มาต่อกับชื่อไฟล์ภาพ
// 2) ลดการแก้ URL ซ้ำหลายจุดในโปรแกรม
// 3) ใช้ snprintf เพื่อป้องกัน buffer overflow
// ======================================================================
void buildFullUrl(const char* fileName, char* outUrl, size_t outSize) {
  if (outUrl == nullptr || outSize == 0) return;

  if (fileName == nullptr) {
    outUrl[0] = '\0';
    return;
  }

  // รองรับทั้งกรณีส่งชื่อไฟล์สั้น เช่น fan_on.jpg
  // และกรณีส่ง URL เต็ม เช่น http://192.168.1.201/IoTServer/dashboard_frame.jpg
  if (strncmp(fileName, "http://", 7) == 0 || strncmp(fileName, "https://", 8) == 0) {
    snprintf(outUrl, outSize, "%s", fileName);
    return;
  }

  snprintf(outUrl, outSize, "%s%s", baseUrl, fileName);
}

// ======================================================================
// ฟังก์ชัน jsonUnescape
// หน้าที่:
// 1) แปลงข้อความ JSON ที่มีการ escape เช่น \/ ให้กลับเป็น /
// 2) รองรับ \" และ \n เบื้องต้นสำหรับข้อมูลที่ดึงจาก API
// ======================================================================
String jsonUnescape(const String &input) {
  String out;
  out.reserve(input.length());

  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];
    if (c == '\\' && (i + 1) < input.length()) {
      char n = input[i + 1];
      if (n == '/' || n == '\\' || n == '"') {
        out += n;
        i++;
        continue;
      }
      if (n == 'n') {
        out += '\n';
        i++;
        continue;
      }
      if (n == 'r') {
        i++;
        continue;
      }
      if (n == 't') {
        out += '\t';
        i++;
        continue;
      }
    }
    out += c;
  }
  return out;
}

// ======================================================================
// ฟังก์ชัน clearSlideshowList
// หน้าที่:
// 1) ล้างรายการภาพจาก API เดิม
// 2) ใช้ก่อนโหลดข้อมูลชุดใหม่เพื่อป้องกันข้อมูลค้าง
// ======================================================================
void clearSlideshowList() {
  for (uint8_t i = 0; i < MAX_SLIDESHOW_IMAGES; i++) {
    slideshowImageNames[i] = "";
    slideshowImageUrls[i] = "";
  }
  slideshowImageCount = 0;
}

// ======================================================================
// ฟังก์ชัน parseSlideshowListJson
// หน้าที่:
// 1) แยก name และ url จาก JSON ที่ API ส่งกลับ
// 2) บันทึกลงในอาร์เรย์ slideshowImageNames และ slideshowImageUrls
// 3) คืนค่า true เมื่อพบข้อมูลภาพอย่างน้อย 1 รายการ
// หมายเหตุ:
// 1) เวอร์ชันนี้ parse ตามรูปแบบ JSON ของ API ตัวอย่าง
// 2) รองรับรูปแบบ {"name":"...","url":"..."} ต่อเนื่องกันหลายรายการ
// ======================================================================
bool parseSlideshowListJson(const String &json) {
  clearSlideshowList();

  int filesPos = json.indexOf("\"files\"");
  if (filesPos < 0) return false;

  int searchPos = filesPos;
  while (slideshowImageCount < MAX_SLIDESHOW_IMAGES) {
    int nameKey = json.indexOf("\"name\"", searchPos);
    if (nameKey < 0) break;

    int nameColon = json.indexOf(':', nameKey);
    int nameStart = json.indexOf('"', nameColon + 1);
    int nameEnd   = json.indexOf('"', nameStart + 1);
    if (nameColon < 0 || nameStart < 0 || nameEnd < 0) break;

    String rawName = json.substring(nameStart + 1, nameEnd);

    int urlKey = json.indexOf("\"url\"", nameEnd);
    if (urlKey < 0) break;

    int urlColon = json.indexOf(':', urlKey);
    int urlStart = json.indexOf('"', urlColon + 1);
    if (urlColon < 0 || urlStart < 0) break;

    int urlEnd = urlStart + 1;
    while (urlEnd < (int)json.length()) {
      if (json[urlEnd] == '"' && json[urlEnd - 1] != '\\') break;
      urlEnd++;
    }
    if (urlEnd >= (int)json.length()) break;

    String rawUrl = json.substring(urlStart + 1, urlEnd);

    slideshowImageNames[slideshowImageCount] = jsonUnescape(rawName);
    slideshowImageUrls[slideshowImageCount]  = jsonUnescape(rawUrl);
    slideshowImageCount++;

    searchPos = urlEnd + 1;
  }

  return (slideshowImageCount > 0);
}

// ======================================================================
// ฟังก์ชัน fetchSlideshowListFromApi
// หน้าที่:
// 1) เรียก API เพื่อดึงรายการภาพสำหรับ Slide Show
// 2) ตรวจ HTTP status และแยกข้อมูล JSON ที่ได้รับ
// 3) บันทึกเวลาโหลดล่าสุดไว้ใช้กำหนดรอบ refresh
// ======================================================================
bool fetchSlideshowListFromApi(bool forceRefresh = false) {
  if (!wifiReady) {
    wifiReady = connectWiFi();
    if (!wifiReady) return false;
  }

  unsigned long now = millis();
  if (!forceRefresh && slideshowListLoaded && slideshowImageCount > 0 &&
      (now - g_lastSlideshowFetchMs) < SLIDESHOW_LIST_REFRESH_MS) {
    return true;
  }

  HTTPClient http;
  WiFiClient client;
  http.setConnectTimeout(5000);
  http.setTimeout(8000);

  if (!http.begin(client, slideshowApiUrl)) {
    Serial.println("Slideshow API begin failed");
    return false;
  }

  int httpCode = http.GET();
  Serial.print("Slideshow API HTTP code = ");
  Serial.println(httpCode);

  if (httpCode != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  if (payload.length() == 0) return false;

  bool ok = parseSlideshowListJson(payload);
  if (ok) {
    slideshowListLoaded = true;
    g_lastSlideshowFetchMs = now;
    Serial.print("Slideshow API image count = ");
    Serial.println(slideshowImageCount);
  }
  return ok;
}

// ======================================================================
// ฟังก์ชัน getGalleryImageCount
// หน้าที่:
// 1) คืนจำนวนภาพที่ใช้จริงใน Gallery/Slide Show
// 2) ถ้ามีรายการจาก API ให้ใช้ข้อมูลจาก API ก่อน
// 3) ถ้า API ยังไม่พร้อม ให้ใช้ fallback ในโปรแกรม
// ======================================================================
int getGalleryImageCount() {
  if (slideshowListLoaded && slideshowImageCount > 0) {
    return slideshowImageCount;
  }
  return FALLBACK_IMAGE_COUNT;
}

// ======================================================================
// ฟังก์ชัน getGalleryImageNameByIndex
// หน้าที่:
// 1) คืนชื่อไฟล์ตามลำดับภาพปัจจุบัน
// 2) ใช้ข้อมูลจาก API ก่อน ถ้าไม่มีจึงใช้ fallback
// ======================================================================
String getGalleryImageNameByIndex(int index) {
  int count = getGalleryImageCount();
  if (count <= 0) return "";
  if (index < 0) index = 0;
  if (index >= count) index = count - 1;

  if (slideshowListLoaded && slideshowImageCount > 0) {
    return slideshowImageNames[index];
  }
  return String(fallbackImageFiles[index]);
}

// ======================================================================
// ฟังก์ชัน getGalleryImageUrlByIndex
// หน้าที่:
// 1) คืน URL เต็มของภาพตาม index
// 2) ถ้ามีข้อมูลจาก API จะใช้ URL ตรงจาก API
// 3) ถ้าไม่มี จะสร้าง URL จาก baseUrl + fallbackImageFiles
// ======================================================================
String getGalleryImageUrlByIndex(int index) {
  int count = getGalleryImageCount();
  if (count <= 0) return "";
  if (index < 0) index = 0;
  if (index >= count) index = count - 1;

  if (slideshowListLoaded && slideshowImageCount > 0) {
    return slideshowImageUrls[index];
  }

  char imageUrl[192];
  buildFullUrl(fallbackImageFiles[index], imageUrl, sizeof(imageUrl));
  return String(imageUrl);
}

// ======================================================================
// ฟังก์ชัน findGalleryImageIndexByName
// หน้าที่:
// 1) ค้นหาดัชนีของภาพจากชื่อไฟล์
// 2) ใช้กับคำสั่ง MQTT {"cmd":"show","file":"..."} ได้ทั้ง API และ fallback
// ======================================================================
int findGalleryImageIndexByName(const String &fileName) {
  int count = getGalleryImageCount();
  for (int i = 0; i < count; i++) {
    if (fileName.equalsIgnoreCase(getGalleryImageNameByIndex(i))) {
      return i;
    }
  }
  return -1;
}

// ======================================================================
// ฟังก์ชัน ensureSlideshowListAvailable
// หน้าที่:
// 1) พยายามโหลดรายการภาพจาก API ก่อนใช้งาน Gallery
// 2) ถ้าโหลดไม่ได้ จะยอมให้ระบบทำงานต่อด้วย fallback image list
// ======================================================================
void ensureSlideshowListAvailable(bool forceRefresh = false) {
  bool ok = fetchSlideshowListFromApi(forceRefresh);
  if (!ok) {
    Serial.println("Use fallback slideshow list");
  }
}


// ======================================================================
// ฟังก์ชัน drawRemoteIconByFile
// หน้าที่:
// 1) สร้าง URL เต็มจากชื่อไฟล์ภาพ
// 2) ดาวน์โหลดและวาดภาพ JPEG จาก Web Server
// 3) ใช้กับภาพเฟรม/ภาพหลักที่ยังต้องโหลดผ่านเครือข่าย
// 4) คืนค่า true เมื่อวาดสำเร็จ
// ======================================================================
bool drawRemoteIconByFile(const char* fileName, int16_t x, int16_t y) {
  if (fileName == nullptr || fileName[0] == '\0') return false;

  char iconUrl[192];
  buildFullUrl(fileName, iconUrl, sizeof(iconUrl));
  return drawJpegFromUrlAt(iconUrl, x, y);
}

// ======================================================================
// ฟังก์ชัน drawRgb565ImageAt
// หน้าที่:
// 1) วาดภาพจาก array สี RGB565 ที่เก็บอยู่ใน PROGMEM
// 2) อ่านค่าสีทีละพิกเซลด้วย pgm_read_word() แล้วส่งไปยังจอ
// 3) ใช้กับไอคอนคำสั่งบน Dashboard เพื่อไม่ต้องโหลดจากอินเทอร์เน็ต
// หมายเหตุ:
// 1) imageData ต้องชี้ไปยังข้อมูล uint16_t RGB565
// 2) imageW * imageH ต้องตรงกับขนาด array จริง
// ======================================================================
bool drawRgb565ImageAt(const uint16_t* imageData, uint16_t imageW, uint16_t imageH, int16_t posX, int16_t posY) {
  if (imageData == nullptr || imageW == 0 || imageH == 0) return false;

  for (uint16_t row = 0; row < imageH; row++) {
    for (uint16_t col = 0; col < imageW; col++) {
      uint16_t color565 = pgm_read_word(&imageData[(size_t)row * imageW + col]);
      gfx->drawPixel(posX + col, posY + row, color565);
    }
  }
  return true;
}

// ======================================================================
// ฟังก์ชัน drawDashboardIconFromArray
// หน้าที่:
// 1) วาดไอคอนคำสั่งจากข้อมูลภาพสี RGB565 ในหน่วยความจำ
// 2) แยกเป็นฟังก์ชันกลางเพื่อเรียกใช้ซ้ำได้ง่าย
// 3) คืนค่า true เมื่อวาดสำเร็จ
// ======================================================================
bool drawDashboardIconFromArray(const uint16_t* imageData, uint16_t imageW, uint16_t imageH, int16_t x, int16_t y) {
  return drawRgb565ImageAt(imageData, imageW, imageH, x, y);
}

// ======================================================================
// ฟังก์ชัน getWiFiStrengthText
// หน้าที่:
// 1) คืนค่าความแรงสัญญาณ WiFi เป็น dBm
// 2) ถ้ายังไม่เชื่อมต่อให้แสดงขีดแทน
// ======================================================================
String getWiFiStrengthText() {
  if (WiFi.status() == WL_CONNECTED) {
    return String(WiFi.RSSI()) + " dBm";
  }
  return "-";
}


// ======================================================================
// ฟังก์ชัน formatFloat1
// หน้าที่:
// 1) แปลงค่า float เป็นข้อความทศนิยม 1 ตำแหน่ง
// ======================================================================
String formatFloat1(float value) {
  char buf[24];
  dtostrf(value, 0, 1, buf);
  return String(buf);
}

String formatFloat2(float v) {
  return String(v, 2);
}


// ======================================================================
// ฟังก์ชัน extractJsonFloat
// หน้าที่:
// 1) ดึงค่าเลขทศนิยมจาก JSON แบบง่าย
// 2) รองรับทั้งจำนวนเต็มและทศนิยมติดเครื่องหมายลบ
// ======================================================================
bool extractJsonFloat(const String &json, const char* key, float &outValue) {
  if (key == nullptr) return false;

  String pattern = String("\"") + key + "\"";
  int keyPos = json.indexOf(pattern);
  if (keyPos < 0) return false;

  int colonPos = json.indexOf(':', keyPos + pattern.length());
  if (colonPos < 0) return false;

  int start = colonPos + 1;
  while (start < (int)json.length() && (json[start] == ' ' || json[start] == '	')) start++;

  int end = start;
  bool found = false;
  if (end < (int)json.length() && (json[end] == '-' || isDigit(json[end]) || json[end] == '.')) {
    end++;
    while (end < (int)json.length()) {
      char c = json[end];
      if (!(isDigit(c) || c == '.')) break;
      end++;
    }
    outValue = json.substring(start, end).toFloat();
    found = true;
  }

  return found;
}

// ======================================================================
// ฟังก์ชัน setActuatorByName
// หน้าที่:
// 1) ตั้งค่าสถานะเอาต์พุตตามชื่ออุปกรณ์
// 2) คืนค่า false เมื่อชื่อไม่ตรงกับอุปกรณ์ที่รองรับ
// ======================================================================
bool setActuatorByName(const String &name, bool state) {
  if (name.equalsIgnoreCase("fan"))   { ghFanOn = state;   return true; }
  if (name.equalsIgnoreCase("pump"))  { ghPumpOn = state;  return true; }
  if (name.equalsIgnoreCase("mist"))  { ghMistOn = state;  return true; }
  if (name.equalsIgnoreCase("light")) { ghLightOn = state; return true; }
  if (name.equalsIgnoreCase("vent"))  { ghVentOn = state;  return true; }
  return false;
}

// ======================================================================
// ฟังก์ชัน setSensorByName
// หน้าที่:
// 1) อัปเดตค่าตัวแปรเซนเซอร์ตามชื่อ field ที่รับมาจาก MQTT
// ======================================================================
bool setSensorByName(const String &name, float value) {
  if (name.equalsIgnoreCase("temp") || name.equalsIgnoreCase("temperature")) { ghTempC = value; return true; }
  if (name.equalsIgnoreCase("exttemp") || name.equalsIgnoreCase("temp_ext") || name.equalsIgnoreCase("outtemp") || name.equalsIgnoreCase("tempOut")) { ghExtTempC = value; return true; }
  if (name.equalsIgnoreCase("hum") || name.equalsIgnoreCase("humidity")) { ghHumRH = value; return true; }
  if (name.equalsIgnoreCase("soil") || name.equalsIgnoreCase("soilMoist")) { ghSoilMoist = value; return true; }
  if (name.equalsIgnoreCase("lux") || name.equalsIgnoreCase("lightLux")) { ghLightLux = value; return true; }
  if (name.equalsIgnoreCase("level") || name.equalsIgnoreCase("waterLevel")) { ghWaterLevel = value; return true; }
  if (name.equalsIgnoreCase("ph")) { ghPh = value; return true; }
  if (name.equalsIgnoreCase("ec")) { ghEc = value; return true; }
  return false;
}

// ======================================================================
// ฟังก์ชัน extractJsonString
// หน้าที่:
// 1) ดึงค่าข้อความของ key ที่ต้องการจาก JSON แบบง่าย
// 2) เหมาะกับคำสั่ง JSON ขนาดเล็กจาก App ภายนอก
// 3) ลดการพึ่งพาไลบรารีเพิ่ม เพื่อให้คอมไพล์ได้ง่ายขึ้น
// ตัวอย่าง:
//    {"cmd":"next"}
//    {"file":"TADSANEE5.jpg"}
// ======================================================================
bool extractJsonString(const String &json, const char* key, String &outValue) {
  outValue = "";
  if (key == nullptr) return false;

  String pattern = String("\"") + key + "\"";
  int keyPos = json.indexOf(pattern);
  if (keyPos < 0) return false;

  int colonPos = json.indexOf(':', keyPos + pattern.length());
  if (colonPos < 0) return false;

  int firstQuote = json.indexOf('"', colonPos + 1);
  if (firstQuote < 0) return false;

  int secondQuote = json.indexOf('"', firstQuote + 1);
  if (secondQuote < 0) return false;

  outValue = json.substring(firstQuote + 1, secondQuote);
  return true;
}

// ======================================================================
// ฟังก์ชัน extractJsonInt
// หน้าที่:
// 1) ดึงค่าจำนวนเต็มของ key ที่ต้องการจาก JSON แบบง่าย
// 2) รองรับรูปแบบ เช่น {"index":3}
// ======================================================================
bool extractJsonInt(const String &json, const char* key, int &outValue) {
  if (key == nullptr) return false;

  String pattern = String("\"") + key + "\"";
  int keyPos = json.indexOf(pattern);
  if (keyPos < 0) return false;

  int colonPos = json.indexOf(':', keyPos + pattern.length());
  if (colonPos < 0) return false;

  int start = colonPos + 1;
  while (start < (int)json.length() && (json[start] == ' ' || json[start] == '	')) start++;

  int end = start;
  if (end < (int)json.length() && (json[end] == '-' || isDigit(json[end]))) {
    end++;
    while (end < (int)json.length() && isDigit(json[end])) end++;
    outValue = json.substring(start, end).toInt();
    return true;
  }

  return false;
}

// ======================================================================
// ฟังก์ชัน extractJsonBool
// หน้าที่:
// 1) ดึงค่า true/false ของ key ที่ต้องการจาก JSON แบบง่าย
// 2) รองรับรูปแบบ เช่น {"value":true}
// ======================================================================

// ======================================================================
// ฟังก์ชัน requestDashboardRefresh
// หน้าที่:
// 1) ตั้งธงให้ loop() ไปรีเฟรช Dashboard ในจังหวะที่ปลอดภัย
// 2) ไม่เรียกโหลดภาพ/HTTP จากภายใน MQTT callback โดยตรง
// ======================================================================
void requestDashboardRefresh() {
  g_dashboardRefreshPending = true;
}

// ======================================================================
// ฟังก์ชัน handleAgrSensorJson
// หน้าที่:
// 1) รับ JSON จาก MQTT กลุ่ม AGRI แล้วแมปค่าไปยังตัวแปร Dashboard
// 2) รองรับ Topic AGRI/TEMP/esp32_01 สำหรับ temp_c, t2, rh
// 3) รองรับ field อื่นที่ส่งมาใน payload เช่น gm, Glux, gwl, ph, ec
// 4) เมื่อมีการอัปเดตค่าและหน้าจออยู่ที่ Dashboard จะ redraw ค่าทับทันที
// ======================================================================
void handleAgrSensorJson(const char* topic, const String &payload) {
  bool updated = false;
  float v = 0.0f;

  if (topic != nullptr && strcmp(topic, "AGRI/TEMP/esp32_01") == 0) {
    if (extractJsonFloat(payload, "temp_c", v) || extractJsonFloat(payload, "t1", v)) { ghTempC = v; updated = true; }
    if (extractJsonFloat(payload, "t2", v))     { ghExtTempC = v; updated = true; }
    if (extractJsonFloat(payload, "rh", v) || extractJsonFloat(payload, "humidity", v) || extractJsonFloat(payload, "hum", v)) { ghHumRH = v; updated = true; }
  }

  if (extractJsonFloat(payload, "gm", v)) {
    ghSoilMoist = v;
    updated = true;
  }

  if (extractJsonFloat(payload, "Glux", v) || extractJsonFloat(payload, "glux", v) || extractJsonFloat(payload, "lux", v)) {
    ghLightLux = v;
    updated = true;
  }

  if (extractJsonFloat(payload, "gwl", v)) {
    ghWaterLevel = v;
    updated = true;
  }

  if (extractJsonFloat(payload, "ph", v) || extractJsonFloat(payload, "pH", v)) {
    ghPh = v;
    updated = true;
  }

  if (extractJsonFloat(payload, "ec", v) || extractJsonFloat(payload, "EC", v)) {
    ghEc = v;
    updated = true;
  }

  if (updated && currentScreen == SCREEN_DASHBOARD) {
    requestDashboardRefresh();
  }
}

bool extractJsonBool(const String &json, const char* key, bool &outValue) {
  if (key == nullptr) return false;

  String pattern = String("\"") + key + "\"";
  int keyPos = json.indexOf(pattern);
  if (keyPos < 0) return false;

  int colonPos = json.indexOf(':', keyPos + pattern.length());
  if (colonPos < 0) return false;

  int start = colonPos + 1;
  while (start < (int)json.length() && (json[start] == ' ' || json[start] == '	')) start++;

  if (json.startsWith("true", start)) {
    outValue = true;
    return true;
  }

  if (json.startsWith("false", start)) {
    outValue = false;
    return true;
  }

  return false;
}

// ======================================================================
// ฟังก์ชัน publishSystemStatus
// หน้าที่:
// 1) ส่งสถานะการทำงานกลับไปยัง MQTT เพื่อให้ App ภายนอกรับทราบผล
// 2) ใช้สำหรับ debug และยืนยันว่าคำสั่ง JSON ถูกประมวลผลแล้ว
// ======================================================================
void publishSystemStatus(const char* eventName, const char* detail) {
  if (!mqttClient.connected()) return;

  char payload[256];
  snprintf(payload, sizeof(payload),
           "{\"event\":\"%s\",\"detail\":\"%s\",\"imageMode\":%s,\"index\":%d,\"millis\":%lu}",
           eventName ? eventName : "unknown",
           detail ? detail : "",
           imageMode ? "true" : "false",
           currentImageIndex,
           (unsigned long)millis());

  bool ok = mqttClient.publish(mqttTopicStatus, payload);
  Serial.print("MQTT status publish -> ");
  Serial.println(ok ? "OK" : "FAILED");
}


// ======================================================================
// ฟังก์ชัน publishControlState
// หน้าที่:
// 1) ส่ง snapshot สถานะทั้งหมดของหน้า Dashboard กลับไปยัง App ภายนอกผ่าน MQTT
// 2) ใช้ยืนยันผลหลังรับคำสั่งจากจอสัมผัสหรือคำสั่ง JSON ผ่าน MQTT
// 3) รวมทั้งสถานะโหมดทำงาน เอาต์พุต และค่าจากเซนเซอร์หลักใน payload เดียว
// ======================================================================
void publishControlState(const char* source) {
  if (!ensureMqttConnected()) {
    Serial.println("MQTT not connected, skip control publish");
    return;
  }

  char payload[512];
  snprintf(payload, sizeof(payload),
           "{\"event\":\"control_state\",\"source\":\"%s\",\"screen\":\"%s\",\"imageMode\":%s,\"index\":%d,\"autoMode\":%s,\"fan\":%s,\"pump\":%s,\"mist\":%s,\"light\":%s,\"vent\":%s,\"temp\":%.1f,\"extTemp\":%.1f,\"hum\":%.1f,\"soil\":%.1f,\"lux\":%.1f,\"level\":%.1f,\"ph\":%.2f,\"ec\":%.2f,\"millis\":%lu}",
           source ? source : "unknown",
           (currentScreen == SCREEN_WELCOME) ? "welcome" :
           (currentScreen == SCREEN_DASHBOARD) ? "dashboard" : "image",
           imageMode ? "true" : "false",
           currentImageIndex,
           ghAutoMode ? "true" : "false",
           ghFanOn ? "true" : "false",
           ghPumpOn ? "true" : "false",
           ghMistOn ? "true" : "false",
           ghLightOn ? "true" : "false",
           ghVentOn ? "true" : "false",
           ghTempC,
           ghExtTempC,
           ghHumRH,
           ghSoilMoist,
           ghLightLux,
           ghWaterLevel,
           ghPh,
           ghEc,
           (unsigned long)millis());

  bool ok = mqttClient.publish(mqttTopicStatus, payload);
  Serial.print("MQTT control publish -> ");
  Serial.println(ok ? "OK" : "FAILED");

  if (!ok) {
    Serial.print("MQTT state = ");
    Serial.println(mqttClient.state());
    if (WiFi.status() == WL_CONNECTED) {
      mqttClient.disconnect();
    }
  }
}

// ======================================================================
// ฟังก์ชัน ensureMqttConnected
// หน้าที่:
// 1) ตรวจและเชื่อมต่อ MQTT เมื่อยังไม่เชื่อมต่อ
// 2) ใช้ Client ID แบบสุ่มจาก MAC + เวลาเพื่อหลีกเลี่ยงชนกัน
// ======================================================================
bool ensureMqttConnected() {
  if (WiFi.status() != WL_CONNECTED) return false;
  if (mqttClient.connected()) return true;

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setKeepAlive(15);
  mqttClient.setSocketTimeout(5);

  String clientId = "IoTAGIApp-" + String((uint32_t)ESP.getEfuseMac(), HEX) + "-" + String(millis(), HEX);
  bool ok = mqttClient.connect(clientId.c_str());

  Serial.print("MQTT connect to ");
  Serial.print(mqttServer);
  Serial.print(":");
  Serial.print(mqttPort);
  Serial.print(" -> ");
  Serial.println(ok ? "OK" : "FAILED");

  if (ok) {
    bool subOk = mqttClient.subscribe(mqttTopicCommand);
    Serial.print("MQTT subscribe ");
    Serial.print(mqttTopicCommand);
    Serial.print(" -> ");
    Serial.println(subOk ? "OK" : "FAILED");

    bool subAgriOk = mqttClient.subscribe(mqttTopicAgriAll);
    Serial.print("MQTT subscribe ");
    Serial.print(mqttTopicAgriAll);
    Serial.print(" -> ");
    Serial.println(subAgriOk ? "OK" : "FAILED");

    publishSystemStatus("mqtt_connected", "ready");
  }

  return ok;
}

// ======================================================================
// ฟังก์ชัน publishTouchPoint
// หน้าที่:
// 1) ส่งค่าพิกัด Touch X,Y ไปยัง MQTT topic ที่กำหนด
// 2) รูปแบบข้อมูลเป็น JSON อ่านง่ายและนำไปใช้ต่อได้สะดวก
// ======================================================================
void publishTouchPoint(uint16_t x, uint16_t y) {
  if (!ensureMqttConnected()) {
    Serial.println("MQTT not connected, skip touch publish");
    return;
  }

  char payload[96];
  snprintf(payload, sizeof(payload), "{\"x\":%u,\"y\":%u,\"imageMode\":%s,\"millis\":%lu}",
           x, y, imageMode ? "true" : "false", (unsigned long)millis());

  bool ok = mqttClient.publish(mqttTopicTouch, payload);

  Serial.print("MQTT publish ");
  Serial.print(mqttTopicTouch);
  Serial.print(" = ");
  Serial.print(payload);
  Serial.print(" -> ");
  Serial.println(ok ? "OK" : "FAILED");
}

// ======================================================================
// ฟังก์ชัน handleMqttJsonCommand
// หน้าที่:
// 1) แปลคำสั่ง JSON ที่รับมาจาก App ภายนอกผ่าน MQTT
// 2) ควบคุมหน้า Home, โหมดภาพ, การเลื่อนภาพ, การเลือกภาพตาม index
// 3) รองรับการเปลี่ยน baseUrl จากภายนอกได้
// ตัวอย่างคำสั่ง:
//    {"cmd":"home"}
//    {"cmd":"next"}
//    {"cmd":"prev"}
//    {"cmd":"show","index":5}
//    {"cmd":"show","file":"TADSANEE8.jpg"}
//    {"cmd":"imageMode","value":true}
//    {"cmd":"setBaseUrl","baseUrl":"http://192.168.1.240/IoTServer/"}
// ======================================================================
bool handleMqttJsonCommand(const String &json) {
  String cmd;
  extractJsonString(json, "cmd", cmd);
  cmd.trim();

  if (cmd.length() == 0) {
    publishSystemStatus("mqtt_error", "missing_cmd");
    return false;
  }

  if (cmd.equalsIgnoreCase("home")) {
    showStartHomeScreen();
    publishSystemStatus("cmd_home", "ok");
    publishControlState("mqtt_home");
    return true;
  }

  if (cmd.equalsIgnoreCase("dashboard")) {
    showDashboardScreen();
    publishSystemStatus("cmd_dashboard", "ok");
    publishControlState("mqtt_dashboard");
    return true;
  }

  if (cmd.equalsIgnoreCase("next")) {
    imageMode = true;
    currentScreen = SCREEN_IMAGE;
    showNextImage();
    publishSystemStatus("cmd_next", "ok");
    return true;
  }

  if (cmd.equalsIgnoreCase("prev")) {
    imageMode = true;
    currentScreen = SCREEN_IMAGE;
    showPrevImage();
    publishSystemStatus("cmd_prev", "ok");
    return true;
  }

  if (cmd.equalsIgnoreCase("show")) {
    int indexValue = -1;
    String fileName;

    ensureSlideshowListAvailable(false);

    if (extractJsonInt(json, "index", indexValue)) {
      if (indexValue >= 0 && indexValue < getGalleryImageCount()) {
        currentImageIndex = indexValue;
        imageMode = true;
        currentScreen = SCREEN_IMAGE;
        showCurrentImage();
        publishSystemStatus("cmd_show_index", "ok");
        return true;
      }
      publishSystemStatus("mqtt_error", "index_out_of_range");
      return false;
    }

    if (extractJsonString(json, "file", fileName)) {
      int foundIndex = findGalleryImageIndexByName(fileName);
      if (foundIndex >= 0) {
        currentImageIndex = foundIndex;
        imageMode = true;
        currentScreen = SCREEN_IMAGE;
        showCurrentImage();
        publishSystemStatus("cmd_show_file", "ok");
        return true;
      }
      publishSystemStatus("mqtt_error", "file_not_found");
      return false;
    }

    publishSystemStatus("mqtt_error", "show_requires_index_or_file");
    return false;
  }

  if (cmd.equalsIgnoreCase("refreshSlideshow")) {
    bool ok = fetchSlideshowListFromApi(true);
    publishSystemStatus("cmd_refresh_slideshow", ok ? "ok" : "api_failed");
    return ok;
  }

  if (cmd.equalsIgnoreCase("imageMode")) {
    bool modeValue = false;
    if (extractJsonBool(json, "value", modeValue)) {
      imageMode = modeValue;
      if (imageMode) {
        currentScreen = SCREEN_IMAGE;
        showCurrentImage();
      } else {
        showDashboardScreen();
      }
      publishSystemStatus("cmd_imageMode", modeValue ? "true" : "false");
      publishControlState("mqtt_imageMode");
      return true;
    }
    publishSystemStatus("mqtt_error", "imageMode_requires_bool_value");
    return false;
  }

  if (cmd.equalsIgnoreCase("mode")) {
    String value;
    if (extractJsonString(json, "value", value)) {
      ghAutoMode = value.equalsIgnoreCase("auto");
      if (currentScreen == SCREEN_DASHBOARD) drawDashboardScreen();
      publishSystemStatus("cmd_mode", ghAutoMode ? "auto" : "manual");
      publishControlState("mqtt_mode");
      return true;
    }
    publishSystemStatus("mqtt_error", "mode_requires_value");
    return false;
  }

  if (cmd.equalsIgnoreCase("control")) {
    String target;
    bool state = false;
    if (extractJsonString(json, "target", target) && extractJsonBool(json, "value", state)) {
      if (setActuatorByName(target, state)) {
        if (currentScreen == SCREEN_DASHBOARD) drawDashboardScreen();
        publishSystemStatus("cmd_control", target.c_str());
        publishControlState("mqtt_control");
        return true;
      }
      publishSystemStatus("mqtt_error", "unknown_target");
      return false;
    }
    publishSystemStatus("mqtt_error", "control_requires_target_and_value");
    return false;
  }

  if (cmd.equalsIgnoreCase("sensorUpdate")) {
    String target;
    float value = 0.0f;
    if (extractJsonString(json, "target", target) && extractJsonFloat(json, "value", value)) {
      if (setSensorByName(target, value)) {
        if (currentScreen == SCREEN_DASHBOARD) drawDashboardScreen();
        publishSystemStatus("cmd_sensorUpdate", target.c_str());
        publishControlState("mqtt_sensor");
        return true;
      }
      publishSystemStatus("mqtt_error", "unknown_sensor");
      return false;
    }
    publishSystemStatus("mqtt_error", "sensorUpdate_requires_target_and_value");
    return false;
  }

  if (cmd.equalsIgnoreCase("setBaseUrl")) {
    String newBaseUrl;
    if (extractJsonString(json, "baseUrl", newBaseUrl)) {
      if (newBaseUrl.length() < (int)sizeof(baseUrl)) {
        strncpy(baseUrl, newBaseUrl.c_str(), sizeof(baseUrl) - 1);
        baseUrl[sizeof(baseUrl) - 1] = '\0';
        publishSystemStatus("cmd_setBaseUrl", baseUrl);
        return true;
      }
      publishSystemStatus("mqtt_error", "baseUrl_too_long");
      return false;
    }
    publishSystemStatus("mqtt_error", "missing_baseUrl");
    return false;
  }

  if (cmd.equalsIgnoreCase("status")) {
    publishSystemStatus("cmd_status", "ok");
    publishControlState("mqtt_status");
    return true;
  }

  publishSystemStatus("mqtt_error", "unknown_cmd");
  return false;
}

// ======================================================================
// ฟังก์ชัน mqttCallback
// หน้าที่:
// 1) รับข้อความจาก MQTT topic คำสั่ง
// 2) แปลง payload เป็น String
// 3) ส่งต่อไปยังตัวแปลคำสั่ง JSON
// ======================================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("MQTT RX topic = ");
  Serial.println(topic ? topic : "");
  Serial.print("MQTT RX payload = ");
  Serial.println(message);

  handleAgrSensorJson(topic, message);

  if (topic && strcmp(topic, mqttTopicCommand) == 0) {
    handleMqttJsonCommand(message);
  }
}

// ======================================================================
// ฟังก์ชัน drawStatusPill
// หน้าที่:
// 1) วาดป้ายสถานะเปิด/ปิด แบบง่ายสำหรับ Dashboard
// ======================================================================
void drawStatusPill(int x, int y, int w, int h, const char* label, bool onState) {
  uint16_t fillColor = onState ? GREEN : RED;
  gfx->fillRoundRect(x, y, w, h, 8, fillColor);
  gfx->drawRoundRect(x, y, w, h, 8, WHITE);
  gfx->setFont();
  gfx->setTextSize(1);
  gfx->setTextColor(BLACK);
  gfx->setCursor(x + 8, y + 18);
  gfx->print(label);
}

// ======================================================================
// ฟังก์ชัน drawDashboardButton
// หน้าที่:
// 1) วาดปุ่มควบคุมอุปกรณ์สำหรับหน้า Dashboard
// ======================================================================
void drawDashboardButton(int x, int y, int w, int h, const char* title, bool onState) {
  uint16_t fillColor = onState ? CYAN : GRAY;
  gfx->fillRoundRect(x, y, w, h, 10, fillColor);
  gfx->drawRoundRect(x, y, w, h, 10, WHITE);
  gfx->setFont();
  gfx->setTextSize(2);
  gfx->setTextColor(BLACK);
  gfx->setCursor(x + 10, y + 24);
  gfx->print(title);
  gfx->setTextSize(1);
  gfx->setCursor(x + 10, y + 46);
  gfx->print(onState ? "ON" : "OFF");
}

// ======================================================================
// ฟังก์ชัน drawDashboardValueBox
// หน้าที่:
// 1) วาดกล่องค่าเซนเซอร์ 2 คอลัมน์บน Dashboard
// ======================================================================
void drawDashboardValueBox(int x, int y, int w, int h, const char* title, const String &value, const char* unit, uint16_t color) {
  gfx->drawRoundRect(x, y, w, h, 8, color);
  gfx->setFont();
  gfx->setTextSize(1);
  gfx->setTextColor(GRAY);
  gfx->setCursor(x + 8, y + 14);
  gfx->print(title);
  gfx->setTextSize(2);
  gfx->setTextColor(WHITE);
  gfx->setCursor(x + 8, y + 38);
  gfx->print(value);
  gfx->setTextSize(1);
  gfx->setTextColor(color);
  gfx->setCursor(x + 8, y + 56);
  gfx->print(unit);
}

// ======================================================================
// ฟังก์ชัน drawDashboardFrameFromUrl
// หน้าที่:
// 1) โหลดเฟรม Dashboard จาก Web Server ตามค่า baseUrl
// 2) ใช้ชื่อไฟล์ dashboardFrameFile เพียงจุดเดียวเพื่อแก้ไขภายหลังได้ง่าย
// 3) คืนค่า true เมื่อโหลดและวาดเฟรมสำเร็จ
// หมายเหตุ:
// 1) ควรเก็บไฟล์เฟรมไว้ที่
//    http://agriculture.3bbddns.com:37473/IoTServer/dashboard_frame.jpg
// 2) หากใช้ชื่อไฟล์อื่น ให้แก้ที่ตัวแปร dashboardFrameFile
// ======================================================================
bool drawDashboardFrameFromUrl() {
  if (g_lastHttpFailMs != 0 && millis() - g_lastHttpFailMs < 1500) return false;
  return drawRemoteIconByFile(dashboardFrameFile, 0, 0);
}

// ======================================================================
// ฟังก์ชัน drawDashboardOverlayValues
// หน้าที่:
// 1) วาดค่าจริงทับบนเฟรม Dashboard ที่โหลดจาก URL
// 2) ล้างเฉพาะบริเวณตัวเลขและพื้นที่ไอคอนด้วยสีดำ เพื่อให้กลมกลืนกับเฟรม
// 3) ใช้ตำแหน่งให้ใกล้เคียงกับเฟรม Dashboard รุ่นล่าสุดของผู้ใช้
// ======================================================================
void drawDashboardOverlayValues() {
  gfx->setFont();
  gfx->setTextWrap(false);

  // ล้างพื้นที่ตัวเลขให้กว้างขึ้น เพื่อรองรับตัวหนังสือขนาดใหญ่ขึ้น
  gfx->fillRect(34, 94, 82, 30, BLACK);     // Air Temp
  gfx->fillRect(116, 94, 82, 30, BLACK);    // Humidity
  gfx->fillRect(220, 94, 90, 30, BLACK);    // Soil Moisture

  gfx->fillRect(40, 160, 74, 28, BLACK);    // Water Level
  gfx->fillRect(118, 160, 96, 28, BLACK);   // Light Level
  gfx->fillRect(236, 140, 58, 28, BLACK);   // pH
  gfx->fillRect(206, 174, 100, 28, BLACK);  // EC

  gfx->fillRect(48, 252, 96, 30, BLACK);    // Greenhouse Temp
  gfx->fillRect(188, 252, 106, 30, BLACK);  // Ext. Greenhouse Temp

  // พื้นที่ไอคอน CONTROL & MENU
  gfx->fillRect(18, 388, 286, 70, BLACK);

  // ค่าหลักด้านบนให้ใหญ่ขึ้น อ่านง่ายขึ้น
  gfx->setTextColor(WHITE, BLACK);
  gfx->setTextSize(2);
  gfx->setCursor(45, 118);  gfx->print(formatFloat1(ghTempC));
  gfx->setCursor(125, 118); gfx->print(formatFloat1(ghHumRH));
  gfx->setCursor(230, 118); gfx->print(formatFloat1(ghSoilMoist));

  gfx->setCursor(50, 182);  gfx->print(formatFloat1(ghWaterLevel));

  // ค่าแสงยาวกว่าช่องอื่น จึงใช้ขนาด 1 เพื่อไม่ให้ล้นกรอบ
  gfx->setTextSize(1);
  gfx->setCursor(128, 180); gfx->print(String((int)ghLightLux));

  gfx->setTextSize(2);
  gfx->setCursor(242, 158); gfx->print(formatFloat1(ghPh));

  // ค่า EC ใช้ขนาด 1 เพื่อไม่ให้ชนชื่อหน่วยในเฟรม
  gfx->setTextSize(1);
  gfx->setCursor(214, 190); gfx->print(formatFloat2(ghEc));

  // ค่าอุณหภูมิด้านล่างขยายให้เด่นขึ้น
  gfx->setTextSize(2);
  gfx->setCursor(58, 272);  gfx->print(formatFloat1(ghTempC));
  gfx->setCursor(198, 272); gfx->print(formatFloat1(ghExtTempC));

  // เน้นสถานะการเชื่อมต่อแบบจุดเล็กบนแถบบน
  gfx->fillCircle(280, 42, 4, (wifiReady && WiFi.status() == WL_CONNECTED) ? GREEN : RED);
  gfx->fillCircle(294, 42, 4, mqttClient.connected() ? GREEN : RED);
}

// ======================================================================
// ฟังก์ชัน drawDashboardIconsFromUrl
// หน้าที่:
// 1) วาดไอคอน FAN / PUMP / MIST / LIGHT / GALLERY จากไฟล์ icons_rgb565_V1.h
// 2) ไม่ต้องโหลดไอคอนจากอินเทอร์เน็ต จึงตอบสนองได้เร็วและเสถียรกว่าเดิม
// 3) หากข้อมูลภาพไม่พร้อม จะ fallback เป็นปุ่มกราฟิกเดิมเพื่อให้ระบบใช้งานต่อได้
// หมายเหตุ:
// 1) icon_fan_on มีข้อมูล 51x50 พิกเซล
// 2) ไอคอนชุดอื่นใช้ขนาด 50x50 พิกเซล
// ======================================================================
void drawDashboardIconsFromUrl() {
  bool okFan = false;
  bool okPump = false;
  bool okMist = false;
  bool okLight = false;
  bool okGallery = false;

  if (ghFanOn) {
    okFan = drawDashboardIconFromArray(icon_fan_on, 51, 50, DASH_ICON_X1, DASH_ICON_Y1);
  } else {
    okFan = drawDashboardIconFromArray(icon_fan_off, 50, 50, DASH_ICON_X1, DASH_ICON_Y1);
  }

  if (ghPumpOn) {
    okPump = drawDashboardIconFromArray(icon_pump_on, 50, 50, DASH_ICON_X2, DASH_ICON_Y1);
  } else {
    okPump = drawDashboardIconFromArray(icon_pump_off, 50, 50, DASH_ICON_X2, DASH_ICON_Y1);
  }

  if (ghMistOn) {
    okMist = drawDashboardIconFromArray(icon_mist_on, 50, 50, DASH_ICON_X3, DASH_ICON_Y1);
  } else {
    okMist = drawDashboardIconFromArray(icon_mist_off, 50, 50, DASH_ICON_X3, DASH_ICON_Y1);
  }

  if (ghLightOn) {
    okLight = drawDashboardIconFromArray(icon_light_on, 50, 50, DASH_ICON_X4, DASH_ICON_Y1);
  } else {
    okLight = drawDashboardIconFromArray(icon_light_off, 50, 50, DASH_ICON_X4, DASH_ICON_Y1);
  }

  okGallery = drawDashboardIconFromArray(icon_gallery, 50, 50, DASH_ICON_X5, DASH_ICON_Y1);

  if (!okFan)     drawDashboardButton(DASH_ICON_X1, DASH_ICON_Y1, DASH_ICON_W, DASH_ICON_H, "FAN", ghFanOn);
  if (!okPump)    drawDashboardButton(DASH_ICON_X2, DASH_ICON_Y1, DASH_ICON_W, DASH_ICON_H, "PUMP", ghPumpOn);
  if (!okMist)    drawDashboardButton(DASH_ICON_X3, DASH_ICON_Y1, DASH_ICON_W, DASH_ICON_H, "MIST", ghMistOn);
  if (!okLight)   drawDashboardButton(DASH_ICON_X4, DASH_ICON_Y1, DASH_ICON_W, DASH_ICON_H, "LIGHT", ghLightOn);
  if (!okGallery) drawDashboardButton(DASH_ICON_X5, DASH_ICON_Y1, DASH_ICON_W, DASH_ICON_H, "GAL", false);
}


// ======================================================================
// ฟังก์ชัน refreshDashboardInteractiveArea
// หน้าที่:
// 1) รีเฟรชเฉพาะส่วนที่เปลี่ยนบนหน้า Dashboard โดยไม่วาดทั้งหน้าซ้ำ
// 2) ใช้ได้ทั้งกรณีโหลดเฟรมจาก URL สำเร็จ และกรณี fallback แบบวาดกราฟิก
// 3) ช่วยให้การกดปุ่มบน Dashboard ตอบสนองทันทีหลังค่าสถานะเปลี่ยน
// ======================================================================
void refreshDashboardInteractiveArea() {
  if (currentScreen != SCREEN_DASHBOARD) return;

  // ถ้าอยู่ในโหมดแสดงภาพเต็มจอ ไม่ต้องรีเฟรช Dashboard
  if (imageMode) return;

  // กรณีใช้เฟรมจาก URL ให้รีเฟรชเฉพาะส่วนทับค่าและไอคอน
  if (drawDashboardFrameFromUrl()) {
    drawDashboardOverlayValues();
    drawDashboardIconsFromUrl();
    gfx->flush();
    return;
  }

  // ถ้าเฟรมจาก URL โหลดไม่สำเร็จ ให้กลับไปวาดหน้า Dashboard แบบเต็มทั้งหน้า
  gfx->fillScreen(BLACK);

  gfx->setFont();
  gfx->setTextSize(2);
  gfx->setTextColor(CYAN);
  gfx->setCursor(10, 24);
  gfx->print("GREENHOUSE DASHBOARD");

  drawStatusPill(10, 34, 66, 22, ghAutoMode ? "AUTO" : "MANUAL", ghAutoMode);
  drawStatusPill(84, 34, 70, 22, wifiReady && WiFi.status() == WL_CONNECTED ? "WIFI" : "NO WIFI", wifiReady && WiFi.status() == WL_CONNECTED);
  drawStatusPill(162, 34, 70, 22, mqttClient.connected() ? "MQTT" : "NO MQTT", mqttClient.connected());
  drawStatusPill(240, 34, 70, 22, "GALLERY", false);

  drawDashboardValueBox(10, 66, 145, 60, "Air Temp", formatFloat1(ghTempC), "degC", YELLOW);
  drawDashboardValueBox(165, 66, 145, 60, "Humidity", formatFloat1(ghHumRH), "%RH", CYAN);
  drawDashboardValueBox(10, 132, 145, 60, "Soil", formatFloat1(ghSoilMoist), "%", GREEN);
  drawDashboardValueBox(165, 132, 145, 60, "Light", formatFloat1(ghLightLux), "lux", MAGENTA);
  drawDashboardValueBox(10, 198, 145, 60, "Water Level", formatFloat1(ghWaterLevel), "%", BLUE);
  drawDashboardValueBox(165, 198, 70, 60, "pH", formatFloat1(ghPh), "pH", WHITE);
  drawDashboardValueBox(240, 198, 70, 60, "EC", formatFloat1(ghEc), "mS", WHITE);

  gfx->drawRoundRect(10, 264, 300, 52, 10, GRAY);
  gfx->setFont();
  gfx->setTextSize(1);
  gfx->setTextColor(WHITE);
  gfx->setCursor(18, 280);
  gfx->print("Mode control");
  gfx->fillRoundRect(18, 290, 88, 18, 6, ghAutoMode ? GREEN : GRAY);
  gfx->fillRoundRect(114, 290, 88, 18, 6, ghAutoMode ? GRAY : YELLOW);
  gfx->fillRoundRect(210, 290, 92, 18, 6, BLUE);
  gfx->setTextColor(BLACK);
  gfx->setCursor(46, 303); gfx->print("AUTO");
  gfx->setCursor(136, 303); gfx->print("MANUAL");
  gfx->setCursor(232, 303); gfx->print("GALLERY");

  drawDashboardIconsFromUrl();

  gfx->setTextColor(GRAY);
  gfx->setTextSize(1);
  gfx->setCursor(10, 470);
  gfx->print("Tap icons to control greenhouse / data sync via MQTT");
  gfx->flush();
}

// ======================================================================
// ฟังก์ชัน drawWelcomeScreen
// หน้าที่:
// 1) แสดงหน้าแรกก่อนเข้าสู่ Dashboard
// 2) เมื่อแตะหน้าจอจะเข้าสู่ระบบควบคุมโรงเรือนปลูกผักก่อนเสมอ
// ======================================================================

// ----------------------------------------------------------------------
// Center text horizontally inside a rectangle
// ----------------------------------------------------------------------
void drawCenteredTextInRect(const String &text, int16_t x, int16_t y, int16_t w, int16_t h) {
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  gfx->getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);
  int16_t tx = x + (w - (int16_t)tbw) / 2 - tbx;
  int16_t ty = y + (h - (int16_t)tbh) / 2 - tby;
  gfx->setCursor(tx, ty);
  gfx->print(text);
}


// ======================================================================
// Touch Debug Overlay
// วัตถุประสงค์:
// 1) วาดกรอบสี่เหลี่ยมครอบบริเวณที่แตะจริงบนหน้า Dashboard
// 2) ใช้ตรวจสอบว่าโซนสัมผัสของ icons ตรงกับตำแหน่งภาพหรือไม่
// 3) ใช้เฉพาะงานทดสอบ/คาลิเบรตหน้าจอสัมผัส
// ======================================================================
bool touchDebugEnabled = true;
int16_t lastTouchBoxX = -1;
int16_t lastTouchBoxY = -1;
int16_t lastTouchBoxW = 0;
int16_t lastTouchBoxH = 0;

void clearTouchDebugBox() {
  if (!touchDebugEnabled) return;
  if (currentScreen != SCREEN_DASHBOARD) return;
  if (lastTouchBoxX < 0 || lastTouchBoxY < 0) return;

  // วาดหน้า Dashboard ซ้ำเพื่อเคลียร์กรอบเก่าให้สะอาด
  drawDashboardScreen();
  lastTouchBoxX = -1;
  lastTouchBoxY = -1;
  lastTouchBoxW = 0;
  lastTouchBoxH = 0;
}

void drawTouchDebugBox(int16_t x, int16_t y, int16_t w, int16_t h) {
  if (!touchDebugEnabled) return;
  if (currentScreen != SCREEN_DASHBOARD) return;

  clearTouchDebugBox();

  gfx->drawRoundRect(x, y, w, h, 6, YELLOW);
  gfx->drawRoundRect(x + 1, y + 1, w - 2, h - 2, 6, RED);

  lastTouchBoxX = x;
  lastTouchBoxY = y;
  lastTouchBoxW = w;
  lastTouchBoxH = h;
}


void drawWelcomeScreen() {
  currentScreen = SCREEN_WELCOME;
  imageMode = false;

  // --------------------------------------------------------------------
  // Dark Theme palette สำหรับหน้า Welcome
  // --------------------------------------------------------------------
  const uint16_t BG_DARK      = gfx->color565(8, 12, 18);
  const uint16_t PANEL_DARK   = gfx->color565(18, 24, 32);
  const uint16_t PANEL_BORDER = gfx->color565(95, 110, 130);
  const uint16_t TITLE_CYAN   = gfx->color565(70, 215, 235);
  const uint16_t SUBTEXT      = gfx->color565(225, 230, 235);
  const uint16_t STATUS_AMBER = gfx->color565(255, 196, 70);
  const uint16_t BTN_DARK     = gfx->color565(36, 120, 72);
  const uint16_t BTN_BORDER   = gfx->color565(110, 255, 170);
  const uint16_t FOOTER_TEXT  = gfx->color565(150, 160, 175);
  const uint16_t OK_COLOR     = gfx->color565(110, 255, 170);
  const uint16_t BAD_COLOR    = gfx->color565(255, 120, 120);
  const uint16_t INFO_COLOR   = gfx->color565(120, 210, 255);

  gfx->fillScreen(BG_DARK);

  // หัวเรื่องระบบ
  gfx->setFont();
  gfx->setTextSize(2);
  gfx->setTextColor(TITLE_CYAN);
  gfx->setCursor(34, 28);
  gfx->print("SMART GREENHOUSE");

  gfx->setTextColor(SUBTEXT);
  gfx->setCursor(22, 60);
  gfx->print("Hardware Status");

  // กล่องสถานะหลัก
  gfx->fillRoundRect(4, 60, 312, 338, 12, PANEL_DARK);
  gfx->drawRoundRect(4, 60, 312, 338, 12, PANEL_BORDER);

  gfx->setTextSize(2);
  gfx->setTextColor((wifiReady && WiFi.status() == WL_CONNECTED) ? GREEN : STATUS_AMBER);
  gfx->setCursor(68, 78);
  if (wifiReady && WiFi.status() == WL_CONNECTED) {
    gfx->print("System Ready");
  } else {
    gfx->print("Waiting WiFi...");
  }

  gfx->drawFastHLine(18, 104, 284, PANEL_BORDER);

  int16_t y = 118;
  const int16_t x = 16;
  const int16_t step = 14;

  drawSystemReadyLine(x, y, "Board", "ESP32-S3 + JC3248W535", INFO_COLOR); y += step;
  drawSystemReadyLine(x, y, "Display", String(LCD_NATIVE_WIDTH) + "x" + String(LCD_NATIVE_HEIGHT) + " LCD", INFO_COLOR); y += step;
  drawSystemReadyLine(x, y, "Display Driver", g_displayReady ? "AXS15231B OK" : "AXS15231B FAIL", g_displayReady ? OK_COLOR : BAD_COLOR); y += step;
  drawSystemReadyLine(x, y, "Touch", getTouchStatusText() + " @0x3B", g_touchControllerDetected ? OK_COLOR : BAD_COLOR); y += step;
  drawSystemReadyLine(x, y, "Touch Pins", "SDA4 SCL8 RST12 INT11", INFO_COLOR); y += step;
  drawSystemReadyLine(x, y, "LCD Bus", "QSPI CS45 SCK47 D0/21 D1/48", INFO_COLOR); y += step;
  drawSystemReadyLine(x, y, "LCD Bus-2", "D2/40 D3/39", INFO_COLOR); y += step;
  drawSystemReadyLine(x, y, "CPU / PSRAM", String(getCpuFrequencyMhz()) + "MHz / " + formatBoolYesNo(psramFound()), psramFound() ? OK_COLOR : STATUS_AMBER); y += step;
  drawSystemReadyLine(x, y, "Flash / Heap", String(ESP.getFlashChipSize() / (1024 * 1024)) + "MB / " + getMemoryTextKB(ESP.getFreeHeap()), INFO_COLOR); y += step;
  drawSystemReadyLine(x, y, "WiFi SSID", String(ssid), INFO_COLOR); y += step;
  drawSystemReadyLine(x, y, "WiFi Status", getWiFiStatusText(), (wifiReady && WiFi.status() == WL_CONNECTED) ? OK_COLOR : BAD_COLOR); y += step;
  drawSystemReadyLine(x, y, "IP / RSSI", getIpText() + " / " + getRssiText(), (WiFi.status() == WL_CONNECTED) ? OK_COLOR : STATUS_AMBER); y += step;
  drawSystemReadyLine(x, y, "MAC", getMacText(), INFO_COLOR); y += step;
  drawSystemReadyLine(x, y, "MQTT Broker", String(mqttServer) + ":" + String(mqttPort), INFO_COLOR); y += step;
  drawSystemReadyLine(x, y, "MQTT Status", getMqttStatusText(), mqttClient.connected() ? OK_COLOR : BAD_COLOR); y += step;
  drawSystemReadyLine(x, y, "Mode / Image", String(ghAutoMode ? "AUTO" : "MANUAL") + " / " + (imageMode ? "IMAGE" : "UI"), ghAutoMode ? OK_COLOR : STATUS_AMBER); y += step;
  drawSystemReadyLine(x, y, "Outputs", "F:" + formatBoolOnOff(ghFanOn) + " P:" + formatBoolOnOff(ghPumpOn) + " M:" + formatBoolOnOff(ghMistOn), INFO_COLOR); y += step;
  drawSystemReadyLine(x, y, "Outputs-2", "L:" + formatBoolOnOff(ghLightOn) + " V:" + formatBoolOnOff(ghVentOn), INFO_COLOR); y += step;
  drawSystemReadyLine(x, y, "Touch", "Open Dashboard/Gallery", INFO_COLOR);

  // ปุ่มเข้าสู่ Dashboard ในโทนมืด
  gfx->fillRoundRect(35, 405, 250, 50, 12, BTN_DARK);
  gfx->drawRoundRect(35, 405, 250, 50, 12, BTN_BORDER);
  gfx->setTextSize(2);
  gfx->setTextColor(WHITE);
  drawCenteredTextInRect("ENTER DASHBOARD", 35, 405, 250, 50);

  gfx->setTextSize(1);
  gfx->setTextColor(FOOTER_TEXT);
  gfx->setCursor(12, 468);
  gfx->print("V49 2026-03-28 23:10 ICT  By Weerawat");
  gfx->flush();
}

// ======================================================================
// ฟังก์ชัน drawDashboardScreen
// หน้าที่:
// 1) วาดหน้า Dashboard สำหรับควบคุมโรงเรือนปลูกผัก
// 2) แสดงค่าเซนเซอร์หลักและปุ่มควบคุมอุปกรณ์อย่างครบถ้วน
// ======================================================================
void drawDashboardScreen() {
  currentScreen = SCREEN_DASHBOARD;
  imageMode = false;
  gfx->fillScreen(BLACK);

  // โหลดเฟรม Dashboard จาก URL ก่อน
  // 1) หากสำเร็จ จะวาดข้อมูลจริงทับเฉพาะจุด
  // 2) หากไม่สำเร็จ จะ fallback ไปใช้หน้าจอ Dashboard แบบวาดกราฟิกเดิม
  bool frameOk = drawDashboardFrameFromUrl();
  if (frameOk) {
    refreshDashboardInteractiveArea();
    gfx->flush();
    return;
  }

  // ===== fallback แบบเดิม =====
  gfx->setFont();
  gfx->setTextSize(2);
  gfx->setTextColor(CYAN);
  gfx->setCursor(10, 24);
  gfx->print("GREENHOUSE DASHBOARD");

  drawStatusPill(10, 34, 66, 22, ghAutoMode ? "AUTO" : "MANUAL", ghAutoMode);
  drawStatusPill(84, 34, 70, 22, wifiReady && WiFi.status() == WL_CONNECTED ? "WIFI" : "NO WIFI", wifiReady && WiFi.status() == WL_CONNECTED);
  drawStatusPill(162, 34, 70, 22, mqttClient.connected() ? "MQTT" : "NO MQTT", mqttClient.connected());
  drawStatusPill(240, 34, 70, 22, "GALLERY", false);

  drawDashboardValueBox(10, 66, 145, 60, "Air Temp", formatFloat1(ghTempC), "degC", YELLOW);
  drawDashboardValueBox(165, 66, 145, 60, "Humidity", formatFloat1(ghHumRH), "%RH", CYAN);
  drawDashboardValueBox(10, 132, 145, 60, "Soil", formatFloat1(ghSoilMoist), "%", GREEN);
  drawDashboardValueBox(165, 132, 145, 60, "Light", formatFloat1(ghLightLux), "lux", MAGENTA);
  drawDashboardValueBox(10, 198, 145, 60, "Water Level", formatFloat1(ghWaterLevel), "%", BLUE);
  drawDashboardValueBox(165, 198, 70, 60, "pH", formatFloat1(ghPh), "pH", WHITE);
  drawDashboardValueBox(240, 198, 70, 60, "EC", formatFloat1(ghEc), "mS", WHITE);

  gfx->drawRoundRect(10, 264, 300, 52, 10, GRAY);
  gfx->setFont();
  gfx->setTextSize(1);
  gfx->setTextColor(WHITE);
  gfx->setCursor(18, 280);
  gfx->print("Mode control");
  gfx->fillRoundRect(18, 290, 88, 18, 6, ghAutoMode ? GREEN : GRAY);
  gfx->fillRoundRect(114, 290, 88, 18, 6, ghAutoMode ? GRAY : YELLOW);
  gfx->fillRoundRect(210, 290, 92, 18, 6, BLUE);
  gfx->setTextColor(BLACK);
  gfx->setCursor(46, 303); gfx->print("AUTO");
  gfx->setCursor(136, 303); gfx->print("MANUAL");
  gfx->setCursor(232, 303); gfx->print("GALLERY");

  drawDashboardIconsFromUrl();

  gfx->setTextColor(GRAY);
  gfx->setTextSize(1);
  gfx->setCursor(10, 470);
  gfx->print("Tap icons to control greenhouse / data sync via MQTT");
  gfx->flush();
}

// ======================================================================
// ฟังก์ชัน showDashboardScreen
// หน้าที่:
// 1) แสดงหน้า Dashboard และอัปเดตข้อมูลล่าสุดบนจอ
// ======================================================================
void showDashboardScreen() {
  drawDashboardScreen();
}

// ======================================================================
// ฟังก์ชัน drawSplashScreen
// หน้าที่:
// 1) คงชื่อฟังก์ชันเดิมไว้เพื่อความเข้ากันได้กับโค้ดรุ่นก่อน
// 2) เปลี่ยนภายในให้เรียกหน้า Welcome Screen แทน
// ======================================================================
void drawSplashScreen() {
  drawWelcomeScreen();
}

// ======================================================================
// ฟังก์ชัน drawControlBar
// หน้าที่:
// 1) เวอร์ชันเต็มจอไม่ใช้แถบควบคุมด้านล่าง
// 2) คงฟังก์ชันไว้เพื่อให้โครงสร้างโปรแกรมอ่านง่าย
// ======================================================================
void drawControlBar() {
  // ไม่ต้องวาดอะไร
}

// ======================================================================
// ฟังก์ชัน drawErrorScreen
// หน้าที่:
// 1) แสดงข้อผิดพลาดระหว่างโหลด/ถอดรหัสภาพ
// ======================================================================
void drawErrorScreen(const String &msg) {
  gfx->fillRect(0, 0, 320, 480, BLACK);
  drawCenteredText("Load image failed", 210, RED, 2);

  gfx->setFont();
  gfx->setTextSize(2);
  gfx->setTextColor(WHITE);
  gfx->setCursor(10, 255);
  gfx->print(msg);
  gfx->flush();
}

// ======================================================================
// ฟังก์ชัน drawLoadingScreen
// หน้าที่:
// 1) แสดงข้อความ Loading image... ระหว่างเปลี่ยนภาพ
// 2) ใช้แจ้งสถานะการดาวน์โหลดภาพให้ผู้ใช้ทราบ
// ======================================================================
void drawLoadingScreen(const char* url) {
  // วาดพื้นหลังดำก่อน เพื่อให้ข้อความสถานะอ่านง่ายและไม่ทับภาพเดิม
  gfx->fillScreen(BLACK);

  // ใช้ฟังก์ชันที่มีอยู่จริงในโปรแกรมแทน drawCenteredMessage()
  drawCenteredText("Loading image...", 210, WHITE, 2);

  // แสดง URL แบบย่อสำหรับการตรวจสอบไฟล์ที่กำลังโหลด
  gfx->setFont();
  gfx->setTextSize(1);
  gfx->setTextColor(GRAY, BLACK);
  gfx->setCursor(8, 300);
  gfx->print("URL: ");
  if (url && *url) {
    String s = String(url);
    if (s.length() > 52) s = s.substring(0, 52) + "...";
    gfx->print(s);
  } else {
    gfx->print("-");
  }
  gfx->flush();
}
// ======================================================================
// ฟังก์ชัน configureWiFiReliability
// หน้าที่:
// 1) ตั้งค่า WiFi ให้อยู่ในโหมดสถานีอย่างเดียว
// 2) ปิดโหมดประหยัดพลังงานเพื่อลดอาการหลุดบน ESP32-S3
// 3) เปิด Auto Reconnect และล้างค่าเก่าที่ค้างจาก session เดิม
// ======================================================================
void configureWiFiReliability() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
}

// ======================================================================
// ฟังก์ชัน connectWiFi
// หน้าที่:
// 1) เชื่อมต่อ WiFi ภายในเวลาที่กำหนด
// 2) ใช้การ reconnect แบบมีช่วงพัก ลดการ begin ซ้ำถี่เกินไป
// 3) เมื่อเชื่อมต่อสำเร็จให้ต่อ MQTT ตามทันที
// ======================================================================
bool connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiReady = true;
    g_wifiDisconnectedSinceMs = 0;
    ensureMqttConnected();
    return true;
  }

  configureWiFiReliability();
  WiFi.disconnect(true, true);
  delay(200);

  gfx->fillRect(0, 470, 320, 10, BLACK);
  drawCenteredText("Connecting WiFi...", 470, YELLOW, 1);  //ข้อความ, ตำแหน่งแกนY, สีตัวอักษร, ขนาดตัวอักษร
  gfx->flush();

  g_lastWiFiConnectAttemptMs = millis();
  WiFi.begin(ssid, password);

  unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    wifiReady = true;
    g_wifiDisconnectedSinceMs = 0;
    Serial.print("WiFi connected, IP = ");
    Serial.println(WiFi.localIP());
    Serial.print("WiFi RSSI = ");
    Serial.println(WiFi.RSSI());

    ensureMqttConnected();

    gfx->fillRect(0, 420, 320, 40, BLACK);
    drawCenteredText("WiFi connected", 445, GREEN, 2);
    gfx->flush();
    delay(500);
    return true;
  }

  wifiReady = false;
  if (g_wifiDisconnectedSinceMs == 0) g_wifiDisconnectedSinceMs = millis();

  gfx->fillRect(0, 420, 320, 40, BLACK);
  drawCenteredText("WiFi failed", 445, RED, 2);
  gfx->flush();
  delay(500);
  return false;
}

// ======================================================================
// ฟังก์ชัน ensureWiFiConnected
// หน้าที่:
// 1) ใช้ใน loop() เพื่อตรวจและฟื้น WiFi อัตโนมัติ
// 2) จะ reconnect เมื่อถึงช่วงเวลาที่กำหนดหรือเมื่อบังคับให้ลองใหม่
// 3) หากหลุดค้างนาน จะตัด session เดิมแล้วเชื่อมใหม่แบบสะอาด
// ======================================================================
bool ensureWiFiConnected(bool forceRetry) {
  if (WiFi.status() == WL_CONNECTED) {
    wifiReady = true;
    g_wifiDisconnectedSinceMs = 0;
    return true;
  }

  wifiReady = false;
  unsigned long now = millis();
  if (g_wifiDisconnectedSinceMs == 0) g_wifiDisconnectedSinceMs = now;

  if (!forceRetry && (now - g_lastWiFiConnectAttemptMs < WIFI_RETRY_INTERVAL_MS)) {
    return false;
  }

  Serial.println("WiFi disconnected -> retrying");

  if (now - g_wifiDisconnectedSinceMs >= WIFI_DROP_RESTART_WAIT_MS) {
    Serial.println("WiFi drop too long -> reset WiFi session");
    WiFi.disconnect(true, true);
    delay(200);
  }

  return connectWiFi();
}

// ======================================================================
// ฟังก์ชัน maintainNetwork
// หน้าที่:
// 1) ดูแล WiFi และ MQTT ให้กลับมาเชื่อมต่อเองอัตโนมัติ
// 2) ลดปัญหา WiFi หลุดบ่อยระหว่างเปิด Dashboard/HTTP/MQTT
// ======================================================================
void maintainNetwork() {
  if (ensureWiFiConnected(false)) {
    if (!mqttClient.connected()) {
      ensureMqttConnected();
    }
    mqttClient.loop();
  }
}

// ======================================================================
// ฟังก์ชัน allocateImageBuffer
// หน้าที่:
// 1) ขอหน่วยความจำจาก PSRAM ก่อน
// 2) ถ้าไม่มี PSRAM จึง fallback ไป malloc ปกติ
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
// ฟังก์ชัน reallocateImageBuffer
// หน้าที่:
// 1) จอง buffer ใหม่ขนาดใหญ่ขึ้น
// 2) คัดลอกข้อมูลเดิม แล้วคืน buffer เดิม
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
// ฟังก์ชัน isJpegBuffer
// หน้าที่:
// 1) ตรวจว่า buffer เริ่มต้นด้วย JPEG SOI หรือไม่
// ======================================================================
bool isJpegBuffer(const uint8_t* buf, size_t len) {
  if (buf == nullptr || len < 4) return false;
  return (buf[0] == 0xFF && buf[1] == 0xD8);
}

// ======================================================================
// ฟังก์ชัน printBufferHead
// หน้าที่:
// 1) พิมพ์ byte 16 ตัวแรกของข้อมูลลง Serial เพื่อ debug
// ======================================================================
void printBufferHead(const uint8_t* buf, size_t len) {
  Serial.print("HEAD = ");
  size_t n = (len < 16) ? len : 16;
  for (size_t i = 0; i < n; i++) {
    if (buf[i] < 16) Serial.print("0");
    Serial.print(buf[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

// ======================================================================
// ฟังก์ชัน downloadImageToMemory
// หน้าที่:
// 1) ดาวน์โหลดไฟล์จาก HTTP server เก็บลงหน่วยความจำ
// 2) รองรับทั้งกรณีรู้ Content-Length และไม่รู้ล่วงหน้า
// ======================================================================
bool downloadImageToMemory(const char* url, uint8_t** outBuf, size_t* outLen) {
  *outBuf = nullptr;
  *outLen = 0;
  g_lastHttpCode = -1;
  g_lastContentType = "";

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("HTTP skipped: WiFi not connected");
    g_lastHttpFailMs = millis();
    return false;
  }

  HTTPClient http;
  WiFiClient client;
  client.setTimeout(10);

  http.setConnectTimeout(5000);
  http.setTimeout(12000);
  http.useHTTP10(true);          // บังคับใช้ HTTP/1.0 ลดปัญหา keep-alive กับ Apache รุ่นเก่า
  http.setReuse(false);          // ปิดการ reuse socket เพื่อไม่ให้ socket ค้างข้ามคำขอ

  if (!http.begin(client, url)) {
    Serial.println("HTTP begin failed");
    g_lastHttpFailMs = millis();
    return false;
  }

  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.addHeader("Accept", "image/jpeg,image/*,*/*");
  http.addHeader("User-Agent", "ESP32-JPEG-Viewer");
  http.addHeader("Connection", "close");  // ขอให้ server ปิด connection หลังส่งจบ ลด timeout ค้าง
  http.collectHeaders(nullptr, 0);

  Serial.print("HTTP GET URL = ");
  Serial.println(url);
  Serial.print("WiFi RSSI = ");
  Serial.println(WiFi.RSSI());

  delay(150);  // เว้นจังหวะสั้น ๆ หลังเช็ก WiFi ก่อนเริ่ม GET
  int httpCode = http.GET();
  g_lastHttpCode = httpCode;

  Serial.print("HTTP code = ");
  Serial.println(httpCode);

  if (httpCode <= 0) {
    Serial.print("HTTP error = ");
    Serial.println(http.errorToString(httpCode));
    g_lastHttpFailMs = millis();
    http.end();
    client.stop();
    return false;
  }

  if (httpCode != HTTP_CODE_OK) {
    g_lastHttpFailMs = millis();
    http.end();
    return false;
  }

  g_lastContentType = http.header("Content-Type");
  Serial.print("Content-Type = ");
  Serial.println(g_lastContentType);

  WiFiClient* stream = http.getStreamPtr();
  int len = http.getSize();

  if (len > 0) {
    uint8_t* buf = allocateImageBuffer((size_t)len);
    if (!buf) {
      Serial.println("Memory allocation failed");
      http.end();
      return false;
    }

    size_t totalRead = 0;
    unsigned long lastDataMs = millis();

    while (http.connected() && totalRead < (size_t)len) {
      size_t avail = stream->available();

      if (avail) {
        int n = stream->readBytes(buf + totalRead, min((size_t)avail, (size_t)(len - totalRead)));
        if (n > 0) {
          totalRead += n;
          lastDataMs = millis();
        }
      } else {
        if (millis() - lastDataMs > 5000) break;
        delay(1);
      }
    }

    if (totalRead == (size_t)len) {
      *outBuf = buf;
      *outLen = totalRead;
      http.end();
      client.stop();
      return true;
    }

    free(buf);
    http.end();
    return false;
  }

  size_t capacity = 16384;
  size_t totalRead = 0;
  uint8_t* buf = allocateImageBuffer(capacity);
  if (!buf) {
    Serial.println("Initial allocation failed");
    http.end();
    return false;
  }

  unsigned long lastDataMs = millis();

  while (http.connected() || stream->available()) {
    size_t avail = stream->available();

    if (avail) {
      if (totalRead + avail > capacity) {
        size_t newCapacity = capacity * 2;
        while (newCapacity < totalRead + avail) newCapacity *= 2;

        uint8_t* newBuf = reallocateImageBuffer(buf, newCapacity, totalRead);
        if (!newBuf) {
          free(buf);
          http.end();
          return false;
        }

        buf = newBuf;
        capacity = newCapacity;
      }

      int n = stream->readBytes(buf + totalRead, avail);
      if (n > 0) {
        totalRead += n;
        lastDataMs = millis();
      }
    } else {
      if (millis() - lastDataMs > 5000) break;
      delay(1);
    }
  }

  if (totalRead > 0) {
    *outBuf = buf;
    *outLen = totalRead;
    http.end();
    return true;
  }

  free(buf);
  http.end();
  return false;
}

// ======================================================================
// ฟังก์ชัน jpegInfo
// หน้าที่:
// 1) แสดงข้อมูลสำคัญของ JPEGDecoder ลง Serial เพื่อ debug
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
// ฟังก์ชัน drawScaledClippedBlock
// หน้าที่:
// 1) รับ block ภาพย่อยจาก JPEGDecoder (MCU)
// 2) แสดง block ตามขนาดจริง 1:1 แบบเต็มพื้นที่จอ
// 3) crop ไม่ให้วาดล้นนอกพื้นที่จอ
// 4) วาดทีละแถวเพื่อลดการใช้ RAM
// หมายเหตุ:
// JPEGDecoder ไม่มี callback วาดแบบ TJpg_Decoder
// ดังนั้นต้องอ่าน MCU แล้ววาดเองในฟังก์ชันนี้
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
// ฟังก์ชัน renderJPEGFromDecoder
// หน้าที่:
// 1) อ่าน MCU จาก JPEGDecoder ทีละ block
// 2) จัดการ block ขอบภาพด้านขวา/ล่างที่อาจเล็กกว่าปกติ
// 3) ส่ง block ไปวาดผ่าน drawScaledClippedBlock()
// ======================================================================
bool renderJPEGFromDecoder() {
  uint16_t mcuW = JpegDec.MCUWidth;
  uint16_t mcuH = JpegDec.MCUHeight;
  uint32_t imgW = JpegDec.width;
  uint32_t imgH = JpegDec.height;

  if (mcuW == 0 || mcuH == 0 || imgW == 0 || imgH == 0) return false;

  uint16_t minW = (imgW % mcuW == 0) ? mcuW : (imgW % mcuW);
  uint16_t minH = (imgH % mcuH == 0) ? mcuH : (imgH % mcuH);

  unsigned long drawStart = millis();

  while (JPEG_USE_SWAPPED_BYTES ? JpegDec.readSwappedBytes() : JpegDec.read()) {
    uint16_t *pImg = JpegDec.pImage;
    if (pImg == nullptr) return false;

    int32_t mcuX = JpegDec.MCUx * mcuW;
    int32_t mcuY = JpegDec.MCUy * mcuH;

    uint16_t blockW = ((mcuX + mcuW) <= (int32_t)imgW) ? mcuW : minW;
    uint16_t blockH = ((mcuY + mcuH) <= (int32_t)imgH) ? mcuH : minH;

    drawScaledClippedBlock(mcuX, mcuY, blockW, blockH, pImg);
  }

  Serial.print("JPEG_USE_SWAPPED_BYTES = ");
  Serial.println(JPEG_USE_SWAPPED_BYTES ? "true" : "false");

  Serial.print("Render time = ");
  Serial.print(millis() - drawStart);
  Serial.println(" ms");
  return true;
}

// ======================================================================
// ฟังก์ชัน drawJpegFromUrlAt
// หน้าที่:
// 1) ดาวน์โหลดภาพ JPEG จาก URL
// 2) ถอดรหัสด้วย JPEGDecoder
// 3) วาดภาพทับลงบนภาพเดิมตามพิกัดที่กำหนด
// 4) ไม่ลบภาพเดิมก่อนวาด
// หมายเหตุ:
// 1) ถ้าพิกัดอยู่นอกขอบจอ ภาพอาจถูก crop หรือไม่ปรากฏเลย
// 2) ค่าที่ผู้ใช้กำหนดคือ (430, 0) ซึ่งเกินความกว้างจอ 320 พิกเซล
// ======================================================================
bool drawJpegFromUrlAt(const char* url, int16_t posX, int16_t posY) {
  uint8_t* jpgBuf = nullptr;
  size_t jpgLen = 0;

  Serial.print("Overlay URL: ");
  Serial.println(url);

  if (!downloadImageToMemory(url, &jpgBuf, &jpgLen)) {
    Serial.println("Overlay download failed");
    return false;
  }

  if (!isJpegBuffer(jpgBuf, jpgLen)) {
    Serial.println("Overlay is not JPEG");
    free(jpgBuf);
    return false;
  }

  int rc = JpegDec.decodeArray(jpgBuf, jpgLen);
  if (rc == 0) {
    Serial.println("Overlay decode start failed");
    free(jpgBuf);
    return false;
  }

  jpgScale = 1.0f;
  jpgDrawX = posX;
  jpgDrawY = posY;
  jpgClipX1 = 0;
  jpgClipY1 = 0;
  jpgClipX2 = gfx->width() - 1;
  jpgClipY2 = gfx->height() - 1;

  Serial.print("Overlay draw at X=");
  Serial.print(jpgDrawX);
  Serial.print(" Y=");
  Serial.println(jpgDrawY);

  bool ok = renderJPEGFromDecoder();
  free(jpgBuf);
  return ok;
}

// ======================================================================
// ฟังก์ชัน showCurrentImage
// หน้าที่:
// 1) โหลดภาพปัจจุบันจาก URL
// 2) ตรวจข้อมูลเบื้องต้น
// 3) ถอดรหัสด้วย JPEGDecoder
// 4) คำนวณตำแหน่งแสดงผลแบบเต็มจอ โดยยึดขนาดจริงของภาพ
// 5) วาดภาพลงจอ
// ======================================================================
void showCurrentImage() {
  currentScreen = SCREEN_IMAGE;
  imageMode = true;
  g_lastGallerySlideMs = millis();
  if (!wifiReady) {
    wifiReady = connectWiFi();
    if (!wifiReady) {
      drawErrorScreen("WiFi not connected");
      return;
    }
  }

  ensureSlideshowListAvailable(false);

  int imageCount = getGalleryImageCount();
  if (imageCount <= 0) {
    drawErrorScreen("No slideshow image");
    return;
  }

  if (currentImageIndex < 0) currentImageIndex = 0;
  if (currentImageIndex >= imageCount) currentImageIndex = 0;

  String imageUrlString = getGalleryImageUrlByIndex(currentImageIndex);
  String imageNameString = getGalleryImageNameByIndex(currentImageIndex);
  const char* url = imageUrlString.c_str();

  Serial.println();
  Serial.print("Loading image: ");
  Serial.println(imageNameString);
  Serial.print("Loading URL: ");
  Serial.println(url);

  drawLoadingScreen(url);

  uint8_t* jpgBuf = nullptr;
  size_t jpgLen = 0;

  if (!downloadImageToMemory(url, &jpgBuf, &jpgLen)) {
    drawErrorScreen("HTTP error");
    return;
  }

  Serial.print("Downloaded bytes = ");
  Serial.println(jpgLen);
  printBufferHead(jpgBuf, jpgLen);

  if (!isJpegBuffer(jpgBuf, jpgLen)) {
    free(jpgBuf);
    drawErrorScreen("Not JPEG data");
    return;
  }

  int rc = JpegDec.decodeArray(jpgBuf, jpgLen);
  if (rc == 0) {
    free(jpgBuf);
    drawErrorScreen("Decode start failed");
    return;
  }

  jpegInfo();

  uint16_t jpgW = JpegDec.width;
  uint16_t jpgH = JpegDec.height;

  if (jpgW == 0 || jpgH == 0) {
    free(jpgBuf);
    drawErrorScreen("Invalid JPEG");
    return;
  }

  // แสดงภาพเต็มจอโดยไม่มีกรอบและไม่มีแถบคำสั่ง
  // ใช้ขนาดจริงของภาพเป็นหลัก: ภาพเล็กจัดกึ่งกลาง ภาพใหญ่ crop เฉพาะส่วนเกิน
  jpgScale = 1.0f;

  jpgDrawX = IMAGE_AREA_X + ((int32_t)IMAGE_AREA_W - (int32_t)jpgW) / 2;
  jpgDrawY = IMAGE_AREA_Y + ((int32_t)IMAGE_AREA_H - (int32_t)jpgH) / 2;

  jpgClipX1 = IMAGE_AREA_X;
  jpgClipY1 = IMAGE_AREA_Y;
  jpgClipX2 = IMAGE_AREA_X + IMAGE_AREA_W - 1;
  jpgClipY2 = IMAGE_AREA_Y + IMAGE_AREA_H - 1;

  gfx->fillRect(IMAGE_AREA_X, IMAGE_AREA_Y, IMAGE_AREA_W, IMAGE_AREA_H, BLACK);

  Serial.print("Draw at X=");
  Serial.print(jpgDrawX);
  Serial.print(" Y=");
  Serial.print(jpgDrawY);
  Serial.print(" scale=");
  Serial.println(jpgScale, 4);

  bool ok = renderJPEGFromDecoder();
  free(jpgBuf);

  if (!ok) {
    drawErrorScreen("JPEG render failed");
    return;
  }

  // วาดภาพ MainMenu.jpg ทับลงบนภาพเดิมทันที โดยไม่ลบพื้นหลังเดิมก่อน
  // หมายเหตุ: พิกัด (430, 0) อยู่นอกขอบจอด้านขวา จึงอาจไม่เห็นภาพบนจอจริง
  char overlayMenuUrl[192];
  buildFullUrl(overlayMenuFile, overlayMenuUrl, sizeof(overlayMenuUrl));
  bool overlayOk = drawJpegFromUrlAt(overlayMenuUrl, OVERLAY_MENU_X, OVERLAY_MENU_Y);
  if (!overlayOk) {
    Serial.println("Overlay draw failed or overlay is outside visible area");
  }

  gfx->flush();
}

// ======================================================================
// ฟังก์ชัน showStartHomeScreen
// หน้าที่:
// 1) กลับไปแสดงหน้าแรกเหมือนตอนเริ่มระบบ
// 2) ใช้หน้า Dashboard แสดงข้อมูลระบบเหมือนตอนเริ่มระบบ
// 3) ไม่เปลี่ยนสถานะ WiFi ที่เชื่อมต่อไว้แล้ว
// ======================================================================
void showStartHomeScreen() {
  imageMode = false;
  drawWelcomeScreen();
}
// ======================================================================
// ฟังก์ชัน showNextImage
// หน้าที่:
// 1) เลื่อนไปภาพถัดไปแบบวนรอบ
// ======================================================================
void showNextImage() {
  ensureSlideshowListAvailable(false);
  int imageCount = getGalleryImageCount();
  if (imageCount <= 0) return;

  currentImageIndex++;
  if (currentImageIndex >= imageCount) currentImageIndex = 0;
  showCurrentImage();
}

// ======================================================================
// ฟังก์ชัน showPrevImage
// หน้าที่:
// 1) เลื่อนไปภาพก่อนหน้าแบบวนรอบ
// ======================================================================
void showPrevImage() {
  ensureSlideshowListAvailable(false);
  int imageCount = getGalleryImageCount();
  if (imageCount <= 0) return;

  currentImageIndex--;
  if (currentImageIndex < 0) currentImageIndex = imageCount - 1;
  showCurrentImage();
}

// ======================================================================
// ฟังก์ชัน markGalleryInteraction
// หน้าที่:
// 1) บันทึกเวลาที่ผู้ใช้มีการแตะ/เปลี่ยนภาพใน Gallery
// 2) ใช้รีเซ็ตตัวจับเวลา Slide Show
// ======================================================================
void markGalleryInteraction() {
  g_lastGalleryInteractionMs = millis();
  g_lastGallerySlideMs = g_lastGalleryInteractionMs;
}

// ======================================================================
// ฟังก์ชัน playGalleryTransitionEffect
// หน้าที่:
// 1) แสดงเอฟเฟกต์ก่อนเปลี่ยนภาพในโหมด Slide Show
// 2) ใช้แถบสีดำปิดจากขอบเข้าหากึ่งกลาง สร้างอารมณ์แบบ Curtain/Wipe
// หมายเหตุ:
// 1) ใช้เวลาสั้น เพื่อไม่ให้รู้สึกหน่วงระหว่างเปลี่ยนภาพ
// ======================================================================
void playGalleryTransitionEffect() {
  if (currentScreen != SCREEN_IMAGE) return;

  int16_t halfW = IMAGE_AREA_W / 2;
  int16_t stepW = halfW / GALLERY_EFFECT_STEPS;
  if (stepW < 1) stepW = 1;

  for (uint8_t i = 0; i < GALLERY_EFFECT_STEPS; i++) {
    int16_t coverW = (i + 1) * stepW;
    if (coverW > halfW) coverW = halfW;

    gfx->fillRect(IMAGE_AREA_X, IMAGE_AREA_Y, coverW, IMAGE_AREA_H, BLACK);
    gfx->fillRect(IMAGE_AREA_X + IMAGE_AREA_W - coverW, IMAGE_AREA_Y, coverW, IMAGE_AREA_H, BLACK);
    gfx->flush();
    delay(GALLERY_EFFECT_STEP_DELAY_MS);
  }
}

// ======================================================================
// ฟังก์ชัน updateGallerySlideshow
// หน้าที่:
// 1) ตรวจเวลาในหน้า Gallery
// 2) ถ้าไม่มีการแตะเปลี่ยนภาพเกินเวลาที่กำหนด ให้เลื่อนไปภาพถัดไปอัตโนมัติ
// 3) เล่นเอฟเฟกต์ก่อนโหลดภาพถัดไป
// ======================================================================
void updateGallerySlideshow() {
  if (currentScreen != SCREEN_IMAGE) return;

  unsigned long now = millis();

  if (now - g_lastGalleryInteractionMs < GALLERY_USER_IDLE_DELAY_MS) return;
  if (now - g_lastGallerySlideMs < GALLERY_SLIDESHOW_INTERVAL_MS) return;

  if ((now - g_lastSlideshowFetchMs) >= SLIDESHOW_LIST_REFRESH_MS) {
    fetchSlideshowListFromApi(true);
  }

  playGalleryTransitionEffect();
  showNextImage();
}

// ======================================================================
// ฟังก์ชัน isTouchInBottomIconBar
// หน้าที่:
// 1) ตรวจว่าตำแหน่งสัมผัสอยู่ในแถบไอคอนด้านล่างหรือไม่
// ======================================================================
bool isTouchInBottomIconBar(uint16_t x, uint16_t y) {
  return (x >= ICON_BAR_X && x < (ICON_BAR_X + ICON_BAR_W) &&
          y >= ICON_BAR_Y && y < (ICON_BAR_Y + ICON_BAR_H));
}

// ======================================================================
// ฟังก์ชัน handleBottomIconTouch
// หน้าที่:
// 1) แยกโซนสัมผัสให้ตรงกับตำแหน่งไอคอนที่ถูกวางทับไว้ด้านล่าง
// 2) ย้อนกลับ = ภาพก่อนหน้า
// 3) ถัดไป   = ภาพถัดไป
// 4) ตั้งค่า, Home, ค้นหา = พิมพ์ log ไว้ก่อน เพื่อให้ต่อยอดคำสั่งได้ภายหลัง
// ======================================================================
void handleBottomIconTouch(uint16_t x, uint16_t y) {
  if (!isTouchInBottomIconBar(x, y)) return;

  Serial.print("Bottom icon touch X=");
  Serial.print(x);
  Serial.print(" Y=");
  Serial.println(y);

  if (x >= ICON_PREV_X1 && x <= ICON_PREV_X2) {
    Serial.println("Icon: PREV");
    markGalleryInteraction();
    showPrevImage();
    return;
  }

  if (x >= ICON_SETTINGS_X1 && x <= ICON_SETTINGS_X2) {
    Serial.println("Icon: SETTINGS -> DASHBOARD");
    showDashboardScreen();
    return;
  }

  if (x >= ICON_HOME_X1 && x <= ICON_HOME_X2) {
    Serial.println("Icon: HOME -> DASHBOARD");
    showDashboardScreen();
    return;
  }

  if (x >= ICON_SEARCH_X1 && x <= ICON_SEARCH_X2) {
    Serial.println("Icon: SEARCH -> WELCOME");
    showStartHomeScreen();
    return;
  }

  if (x >= ICON_NEXT_X1 && x <= ICON_NEXT_X2) {
    Serial.println("Icon: NEXT");
    markGalleryInteraction();
    showNextImage();
    return;
  }
}

// ======================================================================
// ฟังก์ชัน handleTouchPressed
// หน้าที่:
// 1) ตรวจตำแหน่งแตะเพื่อสั่งเปลี่ยนโหมดหรือเปลี่ยนภาพแบบเต็มจอ
// ======================================================================
void handleTouchPressed(uint16_t x, uint16_t y) {
  Serial.print("Pressed X=");
  Serial.print(x);
  Serial.print(" Y=");
  Serial.println(y);

  publishTouchPoint(x, y);

  if (currentScreen == SCREEN_WELCOME) {
    showDashboardScreen();
    publishControlState("touch_enter_dashboard");
    return;
  }

  if (currentScreen == SCREEN_DASHBOARD) {
    // โหมด AUTO / MANUAL
    // ขยายพื้นที่สัมผัสและแยกออกจากแถวไอคอนด้านล่าง
    if (y >= DASH_MODE_TOUCH_Y1 && y <= DASH_MODE_TOUCH_Y2) {
      if (x >= DASH_AUTO_TOUCH_X1 && x <= DASH_AUTO_TOUCH_X2) {
        drawTouchDebugBox(DASH_AUTO_TOUCH_X1, DASH_MODE_TOUCH_Y1, DASH_AUTO_TOUCH_X2 - DASH_AUTO_TOUCH_X1, DASH_MODE_TOUCH_Y2 - DASH_MODE_TOUCH_Y1);
        ghAutoMode = true;
        requestDashboardRefresh();
        publishControlState("touch_auto");
        return;
      }
      if (x >= DASH_MANUAL_TOUCH_X1 && x <= DASH_MANUAL_TOUCH_X2) {
        drawTouchDebugBox(DASH_MANUAL_TOUCH_X1, DASH_MODE_TOUCH_Y1, DASH_MANUAL_TOUCH_X2 - DASH_MANUAL_TOUCH_X1, DASH_MODE_TOUCH_Y2 - DASH_MODE_TOUCH_Y1);
        ghAutoMode = false;
        requestDashboardRefresh();
        publishControlState("touch_manual");
        return;
      }
    }

    // แถวไอคอนควบคุมด้านล่าง 5 ปุ่ม
    // ปรับช่วง Y ให้ตรงกับตำแหน่งไอคอนจริงบน Dashboard Dark Theme
    if (y >= DASH_ICON_TOUCH_Y1 && y <= DASH_ICON_TOUCH_Y2) {
      if (x >= DASH_FAN_TOUCH_X1 && x <= DASH_FAN_TOUCH_X2) {
        drawTouchDebugBox(DASH_FAN_TOUCH_X1, DASH_ICON_TOUCH_Y1, DASH_FAN_TOUCH_X2 - DASH_FAN_TOUCH_X1, DASH_ICON_TOUCH_Y2 - DASH_ICON_TOUCH_Y1);
        ghFanOn = !ghFanOn;
        requestDashboardRefresh();
        publishControlState("touch_fan");
        return;
      }
      if (x >= DASH_PUMP_TOUCH_X1 && x <= DASH_PUMP_TOUCH_X2) {
        drawTouchDebugBox(DASH_PUMP_TOUCH_X1, DASH_ICON_TOUCH_Y1, DASH_PUMP_TOUCH_X2 - DASH_PUMP_TOUCH_X1, DASH_ICON_TOUCH_Y2 - DASH_ICON_TOUCH_Y1);
        ghPumpOn = !ghPumpOn;
        requestDashboardRefresh();
        publishControlState("touch_pump");
        return;
      }
      if (x >= DASH_MIST_TOUCH_X1 && x <= DASH_MIST_TOUCH_X2) {
        drawTouchDebugBox(DASH_MIST_TOUCH_X1, DASH_ICON_TOUCH_Y1, DASH_MIST_TOUCH_X2 - DASH_MIST_TOUCH_X1, DASH_ICON_TOUCH_Y2 - DASH_ICON_TOUCH_Y1);
        ghMistOn = !ghMistOn;
        requestDashboardRefresh();
        publishControlState("touch_mist");
        return;
      }
      if (x >= DASH_LIGHT_TOUCH_X1 && x <= DASH_LIGHT_TOUCH_X2) {
        drawTouchDebugBox(DASH_LIGHT_TOUCH_X1, DASH_ICON_TOUCH_Y1, DASH_LIGHT_TOUCH_X2 - DASH_LIGHT_TOUCH_X1, DASH_ICON_TOUCH_Y2 - DASH_ICON_TOUCH_Y1);
        ghLightOn = !ghLightOn;
        requestDashboardRefresh();
        publishControlState("touch_light");
        return;
      }
      if (x >= DASH_GALLERY_TOUCH_X1 && x <= DASH_GALLERY_TOUCH_X2) {
        drawTouchDebugBox(DASH_GALLERY_TOUCH_X1, DASH_ICON_TOUCH_Y1, DASH_GALLERY_TOUCH_X2 - DASH_GALLERY_TOUCH_X1, DASH_ICON_TOUCH_Y2 - DASH_ICON_TOUCH_Y1);
        currentImageIndex = 0;
        markGalleryInteraction();
        showCurrentImage();
        publishControlState("touch_gallery");
        return;
      }
    }

    return;
  }

  if (currentScreen == SCREEN_IMAGE) {
    // เมื่อลงไอคอนเมนูไว้ที่ตำแหน่ง (0,430)
    // ให้ใช้โซนสัมผัสด้านล่างเป็นลำดับแรก เพื่อให้ตำแหน่งสัมผัสตรงกับภาพไอคอน
    if (isTouchInBottomIconBar(x, y)) {
      handleBottomIconTouch(x, y);
      return;
    }

    // คงการแตะขอบซ้าย/ขวาไว้เป็นทางเลือกสำรองสำหรับเปลี่ยนภาพ
    if (x < PREV_TOUCH_X_MAX) {
      markGalleryInteraction();
      showPrevImage();
      return;
    }

    if (x > NEXT_TOUCH_X_MIN) {
      markGalleryInteraction();
      showNextImage();
      return;
    }

    // แตะกลางจอเพื่อกลับ Dashboard
    showDashboardScreen();
    publishControlState("touch_back_dashboard");
  }
}

// ======================================================================
// ฟังก์ชัน setup
// หน้าที่:
// 1) เริ่มต้น Serial, LCD, backlight, touch และ splash screen
// 2) เชื่อมต่อ WiFi ทันทีหลังเริ่มต้นระบบ
// ======================================================================
void setup() {
  setCpuFrequencyMhz(240);

  Serial.begin(115200);
  delay(100);
  Serial.println("Program start");

  if (!gfx->begin()) {
    g_displayReady = false;
    Serial.println("gfx->begin() failed!");
    while (1) {
      delay(100);
    }
  } else {
    g_displayReady = true;
    Serial.println("gfx->begin() OK");
  }

  if (GFX_BL != GFX_NOT_DEFINED) {
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, HIGH);
  }

  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  Wire.setClock(TOUCH_I2C_CLOCK);

  pinMode(TOUCH_INT_PIN, INPUT_PULLUP);
  pinMode(TOUCH_RST_PIN, OUTPUT);

  digitalWrite(TOUCH_RST_PIN, LOW);
  delay(100);
  digitalWrite(TOUCH_RST_PIN, HIGH);
  delay(100);

  g_touchControllerDetected = probeI2CDevice(TOUCH_ADDR);
  Serial.print("Touch controller @0x3B = ");
  Serial.println(g_touchControllerDetected ? "DETECTED" : "NOT FOUND");

  gfx->setRotation(SCREEN_ROTATION);

  Serial.print("Width  = ");
  Serial.println(gfx->width());
  Serial.print("Height = ");
  Serial.println(gfx->height());

  // แสดงหน้ากำลังเริ่มต้นระบบก่อนเชื่อมต่อ WiFi
  // หมายเหตุ:
  // 1) ยังไม่แสดงข้อความ System Ready ในช่วงนี้
  // 2) จะไปแสดงหน้า Welcome ที่มีข้อความ System Ready หลัง WiFi พร้อมแล้วเท่านั้น
  gfx->fillScreen(BLACK);
  drawCenteredText("System Booting...", 210, WHITE, 2);
  drawCenteredText("Connecting WiFi...", 245, YELLOW, 2);
  gfx->flush();

  // ตั้งค่าความเสถียรของเครือข่ายก่อนเริ่มเชื่อมต่อจริง
  configureWiFiReliability();
  mqttClient.setKeepAlive(30);
  mqttClient.setSocketTimeout(5);

  // เชื่อมต่อ WiFi ทันทีเมื่อเริ่มโปรแกรม
  // ข้อดี:
  // 1) ลดเวลารอเมื่อผู้ใช้แตะเข้าหน้าแสดงภาพ
  // 2) ทำให้ทราบสถานะเครือข่ายตั้งแต่เริ่มต้น
  wifiReady = connectWiFi();

  // แสดงหน้า Welcome หลังขั้นตอนเชื่อมต่อ WiFi เสร็จสิ้น
  // เงื่อนไข:
  // 1) ถ้าเชื่อมต่อสำเร็จ จะเห็นข้อความ System Ready
  // 2) ถ้ายังไม่สำเร็จ หน้า Welcome จะขึ้น Waiting WiFi... แทน
  drawSplashScreen();
}

// ======================================================================
// ฟังก์ชัน loop
// หน้าที่:
// 1) ตรวจการแตะจอแบบ edge trigger เพื่อไม่ให้สั่งซ้ำตอนกดค้าง
// ======================================================================
void loop() {
  maintainNetwork();

  if (g_dashboardRefreshPending) {
    if (millis() - g_lastDashboardRefreshMs >= 80) {
      g_dashboardRefreshPending = false;
      g_lastDashboardRefreshMs = millis();
      refreshDashboardInteractiveArea();
    }
  }

  bool touched = getTouchPoint(touchX, touchY);

  if (touched && !lastTouchState) {
    handleTouchPressed(touchX, touchY);
  }

  lastTouchState = touched;

  updateGallerySlideshow();
  delay(20);
}