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
#include <string.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <stdarg.h>
#include <Ticker.h>
#include <RTClib.h>
#include <NTPClient.h>
#include <GxEPD2_BW.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <SPI.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "Solenoid.h"
#include "Scheduler.h"
#include "ArduinoJson.h"

// UUIDs for the BLE Service and Characteristics
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define NUM_OUTPUTS 3
#define MOTOR_PIN 25
#define SOLENOID_ON_DELAY 1000 // ini adalah delay output dalam miliseconds
#define SOLENOID_OFF_DELAY 2000

GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> epaperDisplay(GxEPD2_213_B74(/*CS=5*/ 5, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4)); // GDEM0213B74 122x250, SSD1680
enum TextAllign {ALLIGN_CENTER, ALLIGN_LEFT, ALLIGN_RIGHT};

RTC_DS3231 rtc;


const int8_t arSolenoidPin[NUM_OUTPUTS] = {26, 27, 12};

bool solenoidStatus[NUM_OUTPUTS] = {false, false, false};

const char* ssid = "Tamaki";
const char* password = "wunangcepe";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200, 60000);

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

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;

void onScheduleExecute(const uint16_t arDuration[]);
void ePaper_Init();
void ePaper_displayText(int row, TextAllign allign, const char* szFmt, ...);
void ePaper_updateDisplay();
void ePaper_displayClock(const DateTime& now);
void ePaper_displaySchedule();
Scheduler scheduler(onScheduleExecute);
bool shouldRunSchedule = false;
bool hasCheckedWeather = false;
uint16_t m_now=0000;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.println("*********");
      Serial.print("Received Value Length: ");
      Serial.println(value.length());
      Serial.print("Received Value: ");
      Serial.println(value.c_str());

      char input[value.length() + 1];
      strcpy(input, value.c_str());

      // Maximum number of schedules expected
      uint16_t timeArray[MAX_SCHEDULER_COUNT];
      uint16_t activeDurationArray[MAX_SCHEDULER_COUNT][NUM_OUTPUTS];
      int scheduleCount = 0;

      // Split the input string by semicolons
      char* schedule = strtok(input, ";");

      while (schedule != NULL && scheduleCount < MAX_SCHEDULER_COUNT) {
        Serial.print("Schedule Count: ");
        Serial.println(scheduleCount);
        // Split each schedule by commas
        char* timeString = strtok(schedule, ",");
        char* durationString1 = strtok(NULL, ",");
        char* durationString2 = strtok(NULL, ",");
        char* durationString3 = strtok(NULL, ",");

        // Split the time part by the colon
        char* hoursString = strtok(timeString, ":");
        char* minutesString = strtok(NULL, ":");

        // Convert strings to integers
        int hours = atoi(hoursString);
        int minutes = atoi(minutesString);
        int duration1 = durationString1 ? atoi(durationString1) : 0;
        int duration2 = durationString2 ? atoi(durationString2) : 0;
        int duration3 = durationString3 ? atoi(durationString3) : 0;

        // Combine hours and minutes into a single integer
        timeArray[scheduleCount] = static_cast<uint16_t>(hours * 100 + minutes);
        activeDurationArray[scheduleCount][0] = static_cast<uint16_t>(duration1);
        activeDurationArray[scheduleCount][1] = static_cast<uint16_t>(duration2);
        activeDurationArray[scheduleCount][2] = static_cast<uint16_t>(duration3);

        // Print the parsed values for debugging
        Serial.print("Time[");
        Serial.print(scheduleCount);
        Serial.print("]: ");
        Serial.println(timeArray[scheduleCount]);
        
        Serial.print("activeDurationArray[");
        Serial.print(scheduleCount);
        Serial.print("]: ");
        Serial.print(activeDurationArray[scheduleCount][0]);
        Serial.print(", ");
        Serial.print(activeDurationArray[scheduleCount][1]);
        Serial.print(", ");
        Serial.println(activeDurationArray[scheduleCount][2]);

        // Move to the next schedule
        schedule = strtok(NULL, ";");
        scheduleCount++;
      }

      // Call scheduler.addTask for each parsed schedule (assuming the function exists)
      for (int i = 0; i < scheduleCount; i++) {
        scheduler.addTask(timeArray[i], activeDurationArray[i]);
      }

      Serial.println("*********");
    }
  }
};

