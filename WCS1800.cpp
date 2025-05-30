// NAME: WCS1800.cpp
//

//#define DEBUG 1

#include <ESP8266WiFi.h>
#include "WCS1800.h"

#ifdef AVR
#define ANALOG_RESOLUTION     1023
#define INTERNAL_REF_VOLTAGE  1.085     // 1.1V with analogReference(INTERNAL)
#define DIVIDER               3.18      // voltage divider R1=100k, R2=220k: 320k/100k=3.2
#elif ESP8266
#define ANALOG_RESOLUTION     1023
#define INTERNAL_REF_VOLTAGE  1.0       // 1.0V is analogReference
#define DIVIDER               3.2       // on ESP8266 D1 mini the voltage divider is R1=100k, R2=220k: 320k/100k=3.2
#else
#error "Define ANALOG_RESOLUTION for your MCU"
#endif

// calibrate VREF for your device, so that current ~0A in idle mode
#define WCS_ZERO_VREF         1.7235    // adjusted, offset from datasheet for Vdd=3.3V is 1.6456V
#define WCS_SENSITIVITY       0.0503    // adjusted, sensitivity (slope) from datasheet for Vdd=3.3V is 0.0503V/A
#define WCS_SENS_RATIO        1.026    	// 1.026 at 20°C, 1.0 at 30°C

WCS1800::WCS1800(uint8_t aPin) {
	wcsPin = aPin;

  meanCurrent = 0;
  for (int i=0; i<10; i++) {
    meanBuffer[i] = 0;
  }
  meanBufPtr = 0;

  #ifdef AVR
  analogReference(INTERNAL);
  #endif
}

float WCS1800::readCurrent() {
  uint16_t value = analogRead(wcsPin);
  float diffVoltage = value * (INTERNAL_REF_VOLTAGE / float(ANALOG_RESOLUTION));
  float absVoltage = diffVoltage * DIVIDER;
  float current = (absVoltage - WCS_ZERO_VREF) / (WCS_SENSITIVITY * WCS_SENS_RATIO);
  
  meanCurrent = (meanCurrent * 10.0 - meanBuffer[meanBufPtr] + current) / 10.0;
  meanBuffer[meanBufPtr++] = current;
  meanBufPtr %= 10;

  #ifdef DEBUG
  Serial.print(value); Serial.print('\t');
  Serial.print(diffVoltage, 4); Serial.print('\t');
  Serial.print(absVoltage, 4); Serial.print('\t');
  Serial.print(current, 4); Serial.print('\t');
  Serial.println(meanCurrent, 1);
  #endif

  return current;
}

float WCS1800::readMeanCurrent() {
  readCurrent();
  return meanCurrent;
}
