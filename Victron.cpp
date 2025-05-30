// NAME: victron.cpp
//

//#define DEBUG 1
//#define DEBUG_CHECKSUM_ERRORS 1

#include <ESP8266WiFi.h>
#include "Victron.h"
#define BUFLEN 512

#ifdef DEBUG_CHECKSUM_ERRORS
void dumpBuffer(char *buffer, int len) {
  Serial.printf("buflen=%d\n", len);
  for (int i=0; i<len; i++) {
    if ((i>0) && (i%16 == 0)) {
      Serial.print('\t');
      for (int j=i-16; j<i; j++) {
        if (isprint(buffer[j])) {
          Serial.print(buffer[j]);
        } else Serial.print('.');
      }
      Serial.println();
    }      

    uint8_t c = buffer[i];
    if (c < 0x10) Serial.print('0');
    Serial.print(c, HEX); Serial.print(' ');
  } 

  // fill with blanks
  for (int i=0; i<(16-len%16)%16; i++) {
    Serial.print(F("   "));
  }
  Serial.print('\t');
  // print last line
  for (int i=((len-1)/16)*16; i<len; i++) {
    if (isprint(buffer[i])) {
      Serial.print(buffer[i]);
    } else Serial.print('.');
  }
  Serial.println("\n-----------------------------------------------");
}
#endif

Victron::Victron(int rx_pin, int tx_pin) {
  victron = new SoftwareSerial(rx_pin, tx_pin);
  batVoltage = batCurrent = 0;
  panelVoltage = panelCurrent = panelPower = 0;
  yieldToday = yieldYesterday = 0;
  lastUpdate = 0L;
}

bool Victron::init() {
  victron->begin(19200);
  victron->enableIntTx(false);
  return true;
}

size_t Victron::readMessage(char *buffer, size_t buflen, uint32_t timeout /* =1500 */) {
  size_t len;
  uint32_t startTime = millis();
  do {
    while (0x0d != victron->read()) {
      yield(); // wait for start of message
      if (millis()-startTime > timeout) return 0;
    }
    len = 0;
    buffer[len++] = 0x0d;        
    
    int c2;
    do {
      c2 = victron->read();
    } while (-1 == c2);
    if (0x0a != c2) continue;
    buffer[len++] = c2;

    int c3;
    do {
      c3 = victron->read();
    } while (-1 == c3);
    if ('P' != c3) continue;
    buffer[len++] = c3;

    int c4;
    do {
      c4 = victron->read();
    } while (-1 == c4);
    if ('I' == c4) {
      buffer[len++] = c4;
      break;
    }
    yield();
  } while (millis()-startTime < timeout);
  if (4 != len) { // \x0d, \x0a, P, I
    Serial.println(F("TIMEOUT while reading from Victron!"));
    return 0;
  }

  startTime = millis();
  do {
    int c;
    do {
      c = victron->read();
      if (-1 != c) {
        buffer[len++] = c;
        break;
      }
      yield();
    } while (millis()-startTime < timeout);
    if (-1 == c) return 0;

    if ((len > 5) && !strncmp(&buffer[len-3], "PID", 3)) { // a second PID found!?
    #ifdef DEBUG
      Serial.println("Second PID found!");
    #endif
      len = 5;
    }
    
    if ((len > 10) && !strncmp(&buffer[len-10], "Checksum", 8)) { // stop reading after checksum field: "Checksum \x09 chksum", len=10
      break;
    }
  } while ((millis()-startTime < timeout) && (len < buflen));
  if (len >= buflen) {
    Serial.println("ERROR: Buffer overrun");
    return 0;
  }

  // valid messages have a checksum of 0
  uint8_t checksum = 0;
  for (int i=0; i<len; i++) {
    checksum += buffer[i];
  }

  if (0 == checksum) { // ok
    return len;
  }
  else { // invalid
  #ifdef DEBUG_CHECKSUM_ERRORS
    Serial.printf("ERROR checksum invalid! 0x%02x\n", checksum);
    dumpBuffer(buffer, len);
    for (int i=0; i<5; i++) {
      int c;
      do {
        c = victron->read();
      } while (-1 == c);
      Serial.printf("%02x, ", (uint8_t)c);
    }
    Serial.println();
  #endif
    return 0;
  } 
}

