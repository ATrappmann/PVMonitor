// NAME: PVMonitor.ino
//
// DESC: Monitor battery charging/discharging of 2 Victron MPPT-Charger
// 
// FEATURES: 
//  * 2 Victron MPPT-Charger (east/west)
//  * 128x32 OLED
//  * WCS1800 Current Sensor
//  * Publish WebServer with HTML/JSON data
//  * Store daily charging power for 2 month in EEPROM
//
// BOARD: ESP8266 -> LOLIN(WEMOS) D1 mini (clone)
// FLASH SIZE: 4MB (FS:none, OTA:~1019KB)
//
// CONNECTIONS:
//    VE.direct interface #1:
//       pin  function -> ESP pin
//        1   GND      -> GND
//        2   RX 3V    -> (nc)
//        3   TX 5V -> Level Shifter -> D7 (RX)
//        4   VCC      -> (nc)
//
//    VE.direct interface #2:
//       pin  function -> ESP pin
//        1   GND      -> GND
//        2   RX 3V    -> (nc)
//        3   TX 5V -> Level Shifter -> D5 (RX)
//        4   VCC      -> (nc)
//
//    128x32 OLED:
//       pin -> ESP pin
//       GND -> GND
//       VCC -> 3V3
//       SDA -> D2
//       SCL -> D1
//
//	  WCS1800 Current Monitor:
//       pin  -> ESP pin
//		   VCC  -> 3.3V
//  	   DOUT -> (nc)
//		   GND  -> GND
//		   AOUT -> A0		 
//
//	  Restart Button (against GND)
//		  ESP pin
//			D3
//
// Copyright (c) 2024,25 by Andreas Trappmann. All rights reserved.
//
// This file is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//

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

#include <time.h>
/*
  Liste der Zeitzonen
  https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  Zeitzone CET = Central European Time -1 -> 1 Stunde zurück
  CEST = Central European Summer Time von
  M3 = März, 5.0 = Sonntag 5. Woche, 02 = 2 Uhr
  bis M10 = Oktober, 5.0 = Sonntag 5. Woche 03 = 3 Uhr
*/
// local time zone definition (Berlin)
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"

#include "WebServer.h"
#include <ESP8266WebServer.h>
extern ESP8266WebServer webserver;

#include "Display.h"
Display display;

#include "Victron.h"
Victron victron1(VICTRON_WEST_RXPIN, -1);  // RX, no TX - WEST side
Victron victron2(VICTRON_EAST_RXPIN, -1);  // RX, no TX - EAST side

#include "WCS1800.h"
WCS1800 wcs(CURRENT_SENSOR_PIN);	// AOUT of current monitor

#include "PersistentData.h"
PersistentData backup;

#include "Shelly.h"
#include "Trucki.h"

char startupTS[20]; // "2024-06-15 18:21:00" -> len=19 + '\0'
uint32_t lastUpdate = 0;
uint32_t lastLoop = 0;
uint32_t lastInverterCheck = -1; // overwrite waiting time interval
float sumBatPower = 0.0;
bool daySaved = false;
VictronTrackerMode veLastTrackerModeEast = VTM_UNDEFINED;
VictronTrackerMode veLastTrackerModeWest = VTM_UNDEFINED;
float timeSunrise = 0.0;
float timeSunset = 0.0;
uint16_t lastMaxPower = TRUCKI_MIN_LOAD;
bool lastInverterRelay = false;
long panelPower = 0;
float inverterPower = 0.0;
long chargePower = 0;
float meanBatVoltage = 0.0;

ShellyStatus statusInv;
ShellyStatus status3EM;
uint32_t errorFlags = 0;

