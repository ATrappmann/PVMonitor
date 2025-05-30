// NAME: PersistentData.cpp
//

//#define DEBUG 1

#include <ESP8266WiFi.h>
#include "PersistentData.h"
#include <ESP_EEPROM.h>

#include "Victron.h"
extern Victron victron1;
extern Victron victron2;

#define MAGIC 0x2206

PersistentData::PersistentData() {
  EEPROM.begin(sizeof(data) + NUM_BACKUP_DAYS*sizeof(PVData) + sizeof(reboot));
  data.magic = 0;
}

bool PersistentData::init(uint32_t resetReason, uint32_t exceptionNo, float totalHouse, float totalInverter) {
  #ifdef DEBUG
  Serial.printf("EEPROM: sizeof(data)=%d, sizeof(reboot)=%d, reboot_addr=%d\n", sizeof(data), sizeof(reboot), EREBOOT_ADDR);
  #endif
  EEPROM.get(EPDDATA_ADDR, data);
  EEPROM.get(EREBOOT_ADDR, reboot);
  if ((MAGIC != data.magic) || (REBOOT_UNINITIALIZED == reboot.rebootInfo)) {
  #ifdef DEBUG
    Serial.println(F("Initializing EEPROM..."));
  #endif

    struct tm timeinfo;
    getLocalTime(&timeinfo);
    timeinfo.tm_hour = 23;
    timeinfo.tm_min = 59;
    timeinfo.tm_sec = 59;
    timeinfo.tm_mday--; // yesterday

    PVData pvdata;
    pvdata.timestamp = mktime(&timeinfo);
    pvdata.yield = victron1.getYieldYesterday() + victron2.getYieldYesterday();    
    pvdata.autonomie = 0;
    pvdata.totalHouse = totalHouse;
    pvdata.totalInverter = totalInverter;
    EEPROM.put(EPVDATA_ADDR, pvdata);

    data.magic = MAGIC;    
    data.numEntries = 1;
    data.pos = 0;
    EEPROM.put(EPDDATA_ADDR, data);

    reboot.sumBatPower = 0.0;
    reboot.sunrise = 6.0;
    reboot.sunset = 18.0;
    reboot.resetReason = resetReason;
    reboot.exceptionNo = exceptionNo;
    reboot.rebootInfo = REBOOT_INITIALIZED;
    EEPROM.put(EREBOOT_ADDR, reboot);
  
    backupStatus = false; // new initialized

    if (!EEPROM.commit()) {
      Serial.println(F("ERROR: Cannot commit EEPROM data!")); 
      return false;
    }
    else return true;
  }
  else {
    reboot.resetReason = resetReason;
    reboot.exceptionNo = exceptionNo;
    EEPROM.put(EREBOOT_ADDR, reboot);
    
    backupStatus = true; // has valid data
    if (!EEPROM.commit()) {
      Serial.println(F("ERROR: Cannot commit EEPROM data!")); 
      return false;
    }
    else return true;
  }
}

bool PersistentData::savePVData(PVData& pvData) {
//  if (NULL == pvData) return false; // no data
  if (MAGIC != data.magic) return false; // not initialized

  data.pos = (data.pos + 1) % NUM_BACKUP_DAYS;
  /*
  data.pvdata[data.pos].timestamp = pvData->timestamp;
  data.pvdata[data.pos].yield = pvData->yield;
  data.pvdata[data.pos].autonomie = pvData->autonomie;
  data.pvdata[data.pos].totalHouse = pvData->totalHouse;
  data.pvdata[data.pos].totalInverter = pvData->totalInverter;
  */
  if (data.numEntries < NUM_BACKUP_DAYS) data.numEntries++;

  EEPROM.put(EPVDATA_ADDR + data.pos*sizeof(PVData), pvData);
  EEPROM.put(EPDDATA_ADDR, data);
  return EEPROM.commit();
}

bool PersistentData::getPVData(PVData& pvData, uint8_t index) {
//  if (NULL == pvData) return false; // no data
  if (MAGIC != data.magic) return false; // not initialized
  if (index >= data.numEntries) return false;

  EEPROM.get(EPVDATA_ADDR + index*sizeof(PVData), pvData);
  /*
  pvData->timestamp = data.pvdata[index].timestamp;
  pvData->yield = data.pvdata[index].yield;
  pvData->autonomie = data.pvdata[index].autonomie;
  pvData->totalHouse = data.pvdata[index].totalHouse;
  pvData->totalInverter = data.pvdata[index].totalInverter;
  */
  return true;
}

bool PersistentData::saveRebootData(float sumBatPower, float sunrise, float sunset, RebootInfo rebootInfo) {
  if (MAGIC != data.magic) {
    Serial.printf("ERROR: EEPROM has wrong magic number %04x\n", data.magic);
    return false; // not initialized
  }

  reboot.sumBatPower = sumBatPower;
  reboot.sunrise = sunrise;
  reboot.sunset = sunset;
  if (REBOOT_UNKNOWN != rebootInfo) {
    reboot.rebootInfo = rebootInfo;
  }
  EEPROM.put(EREBOOT_ADDR, reboot);
  return EEPROM.commit();
}

bool PersistentData::saveRebootInfo(RebootInfo rebootInfo) {
  if (MAGIC != data.magic) {
    Serial.printf("ERROR: EEPROM has wrong magic number %04x\n", data.magic);
    return false; // not initialized
  }

  if (REBOOT_UNKNOWN != rebootInfo) {
    reboot.rebootInfo = rebootInfo;
    EEPROM.put(EREBOOT_ADDR, reboot);
    return EEPROM.commit();
  }
  else return false;
}