bool Victron::update() {
  char buffer[BUFLEN];
  int len = readMessage(buffer, BUFLEN);
  if (0 == len) return false;

  // process read messages
  int pos = 2; // position after \r and \n
  while (pos < len) {
    // check for HEX mode
    if ((pos < len-1) && (':' == buffer[pos]) && ('A' == buffer[pos+1])) {
      pos += 2;
      Serial.print("HEX: :A ");
      while ((pos < len) && ('\n' != buffer[pos])) {
        Serial.printf("%02x ", buffer[pos++]);
      }
      Serial.println();
      pos++;
      if (pos >= len) break; // done
    }

    // read message label
    char veLabel[10];
    int j;
    for (j=0; j<9; j++) {
      char c = buffer[pos+j];
      if ('\t' == c) break;
      veLabel[j] = c;
    }
    veLabel[j] = '\0'; 
    pos += j+1;
  #ifdef DEBUG
    Serial.printf("\nlabel=%s", veLabel);
  #endif
          
    // read message value
    char veValue[34];
    for (j=0; j<33; j++) {
      char c = buffer[pos+j];
      if ('\r' == c) break;
      veValue[j] = c;
    }
    veValue[j] = '\0';
    pos += j+2;
  #ifdef DEBUG
    Serial.print(F(", value=")); 
    if (!strcmp(veLabel, "Checksum")) {
      Serial.printf("0x%02x", veValue[0]);
    }
    else Serial.print(veValue);
  #endif

    // parse message
    uint32_t lval;  // universal helper variable

    if (!strcmp(veLabel, "Checksum")) { // end of message
      break;
    }
    else if (!strcmp(veLabel, "V")) { // main/battery voltage
      sscanf(veValue, "%ld", &lval);
      float mV = lval;
      if ((mV > 0) && (mV < 60000)) { // valid
        batVoltage = mV / 1000.0;
      }
  #ifdef DEBUG
      Serial.printf(", battery voltage=%.02fV", batVoltage);
  #endif
    } 
    else if (!strcmp(veLabel, "I")) { // main/battery current
      sscanf(veValue, "%ld", &lval);
      float mA = lval;
      if (mA < 35000) { // valid
        batCurrent = mA / 1000.0;
      }
  #ifdef DEBUG
      Serial.printf(", battery current=%.03fA", batCurrent);
  #endif
    } 
    else if (!strcmp(veLabel, "VPV")) { // panel voltage
      sscanf(veValue, "%ld", &lval);
      float mV = lval;
      if (mV < 100000) { // valid
        panelVoltage = mV / 1000.0;
      }
  #ifdef DEBUG
      Serial.printf(", panel voltage=%.02fV", panelVoltage);
  #endif
    } 
    else if (!strcmp(veLabel, "PPV")) { // panel power
      sscanf(veValue, "%ld", &lval);
      if (lval < 1200) { // valid
        panelPower = lval;
        if (panelVoltage > 0) {
          panelCurrent = panelPower / panelVoltage;
        }
        else panelCurrent = 0;
      } 
  #ifdef DEBUG
      Serial.printf(", panel power=%ldW, current=%.1fA", panelPower, panelCurrent);
  #endif
    }
    else if (!strcmp(veLabel, "CS")) { // state of operation
      sscanf(veValue, "%d", &operationState);
      switch (operationState) {
        case 0:  operationStateStr = "Off"; break;
        case 2:  operationStateStr = "Fault"; break;
        case 3:  operationStateStr = "Bulk"; break;
        case 4:  operationStateStr = "Absorption"; break;
        case 5:  operationStateStr = "Float"; break;
        case 7:  operationStateStr = "Equalize"; break;
        case 245: operationStateStr = "Starting Up"; break;
        case 247: operationStateStr = "Auto Equalize"; break;
        case 252: operationStateStr = "External Control"; break;
        default: operationState = VOS_UNDEFINED; operationStateStr = "MPPT State?"; break;
      }
  #ifdef DEBUG
      Serial.printf(", operation state=%s", operationStateStr);
  #endif
    } 
    else if (!strcmp(veLabel, "MPPT")) { // tracker operation mode
      sscanf(veValue, "%d", &trackerMode);
      switch (trackerMode) {
        case 0:  trackerModeStr = "Off"; break;
        case 1:  trackerModeStr = "Limited VA"; break;
        case 2:  trackerModeStr = "MPPT"; break;
        default: trackerMode = VTM_UNDEFINED; trackerModeStr = "Mode?"; break;
      }
  #ifdef DEBUG
      Serial.printf(", tracker mode=%s", trackerModeStr);
  #endif
    }
    else if (!strcmp(veLabel, "H20")) { // yield today
      sscanf(veValue, "%ld", &lval);
      yieldToday = lval * 10;
  #ifdef DEBUG      
      Serial.printf(", yield today=%dWh", yieldToday);
  #endif
    }
    else if (!strcmp(veLabel, "H22")) { // yield yesterday
      sscanf(veValue, "%ld", &lval);
      yieldYesterday = lval * 10;
  #ifdef DEBUG
      Serial.printf(", yield yesterday=%dWh", yieldYesterday);
  #endif
    }
    else if (!strcmp(veLabel, "PID")) { // the product_id is the start of a new message
      sscanf(veValue, "0x%04x", &productId);      
      if (!strcmp(veValue, "0xA058")) {
        productName = "SmartSolar MPPT 150|35";
      } else if (!strcmp(veValue, "0xA060")) {
        productName = "SmartSolar MPPT 100|20 48V";
      } else {
        productName = "Unknown device";
      }
    }
  #ifdef DEBUG // optional key/value pairs
    else if (!strcmp(veLabel, "FW")) { // firmware version
      Serial.print(F(", FW V")); 
      int i;
      for (i=0; i<strlen(veValue)-2; i++) Serial.print(veValue[i]);
      Serial.print('.');
      for (; i<strlen(veValue); i++) Serial.print(veValue[i]);
    }
    else if (!strcmp(veLabel, "SER#")) { // serial number
      Serial.print(F(", SER #")); Serial.print(veValue);
    }
    else if (!strcmp(veLabel, "H19")) { // yield total (user resettable counter)
      sscanf(veValue, "%ld", &lval);
      float yield = lval;
      Serial.print(F(", yield total=")); Serial.print(yield/100.0); Serial.print("kWh");
    }
    else if (!strcmp(veLabel, "H21")) { // max. power today
      Serial.print(F(", max. power today=")); Serial.print(veValue); Serial.print('W');
    }
    else if (!strcmp(veLabel, "H23")) { // max. power yesterday
      Serial.print(F(", max. power yesterday=")); Serial.print(veValue); Serial.print('W');
    }
    else if (!strcmp(veLabel, "HSDS")) { // historical data
      Serial.print(F(", day sequence number=")); Serial.print(veValue); 
    }
    else if (!strcmp(veLabel, "OR")) { // off reason
      Serial.print(F(", off reason=")); 
      if (!strcmp(veValue, "0x00000000")) Serial.print(F("Running"));
      else if (!strcmp(veValue, "0x00000001")) Serial.print(F("No input power"));
      else if (!strcmp(veValue, "0x00000002")) Serial.print(F("Switched off (power switch)"));
      else if (!strcmp(veValue, "0x00000004")) Serial.print(F("Switched off (device mode register)"));
      else if (!strcmp(veValue, "0x00000008")) Serial.print(F("Remote input"));
      else if (!strcmp(veValue, "0x00000010")) Serial.print(F("Protection active"));
      else if (!strcmp(veValue, "0x00000020")) Serial.print(F("Paygo"));
      else if (!strcmp(veValue, "0x00000040")) Serial.print(F("BMS"));
      else if (!strcmp(veValue, "0x00000080")) Serial.print(F("Engine shutdown detection"));
      else if (!strcmp(veValue, "0x00000100")) Serial.print(F("Analysing input voltage"));
      else Serial.print("Unknown?");
    }
    else if (!strcmp(veLabel, "ERR")) { // error code
      Serial.print(F(", error code="));
      int error;
      sscanf(veValue, "%d", &error);
      switch (error) {
        case 0: Serial.print(F("No error")); break;
        case 2: Serial.print(F("Battery voltage too high")); break;
        case 17: Serial.print(F("Charger temperature too high")); break;
        case 18: Serial.print(F("Charger over current")); break;
        case 19: Serial.print(F("Charger current reversed")); break;
        case 20: Serial.print(F("Bulk time limit exceeded")); break;
        case 21: Serial.print(F("Current sensor issue")); break;
        case 26: Serial.print(F("Terminals overheated")); break;
        case 28: Serial.print(F("Converter issue")); break;
        case 33: Serial.print(F("Input voltage too high (solar panel)")); break;
        case 34: Serial.print(F("Input current too high (solar panel)")); break;
        case 38: Serial.print(F("Input shutdown (due to excessive battery voltage)")); break;
        case 39: Serial.print(F("Input shutdown (due to current flow during off mode)")); break;
        case 65: Serial.print(F("Lost communication with one of devices")); break;
        case 66: Serial.print(F("Synchronised charging device configuration issue")); break;
        case 67: Serial.print(F("BMS connection lost")); break;
        case 68: Serial.print(F("Network misconfiguration")); break;
        case 116: Serial.print(F("Factory calibration data lost")); break;
        case 117: Serial.print(F("Invald/incompatible firmware")); break;
        case 119: Serial.print(F("User settings invalid")); break;
        default: Serial.print(F("Unknown?")); break;
      }
    }
    else if (!strcmp(veLabel, "LOAD")) { // load output state
      Serial.print(F(", load output state=")); Serial.print(veValue);
    }
    else if (!strcmp(veLabel, "IL")) { // load current
      sscanf(veValue, "%ld", &lval);
      float mA = lval;
      Serial.print(F(", load current=")); Serial.print(mA/1000.0); Serial.print('A');    
    }
  #endif
  }

  lastUpdate = millis();
  return true;  
}
