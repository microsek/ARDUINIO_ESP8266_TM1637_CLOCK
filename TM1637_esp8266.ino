/*
  EDIT code by AI cload
  นาฬิกา NTP ด้วย ESP8266 NodeMCU + TM1637
  - Board : NodeMCU 1.0 (ESP-12E Module)
  - Library: TM1637 (by Avishay Orpaz)

  การต่อสาย
    TM1637 CLK -> D5
    TM1637 DIO -> D7
    TM1637 VCC -> 3V3 (หรือ VIN/5V)
    TM1637 GND -> GND
    สวิตช์ 12/24 ชม. -> ต่อระหว่าง D1 กับ GND
        (ต่อลง GND = โหมด 12 ชม. / ปล่อยเปิด = โหมด 24 ชม.)
*/

#include <ESP8266WiFi.h>
#include <time.h>
#include <TM1637Display.h>

// ---------- WiFi ----------
#define WIFI_SSID     "yourssid"
#define WIFI_PASSWORD "yourpass"

// ---------- เวลา (ไทย UTC+7 ไม่มี DST) ----------
const long TZ_OFFSET  = 7 * 3600;
const int  DST_OFFSET = 0;
const time_t MIN_VALID_TIME = 1700000000;   // ก่อนค่านี้ = ยังไม่ได้ซิงค์ NTP

// ---------- ขา ----------
#define CLK       D5
#define DIO       D7
#define MODE_PIN  D1

TM1637Display display(CLK, DIO);

// ---------- ข้อความบนจอ ----------
const uint8_t SEG_CONN[] = {
  SEG_A | SEG_D | SEG_E | SEG_F,           // C
  SEG_C | SEG_D | SEG_E | SEG_G,           // o
  SEG_C | SEG_E | SEG_G,                   // n
  SEG_C | SEG_E | SEG_G                    // n
};

const uint8_t SEG_DONE[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,         // d
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F, // O
  SEG_C | SEG_E | SEG_G,                         // n
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G          // E
};

const uint8_t SEG_DASH[] = { SEG_G, SEG_G, SEG_G, SEG_G };   // ----

// ---------- ตัวแปร ----------
unsigned long lastWifiTry = 0;
bool syncedShown = false;

bool timeValid() {
  return time(nullptr) > MIN_VALID_TIME;
}

void startWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void setup() {
  pinMode(MODE_PIN, INPUT_PULLUP);
  display.setBrightness(7);          // 0-7 (7 = สว่างสุด)
  display.setSegments(SEG_CONN);

  startWiFi();
  lastWifiTry = millis();

  // เริ่มซิงค์เวลา (ถ้า WiFi ยังไม่ติด จะลองใหม่เองเมื่อเน็ตมา)
  configTime(TZ_OFFSET, DST_OFFSET,
             "time.navy.mi.th", "th.pool.ntp.org", "time.google.com");
}

void loop() {
  // ยังไม่ได้เวลาจริง (เพิ่งเปิดเครื่อง / ไฟดับมา)
  if (!timeValid()) {
    if (WiFi.status() != WL_CONNECTED) {
      display.setSegments(SEG_CONN);                // รอ WiFi
      if (millis() - lastWifiTry > 15000) {         // ลองต่อใหม่ทุก 15 วิ
        lastWifiTry = millis();
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      }
    } else {
      display.setSegments(SEG_DASH);                // WiFi ติดแล้ว รอ NTP
    }
    delay(200);
    return;
  }

  // ซิงค์สำเร็จครั้งแรก โชว์ "donE" สั้นๆ
  if (!syncedShown) {
    display.setSegments(SEG_DONE);
    delay(1000);
    syncedShown = true;
  }

  // อ่านเวลา (WiFi หลุดก็ยังเดินต่อได้)
  time_t now = time(nullptr);
  struct tm tmNow;
  localtime_r(&now, &tmNow);

  uint8_t hour = tmNow.tm_hour;
  bool mode12h = (digitalRead(MODE_PIN) == LOW);   // LOW = โหมด 12 ชม.
  if (mode12h) {
    hour = hour % 12;
    if (hour == 0) hour = 12;
  }

  uint8_t colon = (tmNow.tm_sec % 2 == 0) ? 0x40 : 0x00;
  // โหมด 12 ชม. ไม่แสดง 0 นำหน้า / โหมด 24 ชม. แสดง 0 นำหน้า
  display.showNumberDecEx(hour * 100 + tmNow.tm_min, colon, !mode12h);

  delay(100);
}
