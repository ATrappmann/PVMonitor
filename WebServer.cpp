// NAME: WebServer.cpp
//

//#define DEBUG 1

#include "Configuration.h"
#include "WebServer.h"
#include <time.h>

#if defined(ESP8266)
#include <ESP8266WebServer.h>
ESP8266WebServer webserver(80);
#endif

#include "Victron.h"
#include "Shelly.h"
#include "PersistentData.h"

extern char startupTS[];
extern uint32_t lastUpdate, lastInverterCheck;
extern Victron victron1, victron2;
extern float timeSunrise;
extern float timeSunset;
extern float sumBatPower;
extern long panelPower;
extern float inverterPower;
extern long chargePower;
extern uint16_t lastMaxPower;
extern float meanBatVoltage;
extern PersistentData backup;
extern ShellyStatus statusInv;
extern ShellyStatus status3EM;

void handleRoot() {
  String answer = "Hello from ";
  answer += WiFi.getHostname();
  answer += "\ntime: ";

  time_t now = time(nullptr);
  String time = String(ctime(&now));
  time.trim();
  answer += time;
  
  answer += "\nstartupTS: ";
  answer += startupTS;
  answer += "\nresetReason: ";
  answer += backup.getResetReason();
  answer += " - ";
  answer += ESP.getResetReason();
  if (REASON_EXCEPTION_RST == backup.getResetReason()) {
    answer += " #";
    answer += backup.getExceptionNo(); 
  }
  answer += "\nrebootInfo: ";
  answer += backup.getRebootInfo();
  answer += " - ";
  switch(backup.getRebootInfo()) {
    case REBOOT_UNINITIALIZED: answer += "Uninitialized EEPROM"; break;
    case REBOOT_INITIALIZED: answer += "Initialized EEPROM"; break;
    case REBOOT_NO_WIFI: answer += "No WiFi connection"; break;
    case REBOOT_NO_INVERTER_STATUS: answer += "No Inverter status"; break;
    case REBOOT_TRUCKI_MAXPOWER: answer += "MaxPower not available"; break;
    case REBOOT_BUTTON: answer += "Reboot button"; break;
    case REBOOT_NO_SHELLY3EM_STATUS: answer += "No Shelly3EM status"; break;
    default: answer += "Unknown"; break;
  }
  answer += "\nbackupStatus: ";
  answer += backup.getBackupStatus();
  answer += "\n";

  answer += "\r\n";

  webserver.sendHeader("Connection", "close");
  webserver.send(200, "text/plain", answer);
}