void onScheduleExecute(const uint16_t arDuration[]) {
  Serial.printf("onScheduleExecute %d of %d\n", scheduler.currentIdx()+1, scheduler.count());
  ePaper_displaySchedule();
  solenoid.setSolenoidDuration(arDuration);
  solenoid.start();
  for (int i = 0; i < NUM_OUTPUTS; i++) {
      solenoidStatus[i] = true;
  }
  ePaper_Init();
}


void connectToWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) { // 10 seconds timeout
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to WiFi");
    } else {
        Serial.println("Failed to connect to WiFi within 10 seconds.");
    }
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

void checkWeather() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(URL + "lat=" + lat + "&lon=" + lon + "&units=metric&appid=" + ApiKey);
        int httpCode = http.GET();

        Serial.printf("HTTP Code: %d\n", httpCode); // Debugging

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
                Serial.printf("Weather Condition Code: %d\n", weatherConditionCode);

                // Cancel tasks if weather condition is not clear sky (800)
                if (weatherConditionCode != 800) {
                    scheduler.cancelAllTasks();
                    shouldRunSchedule = false;
                    Serial.println("Weather is not clear. All tasks have been cancelled.");
                } else {
                    shouldRunSchedule = true;
                }

                hasCheckedWeather = true;
                ePaper_Init(); // Update display with new weather info
            }
        } else {
            Serial.println("Error: Unable to fetch weather data");
        }
        http.end();
    } else {
        Serial.println("WiFi not connected. Running schedule by default.");
        shouldRunSchedule = true;
        hasCheckedWeather = true;
    }
}

void bluetoothInitialization(){
  BLEDevice::init("ESP32_Test");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE
                    );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("Hello World");

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}



void setup() {
    Serial.begin(115200);
    connectToWiFi();

    solenoid.begin(NUM_OUTPUTS, MOTOR_PIN, SOLENOID_ON_DELAY, SOLENOID_OFF_DELAY);
    solenoid.setSolenoidPins(arSolenoidPin);
    if (rtc.begin()) {
        Serial.println("RTC is running");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    } else {
        Serial.println("Couldn't find RTC");
    }

    bluetoothInitialization();

    ePaper_Init();
    Serial.println("System running...");
    ticker.attach(1, onTimer);

    // Ensure weather check is done before starting the schedule
    weatherTicker.attach(10, checkWeather);
}

void loop() {
    if (!hasCheckedWeather) {
        // Wait for the initial weather check to complete
        delay(1000);
        return;
    }

    if (shouldRunSchedule) {
        scheduler.start(m_now);
        shouldRunSchedule = false; // Ensure the schedule only runs once
        weatherTicker.attach(1800, checkWeather); // Check weather every 30 minutes
    }
}

/*******************************************************************************************/
// ePaper functions
void ePaper_Init()
{
    epaperDisplay.init();
    epaperDisplay.setRotation(1);
    epaperDisplay.setFont(&FreeMonoBold9pt7b);
    epaperDisplay.setTextColor(GxEPD_BLACK);

    epaperDisplay.setFullWindow();
    epaperDisplay.firstPage();
    do {
        int y = 15; // Starting Y position
        epaperDisplay.setCursor(0, y);
        epaperDisplay.print("   SMART IRRIGATION");

        y += 15; // Move to the next line
        epaperDisplay.setCursor(0, y);
        epaperDisplay.print("Weather: ");
        epaperDisplay.println(weatherDescription);

        y += 15; // Move to the next line
        epaperDisplay.setCursor(0, y);
        epaperDisplay.print("Pump: ");
        epaperDisplay.println(solenoidStatus[0] || solenoidStatus[1] || solenoidStatus[2] ? "ON" : "OFF");

        y += 15; // Move to the next line
        epaperDisplay.setCursor(0, y);
        epaperDisplay.print("Valve:");
        for (int i = 0; i < NUM_OUTPUTS; i++) {
            epaperDisplay.print(solenoidStatus[i] ? " ON" : " OFF");
        }
        epaperDisplay.println();

        y += 15; // Move to the next line
        epaperDisplay.setCursor(0, y);
        epaperDisplay.printf("Sched: %02d of %02d -> %02d:%02d",
                             scheduler.currentIdx() + 1, scheduler.count(), 16, 15);

        y += 15; // Move to the next line
        epaperDisplay.setCursor(0, y);
        DateTime now = rtc.now();
        epaperDisplay.printf("%02d/%02d/%04d %02d:%02d:%02d", 
                             now.day(), now.month(), now.year(), 
                             now.hour(), now.minute(), now.second());
    } while (epaperDisplay.nextPage());
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