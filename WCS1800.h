// NAME: WCS1800.h
//
// CONNECTIONS:
//	VCC -> 3.3V
//  DOUT -> nc
//	GND	-> GND
//	AOUT -> Ax
//
#ifndef WCS1800_H
#define WCS1800_H

class WCS1800 {
private:
	uint8_t	wcsPin;

	float		meanCurrent;
	float		meanBuffer[10];
	uint8_t	meanBufPtr;
  
public:
	WCS1800(uint8_t pin);
	
	float readCurrent();
  float readMeanCurrent();
};

#endif /* WCS1800_H */
