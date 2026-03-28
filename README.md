# JC3248W535 Arduino IDE Library

ไลบรารีนี้จัดโครงสร้างใหม่จากโปรเจกต์เดิมเพื่อให้ติดตั้งใน Arduino IDE ได้สะดวกขึ้น โดยรวมไฟล์สำคัญไว้ในรูปแบบมาตรฐานของ Arduino library

## โครงสร้างภายใน
- `src/JC3248W535.h` จุดรวม include หลัก
- `src/JC3248W535_BoardConfig.h` ค่าคงที่ของบอร์ดและพิน
- `src/icons_rgb565_V1.h` ไอคอน RGB565 แบบ PROGMEM
- `src/Roboto_Regular16pt7b.h` และ `src/ShortBaby_Mg2w16pt7b.h` ฟอนต์สำหรับจอ TFT
- `examples/IoTAGIAppV50` ตัวอย่างโปรแกรมหลักที่ย้ายมาอยู่ใน library
- `examples/BasicLibraryInfo` ตัวอย่างทดสอบการติดตั้ง library
- `extras/` เอกสารและภาพประกอบจากโปรเจกต์เดิม

## วิธีติดตั้ง
1. ปิด Arduino IDE ถ้าเปิดอยู่
2. แตกไฟล์ zip นี้ไปไว้ที่โฟลเดอร์ `Documents/Arduino/libraries/`
3. ตรวจสอบให้ได้โครงสร้างดังนี้
   `Documents/Arduino/libraries/JC3248W535_Library_V1/`
4. เปิด Arduino IDE ใหม่
5. ไปที่ **File > Examples > JC3248W535**

## วิธีเรียกใช้
```cpp
#include <JC3248W535.h>
```

## หมายเหตุ
- ตัวอย่าง `IoTAGIAppV50` ยังคงโครงสร้างการทำงานของโปรเจกต์เดิม และปรับ include ให้เหมาะกับรูปแบบ library
- โฟลเดอร์ `extras` ไม่ถูก compile โดย Arduino IDE แต่เก็บไว้เป็นเอกสารอ้างอิงและ asset เดิม


## Update V1.3.0
- เพิ่มไฟล์ `JC3248W535_Colors.h`
- เพิ่ม alias สีมาตรฐาน เช่น `RED`, `GREEN`, `BLUE`, `WHITE`, `BLACK`
- แก้ตัวอย่าง `TouchPointViewer` ให้ใช้งานร่วมกับชุดสีมาตรฐานใน library


## V1.4.0
- Added display helper functions to prevent duplicate QSPI begin calls.
- Updated TouchPointViewer to initialize Canvas only once.
