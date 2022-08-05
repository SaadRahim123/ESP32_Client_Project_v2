#pragma once
#include <esp_task_wdt.h>
#include "defs.hpp"

class WDT {
  public:

    void begin() {
      esp_task_wdt_init(WDT_TIMEOUT, true);                       //enable panic so ESP32 restarts
      esp_task_wdt_add(NULL);                                     //add current thread to WDT watch
      Serial.println("Starting WDT");
    }

    void update() {
      long timeNow = millis();
      if (timeNow - lastReset >= (WDT_TIMEOUT - 1) * 1000 ) {     // Reset 1 second before timeout
        esp_task_wdt_reset();                                     // Reset the watchdog
        lastReset = timeNow;
        //Serial.println("Resetting WDT...");
      }
    }

  public:
    unsigned long timeNow, lastReset;
};
