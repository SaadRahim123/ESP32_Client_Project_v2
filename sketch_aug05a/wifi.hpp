#pragma once
#include <WiFi.h>
#include <WiFiManager.h>
#include "oled.hpp"

class _Wifi {
  public:

    bool connect() {

      //WiFi.mode(WIFI_AP); // explicitly set mode, esp defaults to STA+AP
      //wifiManager.resetSettings();

      wifiManager.setConfigPortalBlocking(true);
      wifiManager.setConfigPortalTimeout(180);
      delay(500);
      if (wifiManager.autoConnect("ESP32")) {  //automatically connect using saved credentials if connection fails it starts an access point
        Serial.print("Connected to IP address: ");
        Serial.print(WiFi.localIP());
        Serial.print(" Wifi Strength: ");
        Serial.print(getRssiAsQuality());
        Serial.print(" %");
        Serial.print(" Dbm: ");
        Serial.println(WiFi.RSSI());

        return true;
      }
      else {
        Serial.println("Access Point Started");
//      Oled.display("Access Point Started");
//      Oled.displayln("192.168.4.1");
        return false;
      }
    }

    char* getRssiAsQuality() {
      int rssi = WiFi.RSSI();
      int quality = 0;
      if (rssi <= -100) {
        quality = 0;
      } else if (rssi >= -50) {
        quality = 100;
      } else {
        quality = 2 * (rssi + 100);
      }

      sprintf(wifiStr, "%02d", quality);
      //Serial.println(wifiStr);    // Uncomment to serial print
      return wifiStr;
    }

    void resetAP() {
      // wifiManager.resetSettings();
    }

  public:
    WiFiManager wifiManager;
    char wifiStr[5];

};

extern _Wifi Wifi;
