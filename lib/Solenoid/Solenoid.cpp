#include <Arduino.h>
#include "Solenoid.h"

void Solenoid::begin(int8_t nOutputCount, int8_t nPin, uint16_t nSolenoidOnDelay,  uint16_t nSolenoidOffDelay)
{
  m_nCurrentIndex = 0;
  m_nState = IDLE;
  m_nSolenoidCount = nOutputCount;
  m_nMotorPumpPin = nPin;
  m_nSolenoidOnDelay = nSolenoidOnDelay;
  m_nSolenoidOffDelay = nSolenoidOffDelay;
  pinMode(m_nMotorPumpPin, OUTPUT);
  digitalWrite(m_nMotorPumpPin, OFF);
}

void Solenoid::setSolenoidPins(const int8_t arPins[])
{
  for (uint8_t i = 0; i < m_nSolenoidCount; i++) {
    m_arSolenoidPin[i] = arPins[i];
    pinMode(m_arSolenoidPin[i], OUTPUT);
    digitalWrite(m_arSolenoidPin[i], OFF);
  }
}

void Solenoid::setSolenoidDuration(const uint16_t arDuration[])
{
  for (uint8_t i = 0; i < m_nSolenoidCount; i++) 
    m_arSolenoidActiveDurationInSeconds[i] = arDuration[i];
}

void Solenoid::start() {
  if (m_nState==IDLE)
  {
    m_nCurrentIndex = 0;
    m_nLastIndex = 0;
    digitalWrite(m_arSolenoidPin[m_nCurrentIndex], ON);
    m_nState = MOTOR_ON;
    m_TickerRun.once_ms(m_nSolenoidOnDelay, &Solenoid::onRun, this);
  }
}

void Solenoid::runStateMachine() {
  switch (m_nState)
  {
  case MOTOR_ON:
    digitalWrite(m_nMotorPumpPin, ON);
    m_nState = SOLENOID_ACTIVE;
    m_TickerRun.once(m_arSolenoidActiveDurationInSeconds[m_nCurrentIndex], &Solenoid::onRun, this);
    break;
  
  case SOLENOID_ACTIVE:
    if (m_nCurrentIndex < m_nSolenoidCount)
    {
      m_nLastIndex = m_nCurrentIndex++;
      digitalWrite(m_arSolenoidPin[m_nCurrentIndex], ON);
      m_TickerRun.once(m_arSolenoidActiveDurationInSeconds[m_nCurrentIndex], &Solenoid::onRun, this);
    }
    else
    {
      digitalWrite(m_nMotorPumpPin, OFF);
      m_nState = IDLE;
    }
    m_TickerSolenoidOff.once_ms(m_nSolenoidOnDelay, &Solenoid::onSolenoidOff, this);
    break;
  }
}
