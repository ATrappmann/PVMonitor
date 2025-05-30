void setup() {
  Serial.begin(115200);
#ifdef AVR
  analogReference(INTERNAL);
#endif
}

#ifdef AVR
#define ANALOG_RESOLUTION     1023
#define INTERNAL_REF_VOLTAGE  1.085     // 1.1V with analogReference(INTERNAL)
#define DIVIDER               3.18      // voltage divider R1=100k, R2=220k: 320k/100k=3.2
#elif ESP8266
#define ANALOG_RESOLUTION     1023
#define INTERNAL_REF_VOLTAGE  1.0		    // 1.0V is analogReference
#define DIVIDER               3.2       // on ESP8266 D1 mini the voltage divider is R1=100k, R2=220k: 320k/100k=3.2
#else
#error "Define ANALOG_RESOLUTION for your MCU"
#endif

// calibrate VREF for your device, so that current ~0A in idle mode
#define WCS_ZERO_VREF         1.7235    // adjusted, offset from datasheet for Vdd=3.3V is 1.6456V
#define WCS_SENSITIVITY		    0.0503    // adjusted, sensitivity (slope) from datasheet for Vdd=3.3V is 0.0503V/A
#define WCS_SENS_RATIO	      1.026    	// 1.026 at 20°C, 1.0 at 30°C

float meanCurrent = 0;
float meanBuffer[10];
uint8_t meanBufPtr = 0;

void loop() {
  uint16_t value = analogRead(A0);
  float diffVoltage = value * (INTERNAL_REF_VOLTAGE / float(ANALOG_RESOLUTION));
  float absVoltage = diffVoltage * DIVIDER;
  float current = (absVoltage - WCS_ZERO_VREF) / (WCS_SENSITIVITY * WCS_SENS_RATIO);
  
  meanCurrent = (meanCurrent * 10.0 - meanBuffer[meanBufPtr] + current) / 10.0;
  meanBuffer[meanBufPtr++] = current;
  meanBufPtr %= 10;

  Serial.print(value); Serial.print('\t');
  Serial.print(diffVoltage, 4); Serial.print('\t');
  Serial.print(absVoltage, 4); Serial.print('\t');
  Serial.print(current, 4); Serial.print('\t');
  Serial.print(meanCurrent, 1);
  Serial.println();
  delay(500);
}
