// NAME: Display.cpp
//

#include <ESP8266WiFi.h>
#include "Display.h"
#include "Configuration.h"

Display::Display() {
  screen = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT);  
}

bool Display::begin() {
  if (screen->begin()) {
    screen->setRotation(2); // upside down 
    screen->setTextColor(SSD1306_WHITE); // Draw white text
    screen->cp437(true);         // Use full 256 char 'Code Page 437' font
    screen->clearDisplay();
//    screen->print(F("Battery\n"));
    screen->display();
    return true;
  }
  else return false;
}

void Display::setCursor(uint8_t line) {
  screen->setCursor(0, 8*line);
}
void Display::setCursor(uint8_t line, uint8_t col) {
  screen->setCursor(6*col, 8*line);
}

void Display::print(char c) {
  screen->print(c);
}
void Display::print(const char * msg) {
  screen->print(msg);
}
void Display::print(const Printable& msg) {
  screen->print(msg);
}
void Display::print(int value) {
  screen->printf("%d", value);
}
void Display::print(long value) {
  screen->printf("%ld", value);
}
void Display::print(float value) {
  screen->printf("%.02f", value);
}

void Display::print(uint8_t line, const char *msg) {
  screen->setCursor(0, line*8);
  screen->print(msg);
}
void Display::print(uint8_t line, const Printable& msg) {
  screen->setCursor(0, line*8);
  screen->print(msg);
}

void Display::show() {
  screen->display();
}

void Display::clean() {
  screen->clearDisplay();
}

void Display::clearLine(uint8_t line) {
  clearLine(line, 0);
}
void Display::clearLine(uint8_t line, uint8_t col) {
  for (int i=0; i<8; i++) {
    screen->writeFastHLine(col*6, line*8+i, SCREEN_WIDTH-col*6, SSD1306_BLACK);
  }
  screen->setCursor(col*6, line*8);
}

size_t Display::write(uint8_t c) {
  return screen->write(c);
}
