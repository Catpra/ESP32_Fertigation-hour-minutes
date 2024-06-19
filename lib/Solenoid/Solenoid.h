#include <Arduino.h>
#include <Ticker.h>
#define MAX_OUTPUTS 8
#define ON 1
#define OFF 0

class Solenoid {
public:
  Solenoid() {
    m_nSolenoidOnDelay = 0;
    m_nSolenoidCount = 0;
    m_nCurrentIndex = 0;
    m_nLastIndex = 0;
    m_nMotorPumpPin=-1;
    m_nState = IDLE;
    m_nSolenoidOffDelay = 0;
  }
  void begin(int8_t nOutputCount, int8_t nPin, uint16_t nSolenoidOnDelay,  uint16_t nSolenoidOffDelay);
  void setSolenoidPins(const int8_t arPins[]);
  void setSolenoidDuration(const uint16_t arDuration[]);
  void start();

protected:
  void runStateMachine();

  static void onRun(Solenoid* p) {
    p->runStateMachine();
  };
  static void onSolenoidOff(Solenoid* p){
      digitalWrite(p->m_arSolenoidPin[p->m_nLastIndex], OFF);
  }

private:
  
  Ticker m_TickerRun;
  Ticker m_TickerSolenoidOff;
  int8_t  m_nMotorPumpPin=-1;
  u_int16_t m_nSolenoidOffDelay;
  u_int16_t m_nSolenoidOnDelay;
  int8_t m_arSolenoidPin[MAX_OUTPUTS];
  uint16_t m_arSolenoidActiveDurationInSeconds[MAX_OUTPUTS];
  uint8_t m_nSolenoidCount;
  enum {IDLE, MOTOR_ON, SOLENOID_ACTIVE} m_nState;

  uint8_t m_nCurrentIndex;
  uint8_t m_nLastIndex;
};