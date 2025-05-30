// NAME: Shelly.h
//
#ifndef SHELLY_H
#define SHELLY_H

//float getShellyPower(String ip); // return watts
//bool  getShellyRelay(String ip);

struct ShellyStatus {
  float power;        // in watt
  float total_power;  // in Wh
  bool  relayOn;      // true if on
};

bool getShellyStatus(String ip, ShellyStatus *status);

#endif /* SHELLY_H */
