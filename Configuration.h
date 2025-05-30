// NAME: Configuration.h
//
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#define RESTART_BUTTON_PIN	D3
#define VICTRON_EAST_RXPIN  D5
#define VICTRON_WEST_RXPIN  D7
#define CURRENT_SENSOR_PIN  A0

#define SHELLY_INVERTER_IP "<IP-Adresse>" 	// ShellyPlug of Inverter
#define TRUCKI_IP          "<IP-Adresse>"	// TruckiStick	
#define SHELLY_3EM_IP      "<IP-Adresse>" 	// Shelly 3EM (3 phase house energy meter)

#define TRUCKI_MIN_LOAD  50 // W
#define TRUCKI_MAX_LOAD 800 // W

#define BATTERY_CAPACITY  5120  // Total battery capacity in Wh
#define BATTERY_NIGHT_SOC 1000  // Battery capacity limit for night mode in Wh

#define SCREEN_WIDTH  128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#ifndef STASSID
#define STASSID "<SSID-Name>"
#define STAPSK  "<WiFi-Passord>"
#endif

#endif
