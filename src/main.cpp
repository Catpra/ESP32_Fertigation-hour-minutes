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

const char* ssid = "Tamaki";
const char* password = "wunangcepe";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200, 60000);

String URL = "http://api.openweathermap.org/data/2.5/weather?";
String ApiKey = "9a553a4d189678d8d2945eee265c3133";

String lat = "-6.981779358476813";
String lon = "110.41328171734197";
String weatherDescription = "Unknown";

Solenoid solenoid;
Ticker ticker;
Ticker ledBlinkOff;
void onScheduleExecute(uint16_t arDuration[]);
void ePaper_Init();
void ePaper_displayText(int row, TextAllign allign, const char* szFmt, ...);
void ePaper_updateDisplay();
void ePaper_displayClock(const DateTime& now);
void ePaper_displaySchedule(int idx, int count, uint16_t hh, uint16_t mm);
Scheduler scheduler(onScheduleExecute);
uint16_t m_now=0000;

void onScheduleExecute(uint16_t arDuration[]) {
  Serial.printf("onScheduleExecute %d of %d\n", scheduler.currentIdx()+1, scheduler.count());
  // ePaper_displaySchedule(scheduler.currentIdx()+1, scheduler.count(), 10, 15);
  ePaper_updateDisplay();
  solenoid.setSolenoidDuration(arDuration);
  solenoid.start();
}
// bool fOn = true;
void onTimer() {
  DateTime now = rtc.now();
  ePaper_displayClock(now);
  // digitalWrite(arSolenoidPin[0], fOn);
  // digitalWrite(arSolenoidPin[1], fOn);
  // digitalWrite(arSolenoidPin[2], fOn);
  // digitalWrite(MOTOR_PIN, fOn);
  // fOn = !fOn;
uint16_t now_hhmm = now.hour() * 100 + now.minute();
scheduler.run(now_hhmm);
}

void setup() {
  String message;
  Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - startTime > 10000) { // 10 seconds timeout
          Serial.println("Failed to connect to WiFi within 10 seconds.");
          break;
      }
    delay(2000);
    Serial.println("Connecting to WiFi...");
  }
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to WiFi");
        // Initialize NTP client
        timeClient.begin();
        timeClient.update();
        //buat ngecek apakah waktu sudah di set atau belum
        Serial.print("NTP time: ");
        Serial.println(timeClient.getFormattedTime());
        // Set RTC time
        rtc.adjust(DateTime(timeClient.getEpochTime()));
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
  scheduler.start(m_now);
}

void loop() {
  // wait for WiFi connection
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // Set HTTP Request Final URL with Location and API key information
    http.begin(URL + "lat=" + lat + "&lon=" + lon + "&units=metric&appid=" + ApiKey);
    
    // start connection and send HTTP Request
    int httpCode = http.GET();
    
    // httpCode will be negative on error
    if (httpCode > 0) {
      // Read Data as a JSON string
      String JSON_Data = http.getString();
      Serial.println("Raw JSON data:");
      Serial.println(JSON_Data);
      
      // Retrieve some information about the weather from the JSON format
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, JSON_Data);
      
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
      } else {
        // Extract and display the Current Weather Info
        const char* city = doc["name"];
        const char* country = doc["sys"]["country"];
        const char* description = doc["weather"][0]["description"];
        weatherDescription = String(description);
        float temp = doc["main"]["temp"];
        float humidity = doc["main"]["humidity"];
        
        Serial.println("\nCurrent Weather Information:");
        Serial.print("Location: ");
        Serial.print(city);
        Serial.print(", ");
        Serial.println(country);
        Serial.print("Description: ");
        Serial.println(description);
        Serial.print("Temperature: ");
        Serial.print(temp);
        Serial.println(" °C");
        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.println(" %");

        // Check if the weather description contains "rain"
        if (strstr(description, "rain") != NULL) {
            scheduler.cancelAllTasks();
            Serial.println("It's raining. All tasks have been cancelled.");
        }
      }
    } else {
      Serial.println("Error: Unable to fetch weather data");
    }
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
  
  // Wait for 30 seconds before next update
  Serial.println("\nWaiting 30 seconds before next update...");
  delay(30000);
}

/*******************************************************************************************/
// ePaper functions
void ePaper_Init()
{
  epaperDisplay.init(); 
  epaperDisplay.setRotation(1);
  epaperDisplay.setFont(&FreeMonoBold9pt7b);
  epaperDisplay.setTextColor(GxEPD_BLACK);
  ePaper_updateDisplay();
  ePaper_displayClock(rtc.now());
}

void ePaper_updateDisplay()
{
  epaperDisplay.setFullWindow();
  epaperDisplay.firstPage();
  do
  {
    epaperDisplay.setCursor(0, 1*15);
    epaperDisplay.print("   SMART IRRIGATION");
    epaperDisplay.setCursor(0, 3*15);
    epaperDisplay.println("Weather: " + weatherDescription);
    epaperDisplay.println("Pump :ON");
    epaperDisplay.println("Valve:OFF OFF OFF");
    epaperDisplay.printf ("Sched:%02d of %02d ->%02d:%02d", 
      scheduler.currentIdx()+1, scheduler.count(), 16, 15);
  }
  while (epaperDisplay.nextPage());
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
  uint16_t wh = 15; //FreeMonoBold9pt7b.yAdvance;
  uint16_t y = row * wh; // y is base line!
  uint16_t wy = y - wh/2;
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
}

void ePaper_displayClock(const DateTime& now) {
  ePaper_displayText(8, ALLIGN_CENTER, "%02d/%02d/%04d %02d:%02d:%02d", 
    now.day(), now.month(), now.year(), 
    now.hour(), now.minute(), now.second());
}

void ePaper_displaySchedule(int idx, int count, uint16_t hh, uint16_t mm) {
  ePaper_displayText(7, ALLIGN_LEFT, "Sched:%02d of %02d ->%02d:%02d", idx, count, hh, mm);
}