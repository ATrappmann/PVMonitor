// NAME: Truck.cpp
//

//#define DEBUG 1

#include "Configuration.h"

#if defined(ESP32)
#include <WiFi.h>
#include <HTTPClient.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#else
#error "Define ESP board specific includes here"
#endif

#include "Trucki.h"

int getTruckiMaxPower(String ip) {
  int maxPower = -1;

  String url = "http://" + ip + "/json";
  WiFiClient wifi;
  HTTPClient client;
  client.begin(wifi, url);
  int httpCode = client.GET();
  if (httpCode > 0) {
    String line = client.getString();
  #ifdef DEBUG
    Serial.println(line);
  #endif
    char *powerStr = strstr(line.c_str(), "\"MAXPOWER\":\"");
    if (NULL != powerStr) {
      sscanf(&powerStr[12], "%d", &maxPower);
  #ifdef DEBUG
      Serial.printf("Trucki: maxPower=%d\n", maxPower);
  #endif  
    }
  } else {
    Serial.printf("ERROR: Cannot get json from TruckiStick at %s, httpCode=%d\n", ip.c_str(), httpCode);
  }
  client.end();
  return maxPower;
}

bool setTruckiMaxPower(String ip, uint16_t maxPower) {
  if (maxPower < TRUCKI_MIN_LOAD) maxPower = TRUCKI_MIN_LOAD; // safety check
  if (maxPower > TRUCKI_MAX_LOAD) maxPower = TRUCKI_MAX_LOAD; // safety check
  #ifdef DEBUG
  Serial.printf("setTruckiMaxPower to %d\n", maxPower);
  #endif

  String url = "http://" + ip + "/?maxPower=" + String(maxPower);
  WiFiClient wifi;
  HTTPClient client;
  client.begin(wifi, url);
  int httpCode = client.GET();
  if (httpCode > 0) {
    client.end();
    return true;
  }
  client.end();
  Serial.printf("ERROR: Cannot set maxPower for TruckiStick at %s, httpCode=%d\n", ip.c_str(), httpCode);
  return false;
}

/*
float getTruckiMeter(String ip) {
  float meter = -1;

  String url = "http://" + ip + "/json";
  WiFiClient wifi;
  HTTPClient client;
  client.begin(wifi, url);
  int httpCode = client.GET();
  if (httpCode > 0) {
    String line = client.getString();
    char *powerStr = strstr(line.c_str(), "\"MQTT_METER_VALUE\":\"");  //"\"METER\":\"");
    if (NULL != powerStr) {
      sscanf(&powerStr[20], "%f", &meter);
  #ifdef DEBUG
      Serial.printf("Trucki: Meter=%.02f\n", meter);
  #endif  
    }
  } else {
    Serial.printf("ERROR: Cannot get json from TruckiStick at %s, httpCode=%d\n", ip.c_str(), httpCode);
  }
  client.end();
  return meter;
}
*/
