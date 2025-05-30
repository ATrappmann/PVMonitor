// NAME: Victron.h
//
#ifndef VICTRON_H
#define VICTRON_H

#include <String.h>
#include <SoftwareSerial.h>

enum VictronOperationState {
  VOS_UNDEFINED = -1,
  VOS_OFF = 0,          // MPPT, Inverter & Charger
  //VOS_LOW_POWER = 1,  // Inverter (Load search)
  VOS_FAULT = 2,        // MPPT, Inverter & Charger
  VOS_BULK = 3,         // MPPT & Charger
  VOS_ABSORPTION = 4,   // MPPT & Charger
  VOS_FLOAT = 5,        // MPTT & Charger
  //VOS_STORAGE = 6,    // Charger
  VOS_EQUALIZE = 7,     // MPPT (Equalize manual)
  //VOS_INVERTING = 9,  // Inverter
  //VOS_POWER_SUPPLY = 11, // Charger
  VOS_STARTING_UP = 245,  // MPPT
  //VOS_REPEATED_ABSORPTION = 246,  // Charger
  VOS_AUTO_EQUALIZE = 247,  // MPPT & Charger
  //VOS_BATTERY_SAFE = 248, // Charger
  VOS_EXTERNAL_CONTROL = 252
};

enum VictronTrackerMode {
  VTM_UNDEFINED = -1,
  VTM_OFF = 0,
  VTM_LIMITED_VA = 1,
  VTM_MPPT = 2
};

class Victron {
private:
  SoftwareSerial *victron;

  uint16_t productId;
  String productName;
  
  float batVoltage;
  float batCurrent;
  float panelVoltage;
  float panelCurrent;
  long  panelPower;
  long  yieldToday;
  long  yieldYesterday;

  VictronOperationState operationState;
  String operationStateStr;
  VictronTrackerMode trackerMode;
  String trackerModeStr;

  uint32_t lastUpdate;

public:
  Victron(int rx_pin, int tx_pin);
  bool init();
  bool update();
  
  uint16_t getProductId() const { return productId; }
  String getProductName() const { return productName; }
  float getBatVoltage() const { return batVoltage; }
  float getBatCurrent() const { return batCurrent; }
  float getPanelVoltage() const { return panelVoltage; }
  float getPanelCurrent() const { return panelCurrent; }
  long  getPanelPower() const { return panelPower; }
  long  getYieldToday() const { return yieldToday; }
  long  getYieldYesterday() const { return yieldYesterday; }
  VictronOperationState getOperationState() const { return operationState; }
  String getOperationStateStr() const { return operationStateStr; }
  VictronTrackerMode getTrackerMode() const { return trackerMode; }
  String getTrackerModeStr() const { return trackerModeStr; }
  uint32_t getLastUpdate() const { return lastUpdate; }

protected:
  size_t readMessage(char *buffer, size_t buflen, uint32_t timeout = 1500L); // return message length or 0 if error
};

#endif /* VICTRON_H */