void setup() {
  errorFlags = 0; // clean start
	
  pinMode(LED_BUILTIN, OUTPUT); // D4
  pinMode(RESTART_BUTTON_PIN, INPUT_PULLUP);	// D3

  // get reset reason
  rst_info *resetInfo = ESP.getResetInfoPtr();

  // initialize serial interface
  Serial.begin(115200);
  Serial.println();
  delay(500); // give IDE time to open serial monitor
  Serial.println();
  Serial.println("PVMonitor " __DATE__ " " __TIME__);
  Serial.printf("Reset reason %d: %s\n", resetInfo->reason, ESP.getResetReason().c_str());
  if (REASON_EXCEPTION_RST == resetInfo->reason) {
	  Serial.printf("Fatal exception (%d)\n", resetInfo->exccause);
  }

  // initialize display
  if(!display.begin()) {
    Serial.println(F("\nERROR: Display allocation failed, rebooting..."));
    ESP.restart();
  }

  // initialize ESP WiFi connection
  Serial.println(F("Connecting to WiFi...")); display.print("Connecting WiFi..."); display.show();
  WiFi.persistent(false);  // do not store SSID&PSK in flash
  WiFi.mode(WIFI_STA);  // station mode
  WiFi.setHostname("PVMonitor");
  WiFi.setAutoReconnect(true);
  WiFi.begin(STASSID, STAPSK);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println(F("ERROR: Cannot establish a WiFi connection..."));
    ESP.restart();
  }

  Serial.print("IP: "); Serial.println(WiFi.localIP().toString());
  Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
  display.print(2, "RSSI: "); display.print(WiFi.RSSI()); display.print(" dBm");
  display.print(3, "IP: "); display.print(WiFi.localIP());
  display.show();

  Serial.print(F("Getting time..."));
  configTime(TIMEZONE, "pool.ntp.org");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    delay(500);
    Serial.print('.');
  }  
  Serial.println();
  sprintf(startupTS, "%d-%02d-%02d %02d:%02d:%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  Serial.println(startupTS);

  webserver.on("/", HTTP_GET, handleRoot);
  webserver.on("/status", HTTP_GET, handleStatus);
  webserver.on("/history", HTTP_GET, handleHistory);
  webserver.begin();

  victron1.init();
  while (!victron1.update()); // wait for initial values
  veLastTrackerModeWest = victron1.getTrackerMode();
  printf("Victron1 (West): %s\n", victron1.getTrackerModeStr());
  
  victron2.init();
  while (!victron2.update()); // wait for initial values
  veLastTrackerModeEast = victron2.getTrackerMode();
  printf("Victron2 (East): %s\n", victron2.getTrackerModeStr());
  
  // init mean current value
  for (int i=0; i<10; i++) {
    wcs.readCurrent();
  }

  if (!getShellyStatus(SHELLY_INVERTER_IP, &statusInv)) {
    Serial.printf("ERROR: ShellyPlug of Inverter not reachable at %s\n", SHELLY_INVERTER_IP);
    ESP.restart();
  }
  Serial.printf("Inverter: power=%.1fW, totalPower=%.1fWh, relay=%d\n", statusInv.power, statusInv.total_power, statusInv.relayOn);

  if (!getShellyStatus(SHELLY_3EM_IP, &status3EM)) {
    Serial.printf("ERROR: Shelly 3EM not reachable at %s\n", SHELLY_3EM_IP);
    ESP.restart();
  }
  Serial.printf("Shelly 3EM: power=%.1fW, totalPower=%.1fWh\n", status3EM.power, status3EM.total_power);

  // get EEPROM data
  if (!backup.init(resetInfo->reason, resetInfo->exccause, status3EM.total_power, statusInv.total_power)) {
    Serial.println(F("\nERROR: initializing EEPROM, rebooting..."));
    ESP.restart();
  }
  Serial.printf("RebootInfo: %d\n", backup.getRebootInfo());
  Serial.printf("ResetReason: %d (%d)\n", backup.getResetReason(), backup.getExceptionNo());
  display.clearLine(0); display.print((long)backup.getResetReason()); 
  display.print('/'); display.print(backup.getRebootInfo()); 
  display.print('/'); display.print(backup.getBackupStatus());
  display.show();

  if (statusInv.relayOn) { // inverter is on
    // get MaxPower from TruckiStick
    int maxPower = getTruckiMaxPower(TRUCKI_IP);
    if (-1 == maxPower) {
      Serial.println(F("\nERROR: Cannot get maxPower from TruckiStick..."));
      backup.saveRebootInfo(REBOOT_TRUCKI_MAXPOWER);
      ESP.restart();    
    }
    lastMaxPower = maxPower;
    sumBatPower = backup.getSumBatPower();
  } 
  else {
    lastMaxPower = TRUCKI_MIN_LOAD; 
    sumBatPower = 0.0; // inverter is off, battery is empty
  }
  lastInverterRelay = statusInv.relayOn;
 
  timeSunrise = backup.getSunrise();
  timeSunset  = backup.getSunset();
  Serial.printf("Backup: batSOC=%.1f, sunrise=%.1f, sunset=%.1f\n", sumBatPower, timeSunrise, timeSunset);

  Serial.println(F("Running..."));
}

