// NAME: Shelly.cpp
//

//#define DEBUG 1

#if defined(ESP32)
#include <WiFi.h>
#include <HTTPClient.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#else
#error "Define ESP board specific includes here"
#endif

#include <ArduinoJson.h>  // from Benoit Blanchon
#include "Shelly.h"

/*
float getShellyPower(String ip) {
  float power = 0.0;
  
  String url = "http://" + ip + "/status";
  WiFiClient wifi;
  HTTPClient client;
  client.begin(wifi, url);
  int httpCode = client.GET();
  if (HTTP_CODE_OK == httpCode) {
    String json = client.getString();
  #ifdef DEBUG
    Serial.println(json);
  #endif
    JsonDocument jsonDoc;
    if (!deserializeJson(jsonDoc, json)) {
      power = jsonDoc["total_power"]; // for Shelly 3EM
      if (0 == power) { // not Shelly 3EM, probably Shelly Plug 
        power = jsonDoc["meters"][0]["power"];
      }
    }
  } 
  else {
    Serial.printf("ERROR: Cannot get status from Shelly at %s, httpCode=%d\n", ip.c_str(), httpCode);
  }
  client.end();
  return power;
}

bool getShellyRelay(String ip) {
  bool relayOn = false;

  String url = "http://" + ip + "/status";
  WiFiClient wifi;
  HTTPClient client;
  client.begin(wifi, url);
  int httpCode = client.GET();
  if (HTTP_CODE_OK == httpCode) {
    String json = client.getString();
  #ifdef DEBUG
    Serial.println(json);
  #endif
    JsonDocument jsonDoc;
    if (!deserializeJson(jsonDoc, json)) {
      relayOn = jsonDoc["relays"][0]["ison"];
    } 
  } 
  else {
    Serial.printf("ERROR: Cannot get status from Shelly at %s, httpCode=%d\n", ip.c_str(), httpCode);
  }
  client.end();
  return relayOn;
}
*/

bool getShellyStatus(String ip, ShellyStatus *status) {
  if (NULL == status) return false;
  
  /* do not overwrite last status
  status->power = 0;
  status->total_power = 0;
  status->relayOn = false;
  */

  String url = "http://" + ip + "/status";
  WiFiClient wifi;
  HTTPClient client;
  client.begin(wifi, url);
  int httpCode = client.GET();
  if (HTTP_CODE_OK == httpCode) {
    String json = client.getString();
  #ifdef DEBUG
    Serial.println(json);
  #endif
    JsonDocument jsonDoc;
    if (deserializeJson(jsonDoc, json)) {
      Serial.println(F("ERROR: JSON parsing failed!"));
      return false;
    }

    status->power = jsonDoc["total_power"]; // on Shelly 3EM
    if (0 != status->power) { // EM3
      float total0 = jsonDoc["emeters"][0]["total"];
      float total1 = jsonDoc["emeters"][1]["total"];
      float total2 = jsonDoc["emeters"][2]["total"];
      status->total_power = total0 + total1 + total2;
    }
    else { // Shelly Plug
      status->power = jsonDoc["meters"][0]["power"];
      status->total_power = jsonDoc["meters"][0]["total"];
      status->total_power /= 60.0; // returned in Watt minutes, converted to Wh
    }
    status->relayOn = jsonDoc["relays"][0]["ison"];
    #ifdef DEBUG
    Serial.printf("power=%.1fW, total_power=%.1fWh, relay=%d\n", status->power, status->total_power, status->relayOn);
    #endif

    client.end();
    return true;
  } 
  else {
    Serial.printf("ERROR: Cannot get status from Shelly at %s, httpCode=%d\n", ip.c_str(), httpCode);
    client.end();
    return false;
  }
}
