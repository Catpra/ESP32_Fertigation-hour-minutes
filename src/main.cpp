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
#include <stdarg.h>
#include <Ticker.h>
#include <RTClib.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "Solenoid.h"
#include "Scheduler.h"

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
    epaperDisplay.println("Weather: Sunny");
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