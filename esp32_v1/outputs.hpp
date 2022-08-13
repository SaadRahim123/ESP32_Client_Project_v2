#pragma once
#include "settings.hpp"
#include "stdlib.h"

struct outputData {
  char* outputName;
  int outputPin;
  bool outputState;
  bool prevState;
  unsigned long outputStartTime;
}

// name, pin, state, start time
#define NUM_OUTPUTS 8

outputData[NUM_OUTPUTS] = {
  "relayZero", 14, false, false, 0,              // AE01 Pin 14, AE01 Pin 2
  "relayOne", 12, true, false, 0,              // AE01 Pin 12, AE01 Pin 4
  "relayTwo", 13, false, false, 0,              // AE01 Pin 13, AE01 Pin 12
  "relayThree", 15, false, false, 0,              // AE01 Pin 15, AE01 Pin 13
  "relayFour", 2, false, false, 0,
  "relayFive", 33, false, false, 0,
  "transZero", 27, false, false, 0,
  "transOne", 26, false, false, 0
};

using namespace std;

class _Output {
  public:

    void begin() {
      // Set pinMode for all pins
      for (int i = 0; i < NUM_OUTPUTS; i++) {
        pinMode(outputData[i].outputPin, OUTPUT);
        // Write default states to pins
        digitalWrite(outputData[i].outputPin, outputData[i].outputState);
      }
    }

    void process (char* payloadAsChar, char* payloadData) {
      int payloadDataAsInt = atoi(payloadData);
      if (payloadDataAsInt == 1) {
        start(payloadAsChar);
        return;
      }
      if (payloadDataAsInt == 0) {
        stop(payloadAsChar);
        return;
      }
    }

    void start (char* messageName) {
      // Serial.println(messageName);
      auto timeNow = millis();
      for (int i = 0; i < NUM_OUTPUTS; i++) {
        if (strcmp(messageName, outputData[i].outputName) == 0) {
          if (outputData[i].outputState == false) {
            outputData[i].outputState = true;
            outputData[i].outputStartTime = timeNow;
        //    Serial.print(messageName);
        //    Serial.println(" started");
            return;
          }
        }
      }
    }

    void stop (char* messageName) {
      // Serial.println(messageName);
      for (int i = 0; i < NUM_OUTPUTS; i++) {
        if (strcmp(messageName, outputData[i].outputName) == 0) {
          if (outputData[i].outputState == true) {
            outputData[i].outputState = false;
         //   Serial.print(messageName);
          //  Serial.println(" stopped");
            return;
          }
        }
      }
    }

    void update() {
      auto timeNow = millis();
      // check if output is on before checking if its a 0
      for (int i = 0; i < NUM_OUTPUTS; i++) {

        if (outputData[i].outputState == true) {
          // convert char* to string
          string nameStr(outputData[i].outputName);
          // If output on time setting is 0 then ignore the update
          if (settings[nameStr] != 0) {
            if (timeNow - outputData[i].outputStartTime >= settings[nameStr]) {
              outputData[i].outputState = false;
            }
          }
        }
        if (outputData[i].outputState != outputData[i].prevState) {
          // Update pins only if state has changed
          digitalWrite(outputData[i].outputPin, outputData[i].outputState);
          Serial.print("Output: ");
          Serial.print(outputData[i].outputName);
          Serial.print(" State: ");
          Serial.println(outputData[i].outputState);
          // Update previous state
          outputData[i].prevState = outputData[i].outputState;
        }
      }
    }


  public:
    bool prevState = false;
};

extern _Output Output;