void loop() {
  if (LOW == digitalRead(RESTART_BUTTON_PIN)) { // save values to EEPROM and restart, good before uploading new sketch
    Serial.println("Restart button pressed...");
	  backup.saveRebootData(sumBatPower, timeSunrise, timeSunset, REBOOT_BUTTON);
    ESP.restart();
  }
  
  int wifi_retry = 0;
  while ((WiFi.status() != WL_CONNECTED) && (wifi_retry < 10)) {
    wifi_retry++;
    WiFi.reconnect();
    Serial.print('.'); display.print('.'); display.show();
    delay(1000);
  }
  if (wifi_retry >= 5) {
    Serial.println(F("\nERROR: Failed to connect to WIFI, rebooting..."));
    backup.saveRebootData(sumBatPower, timeSunrise, timeSunset, REBOOT_NO_WIFI);
    ESP.restart();
  }

  webserver.handleClient();
  
  float batCurrent = wcs.readMeanCurrent();
  meanBatVoltage = (victron1.getBatVoltage() + victron2.getBatVoltage()) / 2.0;

  uint32_t currentTime = millis();
  uint32_t deltaTime = currentTime - lastLoop; 
  lastLoop = currentTime;

  sumBatPower += meanBatVoltage * batCurrent * deltaTime / 3.6E6; // 3600000ms per hour -> Wh
  if (sumBatPower < 0) sumBatPower = 0;
  if (sumBatPower > BATTERY_CAPACITY) sumBatPower = BATTERY_CAPACITY;

  //
  // update display and MPPT tracker values only once per second
  //
  if (currentTime - lastUpdate < 1000L) return;
  lastUpdate = currentTime;

  // read MPPT tracker values
  victron1.update();  // West-Seite
  victron2.update();  // Ost-Seite

  if (!getShellyStatus(SHELLY_INVERTER_IP, &statusInv)) {
    if (errorFlags & (1<<REBOOT_NO_INVERTER_STATUS)) { // flag was already set, reboot
        Serial.printf("ERROR: ShellyPlug of Inverter not reachable at %s\n", SHELLY_INVERTER_IP);
        backup.saveRebootData(sumBatPower, timeSunrise, timeSunset, REBOOT_NO_INVERTER_STATUS);
        ESP.restart();
    }
    else {
      errorFlags |= (1<<REBOOT_NO_INVERTER_STATUS); // set flag
      statusInv.relayOn = lastInverterRelay; // keep old relay status
    }
  }
  else {
	  errorFlags &= ~(1<<REBOOT_NO_INVERTER_STATUS); // reset flag
    inverterPower = statusInv.power;
  }
  
  // update values
  panelPower = victron1.getPanelPower() + victron2.getPanelPower();
  chargePower = panelPower - inverterPower;

  // get current time
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  float timeValue = float(timeinfo.tm_hour) + float(timeinfo.tm_min)/60.0;  // 2:30 -> 2,5

  // check for state change -> Sunrise
  VictronTrackerMode veTrackerModeEast = victron2.getTrackerMode();
  if (VTM_UNDEFINED == veLastTrackerModeEast) veLastTrackerModeEast = veTrackerModeEast;
  if (veLastTrackerModeEast != veTrackerModeEast) {
	  if ((VTM_OFF == veLastTrackerModeEast) && (VTM_OFF != veTrackerModeEast) && (panelPower > TRUCKI_MIN_LOAD)) { // war aus, ist jetzt an -> Sonnenaufgang
		  timeSunrise = timeValue;
  	  veLastTrackerModeEast = veTrackerModeEast;
	  }
  }

  // check for state change -> Sunset
  VictronTrackerMode veTrackerModeWest = victron1.getTrackerMode();
  if (VTM_UNDEFINED == veLastTrackerModeWest) veLastTrackerModeWest = veTrackerModeWest;
  if (veLastTrackerModeWest != veTrackerModeWest) {
    if ((VTM_MPPT == veLastTrackerModeWest) && (VTM_MPPT != veTrackerModeWest)) { // war an, ist jetzt aus -> Sonnenuntergang
      timeSunset = timeValue;
    }
    veLastTrackerModeWest = veTrackerModeWest;
  }

  // at 23:59 every day, store victron yield values into EEPROM
  if ((!daySaved) && (23 == timeinfo.tm_hour) && (59 == timeinfo.tm_min)) {
    time_t time = mktime(&timeinfo);
    PVData pvdata;
    pvdata.timestamp = time;
    pvdata.yield = victron1.getYieldToday() + victron2.getYieldToday();
    if (0 == pvdata.yield) {
      pvdata.yield = victron1.getYieldYesterday() + victron2.getYieldYesterday(); // we didn't make it on time?!
    }

    if (!getShellyStatus(SHELLY_3EM_IP, &status3EM)) {
      if (errorFlags & (1<<REBOOT_NO_SHELLY3EM_STATUS)) { // flag was already set, reboot
          Serial.printf("ERROR: Shelly 3EM not reachable at %s\n", SHELLY_3EM_IP);
          backup.saveRebootData(sumBatPower, timeSunrise, timeSunset, REBOOT_NO_SHELLY3EM_STATUS);
          ESP.restart();
      }
      else errorFlags |= (1<<REBOOT_NO_SHELLY3EM_STATUS); // set flag
    }
    else {
      errorFlags &= ~(1<<REBOOT_NO_SHELLY3EM_STATUS); // reset flag
      Serial.printf("Shelly 3EM: power=%.1fW, totalPower=%.1fWh\n", status3EM.power, status3EM.total_power);

      PVData pvdata_yesterday;
      if (backup.getPVData(pvdata_yesterday, backup.getLastPos())) {
        float sumHouseToday = status3EM.total_power - pvdata_yesterday.totalHouse;
        float sumInverterToday = statusInv.total_power - pvdata_yesterday.totalInverter;
        if (sumHouseToday+sumInverterToday > 0) {
          pvdata.autonomie = sumInverterToday/(sumHouseToday+sumInverterToday) * 100.0;
        }
        else pvdata.autonomie = 0;
      }
      else {
        Serial.printf("ERROR: Cannot get last PV data at index %d\n", backup.getLastPos());
        pvdata.autonomie = 0;
      }

      pvdata.totalHouse = status3EM.total_power;
      pvdata.totalInverter = statusInv.total_power;

      if (backup.savePVData(pvdata)) {
        daySaved = true;  // done for today
      }
    }
  } 
  else if (0 == timeinfo.tm_hour) {
    daySaved = false; // next day
  }

  display.clearLine(0, 9);
  display.print("SOC:"); display.print(int(sumBatPower)); display.print("Wh");
  
  display.clearLine(1);
  display.print("OST:"); display.print(victron2.getPanelPower()); display.print('W');
  display.setCursor(1, 9);
  display.print("Day:"); display.print(victron2.getYieldToday()); display.print("Wh");
   
  display.clearLine(2); 
  display.print("WST:"); display.print(victron1.getPanelPower()); display.print('W'); 
  display.setCursor(2, 9);
  display.print("Day:"); display.print(victron1.getYieldToday()); display.print("Wh");

  display.clearLine(3);
  display.print("BAT:"); display.print(meanBatVoltage); display.print("V, ");
  display.print(victron1.getOperationStateStr().c_str());
  
  display.show();

  //
  // every 5 minutes check if MPPT is still on and thus battery is not low
  //
  if ((-1 != lastInverterCheck) && (currentTime - lastInverterCheck < 5*60*1000L)) return;
  lastInverterCheck = currentTime;

  if (statusInv.relayOn) { // inverter relay is on
    uint16_t maxPower = TRUCKI_MIN_LOAD; // initial default
    if ((VOS_ABSORPTION == victron1.getOperationState()) || (VOS_FLOAT == victron1.getOperationState())) {
      sumBatPower = BATTERY_CAPACITY; // calibrate battery SOC
      if (VOS_ABSORPTION == victron1.getOperationState()) { // when trackers do "Absorption" battery is nearly full
        sumBatPower -= BATTERY_CAPACITY/10; // in Absorption are only ~10% missing
      }
      maxPower = TRUCKI_MAX_LOAD;
    }
    else { // still loading
      chargePower = panelPower - inverterPower;
      
      // dynamic adjust MaxPower-Value on TruckiStick of Inverter
      maxPower = TRUCKI_MIN_LOAD;
      if (meanBatVoltage > 49.0) { // no power saving
        float missingSOC = BATTERY_NIGHT_SOC - sumBatPower;
        if (missingSOC > 0.0) { // check if SOC is to low according to battery voltage
          if (meanBatVoltage >= 53.0) {
            sumBatPower = BATTERY_NIGHT_SOC; // SOC is to low
            missingSOC = 0.0;
          }
          else if ((meanBatVoltage >= 52.0) && (sumBatPower < BATTERY_NIGHT_SOC/2)) {
            sumBatPower = BATTERY_NIGHT_SOC/2;
            missingSOC = BATTERY_NIGHT_SOC - sumBatPower;
          }
        }

        if (panelPower > TRUCKI_MIN_LOAD) { // sunshine? -> dynamic adjust inverter, so that battery will still be loaded for the next night
          // check how much power is missing for the next night
          if (missingSOC > 0.0) {
            float time2Sunset;
            if (timeValue < timeSunset) {
              time2Sunset = timeSunset - timeValue;
            }
            else time2Sunset = 0;
            if (0 == time2Sunset) time2Sunset = 1;

            // Missing:1200W / 6h = 200W/h
            // PV:100W  -> 200W/100W  = 2    >1 -> 100%
            // PV:1000W -> 200W/1000W = 0,2 <=1 -> 20%
            float missingPower = missingSOC / time2Sunset;
            float batteryFeed = missingPower / float(panelPower);
            if (batteryFeed > 1.0) batteryFeed = 0.5; // default 50%, because feed is not reachable today

            maxPower = float(panelPower) * (1.0 - batteryFeed);
            maxPower = (maxPower/TRUCKI_MIN_LOAD)*TRUCKI_MIN_LOAD; // round down
            Serial.printf("PV=%ldW, missingSOC=%.1fWh, remainingSun=%.1fh, missingPower=%.1fW/h, batteryFeed=%.0f%%, maxPower=%dW\n", 
              panelPower, missingSOC, time2Sunset, missingPower, batteryFeed*100.0, maxPower);
          } 
          else { // battery is full for the night
            //if (sumBatPower > BATTERY_CAPACITY/2) {
              maxPower = TRUCKI_MAX_LOAD;
            /*
            }
            else {
              maxPower = int(float(panelPower + 10)/10)*10; // round up
            }
            */
            Serial.printf("PV=%ld, SOC=%.1f, maxPower=%d\n", panelPower, sumBatPower, maxPower);        
          }
        }
        else { // no sunshine
          float time2Sunrise;
          if (timeValue > timeSunrise) { // evening: eq. 21h > 6h
            time2Sunrise = timeSunrise+24.0 - timeValue;	// eq. 6+24 - 21 = 9h remaining
          }
          else time2Sunrise = timeSunrise - timeValue; // before dawn, 6 - 1 = 5h remaining
          if (0 == time2Sunrise) time2Sunrise = 1;
      
          if ((VTM_OFF == veTrackerModeEast) && (VTM_OFF == veTrackerModeWest)) { // night mode
            maxPower = sumBatPower / time2Sunrise;
            maxPower = int(float(maxPower + 10)/10)*10; // round up
            Serial.printf("time2Sunrise=%.1f, SOC=%.1f, maxPower=%d\n", time2Sunrise, sumBatPower, maxPower);
          }
          else {
            // check how much power is missing for the next night
            if ((missingSOC > 0.0) && (meanBatVoltage < 51.0)) {
              maxPower = TRUCKI_MIN_LOAD; // keep minimum while heavy clouds
            }
            else { // graceful add minimum to panel power
              if (time2Sunrise > 18.0) time2Sunrise = 4; // overwrite timespan at dawn
              maxPower = sumBatPower / time2Sunrise;
              maxPower = int(float(maxPower + 10)/10)*10; // round up
              Serial.printf("time2Sunrise=%.1f, SOC=%.1f, maxPower=%d\n", time2Sunrise, sumBatPower, maxPower);
            }
          }
        }
      } 
      else { // Reservebetrieb
        maxPower = TRUCKI_MIN_LOAD;  // do not add power from battery, everything >100W panel power is use for battery charging
      }
    }

    if (maxPower < TRUCKI_MIN_LOAD) maxPower = TRUCKI_MIN_LOAD; // safety check
    if (maxPower > TRUCKI_MAX_LOAD) maxPower = TRUCKI_MAX_LOAD; // safety check

    Serial.printf("PV: %ldW, ChargePower: %.2fW, Output: %.2fW, MaxPower: %dW\n", panelPower, float(panelPower)-statusInv.power, statusInv.power, maxPower);
    if (maxPower != lastMaxPower) {
      //Serial.printf("Set Inverter maxPower to %dW\n", maxPower);
      if (!setTruckiMaxPower(TRUCKI_IP, maxPower)) { // error
        if (errorFlags & (1<<REBOOT_TRUCKI_MAXPOWER)) { // flag was already set, reboot
            Serial.println(F("\nERROR: Cannot set maxPower for TruckiStick..."));
      		  backup.saveRebootData(sumBatPower, timeSunrise, timeSunset, REBOOT_TRUCKI_MAXPOWER);
            ESP.restart();
  	    }
        else {
          errorFlags |= (1<<REBOOT_TRUCKI_MAXPOWER); // set flag
          lastInverterCheck = -1; // overwrite waiting time interval
    		}
      }
      else { // ok
        errorFlags &= ~(1<<REBOOT_NO_SHELLY3EM_STATUS); // reset flag
        lastMaxPower = maxPower;
      }
    } 
  } 
  else { // inverter relay is off
    if (statusInv.relayOn != lastInverterRelay) { // state change
      sumBatPower = 0;
      lastInverterRelay = statusInv.relayOn;
    }
  }
}
