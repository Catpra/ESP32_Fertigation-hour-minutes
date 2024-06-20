/*
Board: LilyGo-T5 2.13" V2.3.1_2.13 Inch E-Paper
DriverChip:   GDEW0213T5
  - DEPG0213BN  : greylevel=2
  - GDEM0213B74 : greylevel=4 *
Product page: https://www.aliexpress.com/item/1005003063164032.html
Github: https://github.com/Xinyuan-LilyGO/LilyGo-T5-Epaper-Series/
Example:
  - Hello World: https://github.com/Xinyuan-LilyGO/LilyGo-T5-Epaper-Series/blob/master/examples/GxEPD_Hello_world/GxEPD_Hello_world.ino
*/

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <stdarg.h>
#include <Ticker.h>
#include <RTClib.h>
#include <NTPClient.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "Solenoid.h"
#include "Scheduler.h"
#include "ArduinoJson.h"

GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> epaperDisplay(GxEPD2_213_B74(/*CS=5*/ 5, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4)); // GDEM0213B74 122x250, SSD1680
enum TextAllign {ALLIGN_CENTER, ALLIGN_LEFT, ALLIGN_RIGHT};

RTC_DS3231 rtc;

#define NUM_OUTPUS 3
#define MOTOR_PIN 25
#define SOLENOID_ON_DELAY 1000 // ini adalah delay output dalam miliseconds
#define SOLENOID_OFF_DELAY 2000

// ini adalah durasi output dalam Seconds
const int8_t arSolenoidPin[NUM_OUTPUS] = {26, 27, 12};
uint16_t arSolenoidActiveDuration[NUM_OUTPUS] = {2, 2, 2};
bool solenoidStatus[NUM_OUTPUS] = {false, false, false};

const char* ssid = "Tamaki";
const char* password = "wunangcepe";

// WiFiUDP ntpUDP;
// NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200, 60000);

String URL = "http://api.openweathermap.org/data/2.5/weather?";
String ApiKey = "9a553a4d189678d8d2945eee265c3133";
String lat = "-6.981779358476813";
String lon = "110.41328171734197";

Solenoid solenoid;
Ticker weatherTicker;
Ticker ticker;
Ticker ledBlinkOff;
String weatherDescription = "Unknown";
int weatherConditionCode = 0;
void onScheduleExecute(const uint16_t arDuration[]);
void ePaper_Init();
void ePaper_displayText(int row, TextAllign allign, const char* szFmt, ...);
void ePaper_displayClock(const DateTime& now);
void ePaper_displaySchedule();
Scheduler scheduler(onScheduleExecute);
uint16_t m_now=0000;

void onScheduleExecute(const uint16_t arDuration[]) {
  Serial.printf("onScheduleExecute %d of %d\n", scheduler.currentIdx()+1, scheduler.count());
  ePaper_displaySchedule();
  solenoid.setSolenoidDuration(arDuration);
  solenoid.start();
  for (int i = 0; i < NUM_OUTPUS; i++) {
      solenoidStatus[i] = true;
  }
  ePaper_Init();
}


uint16_t currentTime()
{
  DateTime now = rtc.now();
  return now.hour()*100 + now.minute();
}

void onTimer() {
  DateTime now = rtc.now();
  uint16_t t = now.hour()*100 + now.minute();
  ePaper_displayClock(now);
  scheduler.run(t);
}

// void updateDisplayWithSolenoidStatus() {
//     epaperDisplay.setFullWindow();
//     epaperDisplay.firstPage();
//     do {
//         epaperDisplay.setCursor(0, 1 * 15);
//         epaperDisplay.print("   SMART IRRIGATION");
//         epaperDisplay.setCursor(0, 3 * 15);
//         epaperDisplay.print("Weather: ");
//         epaperDisplay.println(weatherDescription);
//         epaperDisplay.print("Pump :");
//         epaperDisplay.println(solenoid.isRunning() ? "ON" : "OFF");
//         epaperDisplay.print("Valve:");
//         for (int i = 0; i < NUM_OUTPUS; i++) {
//             epaperDisplay.print(solenoid.isSolenoidOn(i) ? " ON" : " OFF");
//         }
//         epaperDisplay.println();
//         epaperDisplay.printf("Sched:%02d of %02d ->%02d:%02d",
//                              scheduler.currentIdx() + 1, scheduler.count(), 16, 15);
//     } while (epaperDisplay.nextPage());
// }

