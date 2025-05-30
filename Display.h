// NAME: Display.h
//
// CONNECTIONS:
//    128x32 OLED:
//       pin -> ESP pin
//       GND -> GND
//       VCC -> 3V3
//       SDA -> D2
//       SCL -> D1
//
#ifndef DISPLAY_H
#define DISPLAY_H

#define SSD1306_NO_SPLASH 1
#include <Adafruit_SSD1306.h>

class Display : public Print {
protected:
  Adafruit_SSD1306 *screen;

public:
  Display();
  bool begin();

  void setCursor(uint8_t line);
  void setCursor(uint8_t line, uint8_t col);

  void print(char c);
  void print(const char *msg);
  void print(const Printable& msg);
  void print(int value);
  void print(long value);
  void print(float value);

  void print(uint8_t line, const char *msg);
  void print(uint8_t line, const Printable& msg);

  void show();
  void clean();
  void clearLine(uint8_t line);
  void clearLine(uint8_t line, uint8_t col);

protected:
  virtual size_t write(uint8_t);
};

#endif /* DISPLAY_H */
