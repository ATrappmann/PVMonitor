// NAME: PersistentData.h
//
#ifndef PERSISTENT_DATA_H
#define PERSISTENT_DATA_H

#define NUM_BACKUP_DAYS 30

#define EPDDATA_ADDR  0
#define EPVDATA_ADDR  EPDDATA_ADDR + sizeof(PersistentDataStruct)
#define EREBOOT_ADDR  EPVDATA_ADDR + NUM_BACKUP_DAYS*sizeof(PVData)

struct PVData {
  time_t  timestamp;
  long    yield;          // Wh
  float   autonomie;      // in percent
  float   totalHouse;     // Wh
  float   totalInverter;  // Wh
};

enum RebootInfo {
  REBOOT_UNKNOWN = -1,
  REBOOT_UNINITIALIZED = 0,
  REBOOT_INITIALIZED,
  REBOOT_NO_WIFI,
  REBOOT_NO_INVERTER_STATUS,
  REBOOT_TRUCKI_MAXPOWER,
  REBOOT_BUTTON,
  REBOOT_NO_SHELLY3EM_STATUS
};

struct RebootData {
  float       sumBatPower;
  float       sunrise;
  float       sunset;
  uint32_t    resetReason; // rst_reason
  uint32_t    exceptionNo; // exccause;
  RebootInfo  rebootInfo;
};

struct PersistentDataStruct {
  uint16_t  magic;
  uint8_t   numEntries;
  uint8_t   pos;
//  PVData    pvdata[NUM_BACKUP_DAYS];
};

class PersistentData {
private:
  PersistentDataStruct  data;
  RebootData            reboot;
  bool                  backupStatus; // true if EEPROM data was valid, false if initialized

public:
  PersistentData();
  bool init(uint32_t resetReason, uint32_t exceptionNo, float totalHouse, float totalInverter);

  bool savePVData(PVData& pvData);
  bool getPVData(PVData& pvData, uint8_t index);
  uint8_t getNumEntries() const { return data.numEntries; }
  uint8_t getLastPos() const { return data.pos; }
  
  bool saveRebootData(float sumBatPower, float sunrise, float sunset, RebootInfo rebootInfo);
  bool saveRebootInfo(RebootInfo rebootInfo);

  float getSumBatPower() const { return reboot.sumBatPower; }
  float getSunrise() const { return reboot.sunrise; }
  float getSunset() const { return reboot.sunset; }

  uint32_t getResetReason() const { return reboot.resetReason; }
  uint32_t getExceptionNo() const { return reboot.exceptionNo; }
  RebootInfo getRebootInfo() const { return reboot.rebootInfo; }

  bool getBackupStatus() const { return backupStatus; }
};

#endif /* PERSISTENT_DATA_H */
