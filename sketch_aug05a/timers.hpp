#pragma once
#include "settings.hpp"
#include "stdlib.h"

struct timerData {
  char* timerName;
  bool timerState;
  unsigned long timerStartTime;
}

#define NUM_TIMERS 5
timerData[NUM_TIMERS] = {
  "timerZero", false, 0,
  "timerOne", false, 0,
  "timerTwo", false, 0,
  "timerThree", false, 0,
  "timerFour", false, 0
};

using namespace std;

class _Timer {
  public:

    void start(char* startMessage) {
      auto timeNow = millis();
      for (int i = 0; i < NUM_TIMERS; i++) {
        if (strcmp(startMessage, timerData[i].timerName) == 0) {
          // Check specific timer here
          if (timerData[i].timerState == false) {
            timerData[i].timerState = true;
            timerData[i].timerStartTime = timeNow;
            // Serial.print(startMessage);
            // Serial.println(" started");
            return;
          }
          else {
            // Serial.print(timerData[i].timerName);
            // Serial.println(" is already ON");
            return;
          }
        }
      }
    }

    void update() {
      auto timeNow = millis();
      for (int i = 0; i < NUM_TIMERS; i++) {
        // Only check if timer is ON
        if (timerData[i].timerState == true) {
          // convert char* to string to read settings
          string nameStr(timerData[i].timerName);
          if (timeNow - timerData[i].timerStartTime >= settings[nameStr]) {
            timerData[i].timerState = false;
            // Serial.print(timerData[i].timerName);
            // Serial.println(" has timed out");
            return;
          }
        }
      }
    }

    bool state(char* checkMessage) {
      for (int i = 0; i < NUM_TIMERS; i++) {
        if (strcmp(checkMessage, timerData[i].timerName) == 0) {
          return timerData[i].timerState;
        }
      }
      Serial.print("Message: ");
      Serial.print(checkMessage);
      Serial.println(" not recognised");
      return false;
    }


  public:

};

extern _Timer Timer;