void setup() {
  Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - startTime > 10000) { // 10 seconds timeout
          Serial.println("Failed to connect to WiFi within 10 seconds.");
          break;
      }
    Serial.println("Connecting to WiFi...");
  }
      if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to WiFi");
      HTTPClient http;
      http.begin(URL + "lat=" + lat + "&lon=" + lon + "&units=metric&appid=" + ApiKey);
      int httpCode = http.GET();
      
      if (httpCode > 0) {
        String JSON_Data = http.getString();
        Serial.println("Raw JSON data:");
        Serial.println(JSON_Data);
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, JSON_Data);
        
        if (error) {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
        } else {
            const char* description = doc["weather"][0]["description"];
            weatherConditionCode = doc["weather"][0]["id"];
            weatherDescription = String(description);

            Serial.print("Weather Description: ");
            Serial.println(description);

            // Cancel tasks if weather condition is not clear sky (800)
            if (weatherConditionCode != 800) {
                scheduler.cancelAllTasks();
                Serial.println("Weather is not clear. All tasks have been cancelled.");
            }
            ePaper_Init();
        }
      } else {
        Serial.println("Error: Unable to fetch weather data");
      }
      http.end();
        // // Initialize NTP client
        // timeClient.begin();
        // timeClient.update();
        // //buat ngecek apakah waktu sudah di set atau belum
        // Serial.print("NTP time: ");
        // Serial.println(timeClient.getFormattedTime());
        // // Set RTC time
        // rtc.adjust(DateTime(timeClient.getEpochTime()));
      } else {
        Serial.println("WiFi not connected");
      }

  // this will turn on motor pump after 1000ms and turn off after 2000ms
  solenoid.begin(NUM_OUTPUS, MOTOR_PIN, SOLENOID_ON_DELAY, SOLENOID_OFF_DELAY);
  solenoid.setSolenoidPins(arSolenoidPin);
  if (rtc.begin())
  {
    Serial.println("RTC is running");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  else {
    Serial.println("Couldn't find RTC"); 
  }
  scheduler.addTask(1818, arSolenoidActiveDuration);
  scheduler.addTask(1822, arSolenoidActiveDuration);
  scheduler.addTask(2825, arSolenoidActiveDuration);

  ePaper_Init();
  Serial.println("System running...");
  ticker.attach(1, onTimer); 
  scheduler.start(currentTime());
  //weatherTicker.attach(1800, checkWeather);
}

void loop() {

}

/*******************************************************************************************/
// ePaper functions
void ePaper_Init()
{
  epaperDisplay.init(); 
  epaperDisplay.setRotation(1);
  epaperDisplay.setFont(&FreeMonoBold9pt7b);
  epaperDisplay.setTextColor(GxEPD_BLACK);
  ePaper_displayText(1, ALLIGN_CENTER, "SMART IRRIGATION");
  ePaper_displayText(2, ALLIGN_LEFT, "Weather: ", weatherDescription);
  ePaper_displayText(3, ALLIGN_LEFT, "Pump :ON");
  ePaper_displayText(4, ALLIGN_LEFT, "Valve:OFF OFF OFF");
  ePaper_displaySchedule();
  ePaper_displayClock(rtc.now());
}

void ePaper_displayText(int row, TextAllign allign, const char* szFmt, ...)
{
  char buffer[128];
  va_list args;
  va_start(args, szFmt);
  vsnprintf(buffer, 127, szFmt, args);
  va_end(args);
  int16_t tbx, tby; uint16_t tbw, tbh;
  epaperDisplay.getTextBounds(buffer, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t wh = FreeMonoBold9pt7b.yAdvance;
  uint16_t y = row * wh; // y is base line!
  uint16_t wy = y - wh/2 - 1;
  uint16_t x;
  switch (allign)
  {
    case ALLIGN_CENTER:
      x = ((epaperDisplay.width() - tbw) / 2) - tbx;
      break;
    case ALLIGN_LEFT:
      x = 0;
      break;
    case ALLIGN_RIGHT:
      x = epaperDisplay.width() - tbw - tbx;
      break;
  }
  epaperDisplay.setPartialWindow(0, wy, epaperDisplay.width(), wh);
  epaperDisplay.firstPage();
  do
  {
    epaperDisplay.fillScreen(GxEPD_WHITE);
    epaperDisplay.setCursor(x, y);
    epaperDisplay.print(buffer);
  }
  while (epaperDisplay.nextPage());
  // delay(100);
}

void ePaper_displayClock(const DateTime& now) {
  ePaper_displayText(6, ALLIGN_CENTER, "%02d/%02d/%04d %02d:%02d:%02d", 
    now.day(), now.month(), now.year(), 
    now.hour(), now.minute(), now.second());
}

void ePaper_displaySchedule() {
  uint16_t nextTime = scheduler.nextScheduleTime();
  ePaper_displayText(5, ALLIGN_LEFT, "Sched:%02d of %02d ->%02d:%02d", 
    scheduler.currentIdx()+1, scheduler.count(), nextTime/100, nextTime%100);
}