void handleStatus() {
  String answer = "{";

  answer += "\"currentTS\":\"";
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    // 2024-06-15 18:00:00 -> len=19
    char buffer[20];
    sprintf(buffer, "%d-%02d-%02d %02d:%02d:%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    answer += buffer;
  }
  else answer += "n/a";

  uint32_t currentTime = millis();

  answer += "\",\"lastInverterCheck\":";
  answer += (currentTime - lastInverterCheck) / 1000; // in seconds

  answer += ",\"timeSunrise\":";
  answer += timeSunrise;
  
  answer += ",\"timeSunset\":";
  answer += timeSunset;
  
  answer += ",\"bat_soc\":";
  answer += sumBatPower;
  
  answer += ",\"panelPower\":";
  answer += panelPower;

  answer += ",\"chargePower\":";
  answer += chargePower;

  answer += ",\"inverterPower\":";
  answer += inverterPower;

  answer += ",\"maxPower\":";
  answer += lastMaxPower;

  answer += ",\"meanBatVoltage\":";
  answer += meanBatVoltage;

  if (!getShellyStatus(SHELLY_3EM_IP, &status3EM)) {
    Serial.printf("ERROR: Shelly 3EM not reachable at %s\n", SHELLY_3EM_IP);
  }

  answer += ",\"housePower\":";
  answer += status3EM.power;
  
  PVData pvdata;
  if (!backup.getPVData(pvdata, backup.getLastPos())) {
    Serial.printf("ERROR: Cannot get last PV data at index %d\n", backup.getLastPos());
  }

  answer += ",\"sumHouseToday\":";
  answer += status3EM.total_power - pvdata.totalHouse;
  answer += ",\"sumInverterToday\":";
  answer += statusInv.total_power - pvdata.totalInverter;

  answer += ",\"startupTS\":\"";
  answer += startupTS;

  answer += "\",\"resetReason\":";
  answer += backup.getResetReason();

  answer += ",\"rebootInfo\":";
  answer += backup.getRebootInfo();
  
  answer += ",\"age\":";
  answer += currentTime - lastUpdate;
  
  answer += ",\"rssi\":";
  answer += WiFi.RSSI();

  answer += ",\"victron_mppt_1\":{";
  answer += "\"product_id\":";
  answer += victron1.getProductId();
  answer += ",\"product_name\":\"";
  answer += victron1.getProductName();
  answer += "\",\"bat_voltage\":";
  answer += victron1.getBatVoltage();
  answer += ",\"bat_current\":";
  answer += victron1.getBatCurrent();
  answer += ",\"panel_voltage\":";
  answer += victron1.getPanelVoltage();
  answer += ",\"panel_current\":";
  answer += victron1.getPanelCurrent();
  answer += ",\"panel_power\":";
  answer += victron1.getPanelPower();
  answer += ",\"yield_today\":";
  answer += victron1.getYieldToday();
  answer += ",\"operation_state\":\"";
  answer += victron1.getOperationState();
  answer += "\",\"tracker_mode\":\"";
  answer += victron1.getTrackerMode();
  answer += "\",\"age\":";
  answer += currentTime - victron1.getLastUpdate();
  answer += "}";

  answer += ",\"victron_mppt_2\":{";
  answer += "\"product_id\":";
  answer += victron2.getProductId();
  answer += ",\"product_name\":\"";
  answer += victron2.getProductName();
  answer += "\",\"bat_voltage\":";
  answer += victron2.getBatVoltage();
  answer += ",\"bat_current\":";
  answer += victron2.getBatCurrent();
  answer += ",\"panel_voltage\":";
  answer += victron2.getPanelVoltage();
  answer += ",\"panel_current\":";
  answer += victron2.getPanelCurrent();
  answer += ",\"panel_power\":";
  answer += victron2.getPanelPower();
  answer += ",\"yield_today\":";
  answer += victron2.getYieldToday();
  answer += ",\"operation_state\":\"";
  answer += victron2.getOperationState();
  answer += "\",\"tracker_mode\":\"";
  answer += victron2.getTrackerMode();
  answer += "\",\"age\":";
  answer += currentTime - victron2.getLastUpdate();
  answer += "}";

  answer += "}";
  webserver.sendHeader("Cache-Control", "no-cache");
  webserver.sendHeader("Connection", "close");
  webserver.send(200, "application/json", answer);
}

void handleHistory() {
  String answer = "<html>\n<head>\n";
  answer += "<style>table, th, td { border:1px solid black; }</style>\n";
  answer += "<title>VictronMonitor History</title></head>\n";
  answer += "<body>\n<h1>VictronMonitor History</h1>\n";
  answer += "<table>\n<tr><th>Datum</th><th>PV-Leistung</th><th>Autonomie</th><th>totalHouse</th><th>totalInverter</th></tr>\n";

  uint8_t len = backup.getNumEntries();  
  for (int idx=0; idx<len; idx++) {
    PVData data;
    if (backup.getPVData(data, idx)) {
      String timeStr = String(ctime(&data.timestamp));
      answer += "<tr><td>";
    #ifdef DEBUG
      answer += data.timestamp;
      answer += "</td><td>";
    #endif
      answer += timeStr;
      answer += "</td><td>";
      answer += data.yield;
      answer += "Wh</td><td>";
      answer += data.autonomie;
      answer += "%</td><td>";
      answer += data.totalHouse;
      answer += "Wh</td><td>";
      answer += data.totalInverter;
      answer += "Wh</td></tr>\n";
    }
    else {
      answer +="<tr><td colspan=5>n/a</td></tr>\n";
    }
  }

  answer += "</table>\n";
  answer += "</body></html>\n";

  webserver.sendHeader("Connection", "close");
  webserver.send(200, "text/html", answer);
}
