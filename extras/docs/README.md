https://github.com/wpoosommai/JC3248W535.git
# IoTAGIAppV49

ESP32-S3 HMI / Dashboard application for a 3.5-inch 320×480 touch LCD, designed for greenhouse monitoring and control.

This project provides a touchscreen user interface for displaying agricultural sensor data, controlling outputs via MQTT, loading JPEG images from a local server, and running a gallery slideshow with transition effects.

---

1) Overview

`IoTAGIAppV49.ino` is a touchscreen dashboard application built for the ESP32-S3 + JC3248W535 platform.

Main functions:
- Welcome screen and System Ready status page
- Greenhouse dashboard for sensor visualization
- Touch control for AUTO / MANUAL modes
- Output control for FAN / PUMP / MIST / LIGHT / VENT
- MQTT command and status exchange using JSON
- JPEG image viewer from HTTP URL
- Gallery slideshow with automatic playback and transition effects
- Hardware summary page showing system status in one screen

---

2) Hardware Summary

#Main board
- ESP32-S3 + JC3248W535

#Display
- 3.5-inch TFT LCD
- Resolution: 320 × 480
- Driver: AXS15231B
- Graphics library: Arduino_GFX
- Canvas buffering: Arduino_Canvas

#LCD QSPI bus pins
- `CS  = GPIO45`
- `SCK = GPIO47`
- `D0  = GPIO21`
- `D1  = GPIO48`
- `D2  = GPIO40`
- `D3  = GPIO39`

#Touch controller
- I2C address: 0x3B
- `SDA = GPIO4`
- `SCL = GPIO8`
- `RST = GPIO12`
- `INT = GPIO11`
- I2C clock: 400000 Hz

#Backlight
- `GFX_BL = GPIO1`

#Runtime hardware information shown on System Ready screen
- Board / display information
- Touch controller detection
- CPU frequency
- PSRAM status
- Flash size
- Free heap
- Wi-Fi status, IP, RSSI, MAC
- MQTT connection status

---

3) Features

#UI / HMI
- Dark theme welcome screen
- Dedicated System Ready page for hardware and network status
- Dashboard with sensor values and output states
- Touch zones for mode selection and icon control
- Gallery mode with slideshow and transition effect

#Network
- Wi-Fi auto reconnect and stability handling
- MQTT subscribe / publish support
- HTTP JPEG image loading from local server
- Slideshow image list loaded dynamically from API

#Dashboard functions
- View sensor values:
  - Air temperature
  - External temperature
  - Humidity
  - Soil moisture
  - Light level (Lux)
  - Water level
  - pH
  - EC
- Control outputs:
  - FAN
  - PUMP
  - MIST
  - LIGHT
  - VENT
- Switch between:
  - `AUTO`
  - `MANUAL`

#Image functions
- Load JPEG from URL
- Center small images on screen
- Crop oversized images to fit display
- Draw overlay image after main image load
- Run slideshow from API response
- Fallback to internal image filename list when API fails

---

4) Libraries Used

Required Arduino libraries:
- `WiFi`
- `WiFiClient`
- `HTTPClient`
- `PubSubClient`
- `Arduino_GFX_Library`
- `Wire`
- `JPEGDecoder`
- `math.h`

Additional project files:
- `Roboto_Regular16pt7b.h`
- `icons_rgb565_V1.h`

---

5) MQTT Topics

#Subscribe
- `IoTAGIApp/Command`
- `AGRI/#`

#Publish
- `IoTAGIApp/Status`
- `IoTAGIApp/Touch`

---

6) JSON Command Examples

#Screen navigation
```json
{"cmd":"home"}
```

```json
{"cmd":"dashboard"}
```

```json
{"cmd":"gallery"}
```

#Gallery control
```json
{"cmd":"next"}
```

```json
{"cmd":"prev"}
```

```json
{"cmd":"show","index":0}
```

```json
{"cmd":"show","file":"TADSANEE8.jpg"}
```

#Slideshow control
```json
{"cmd":"slideshow","state":"on","interval":10000}
```

```json
{"cmd":"slideshow","state":"off"}
```

```json
{"cmd":"refreshSlideshow"}
```

#Output control
```json
{"cmd":"fan","state":"on"}
```

```json
{"cmd":"pump","state":"off"}
```

```json
{"cmd":"mist","state":"on"}
```

```json
{"cmd":"light","state":"off"}
```

```json
{"cmd":"mode","state":"auto"}
```

#Example status payload
```json
{"temp":27.5,"hum":68.2,"soil":55.0}
```

---

7) Default Network Configuration

#Wi-Fi
```cpp
const char* ssid     = "SmartAgritronics";
const char* password = "99999999";
```

#MQTT Broker
```cpp
const char* mqttServer = "202.29.231.205";
const uint16_t mqttPort = 1883;
```

#Base image URL
```cpp
char baseUrl[128] = "http://192.168.1.200/IoTServer/";
```

#Slideshow API
```cpp
char slideshowApiUrl[160] = "http://192.168.1.201/IoTServer/slideshow_list_api.php";
```

#Dashboard background image
```text
http://192.168.1.201/IoTServer/dashboard_frame.jpg
```

---

8) Build Notes

Recommended environment:
- Arduino IDE 2.x
- ESP32 board package for ESP32-S3
- Proper PSRAM-enabled board configuration

Important notes:
- The project uses JPEGDecoder instead of TJpg_Decoder
- If JPEG colors appear incorrect, check byte order handling
- Touch controller must respond on I2C address 0x3B
- Local HTTP server paths must be reachable from the ESP32-S3 network

---

9) System Ready Screen

The System Ready screen is intended to summarize all important hardware and runtime status in a single page before entering the dashboard.

Displayed information includes:
- Board type
- Display type and resolution
- Touch controller status
- CPU / PSRAM
- Flash / Heap
- Wi-Fi SSID / IP / RSSI / MAC
- MQTT broker and connection state
- System mode and output states

This is useful for:
- Initial hardware verification
- Troubleshooting before normal operation
- Classroom demonstration and embedded systems teaching

---

10) Project Structure

Example structure:

```text
IoTAGIAppV49/
├── IoTAGIAppV49.ino
├── Roboto_Regular16pt7b.h
├── icons_rgb565_V1.h
└── README.md
```

---

11) Typical Use Case

This project is suitable for:
- Smart greenhouse dashboards
- Agricultural IoT HMI panels
- ESP32-S3 touch display projects
- MQTT-based control panels
- Embedded systems teaching demonstrations
- Research and instructional prototypes

---

12) Troubleshooting

#Wi-Fi connects poorly
- Check SSID and password
- Verify signal strength at installation point
- Confirm access point and local server are on the same reachable network

#JPEG overlay or dashboard frame cannot load
- Verify HTTP URL is correct
- Test image URL from phone or browser on the same network
- Confirm ESP32 can reach the server IP address

#Touch does not respond
- Check I2C wiring and address `0x3B`
- Confirm reset and interrupt pins are correct
- Verify touch raw coordinate mapping if display rotation changes

#MQTT commands do not work
- Confirm broker IP and port
- Check topic names exactly
- Verify JSON format is valid

---

13) Version

- Program: `IoTAGIAppV49.ino`
- Version: `V49`
- Date: `2026-03-28 23:10 ICT`

---

14) License

Add your preferred license here, for example:
- MIT License
- Apache-2.0
- Proprietary / Internal Use Only

---

15) Author

Prepared for ESP32-S3 greenhouse dashboard development, classroom use, research demonstration, and embedded systems learning.
