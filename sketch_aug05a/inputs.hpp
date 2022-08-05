#pragma once
#include "settings.hpp"

struct inputData {
  char* inputName;
  int inputPin;
  bool inputState;
  unsigned long inputStartTime;
}

#define NUM_INPUTS 8

inputData[NUM_INPUTS] = {
  "inZero", 14, false, 0,              // AE01 Pin 18
  "inOne", 32, false, 0,              // AE01 Pin 39
  "inTwo", 21, false, 0,              // AE01 Pin 34
  "inThree", 22, false, 0,              // AE01 Pin 35
  "inFour", 33, false, 0,              // AE01 Pin 19
  "inFive", 34, false, 0,              // AE01 Pin 21
  "inSix", 35, false, 0,              // AE01 Pin 22
  "inSeven", 25, false, 0              // AE01 Pin 23
};

class _Input {
  public:

    void begin() {
      // Set pinMode for all pins
      for (int i = 0; i < NUM_INPUTS; i++) {
        pinMode(inputData[i].inputPin, INPUT_PULLUP);
      }
    }

    bool state(char* messageName) {
      for (int i = 0; i < NUM_OUTPUTS; i++) {
        if (strcmp(messageName, inputData[i].inputName) == 0) {
          return inputData[i].inputState;
        }
      }
    }

    void update(char stringToReturn[]) {
      for (int i = 0; i < NUM_INPUTS; i++) {
        auto timeNow = millis();
        int inputReading = digitalRead(inputData[i].inputPin);

        // Check if digitalRead matches on state and that it is has been off before
        if (inputReading != inputData[i].inputState) {     // If pin is equal to triggered state
          inputData[i].inputStartTime = timeNow;  // start timer
        }
        // Check if still on after debounce time
        if (timeNow - inputData[i].inputStartTime >= settings["inputDelay"]) {
          inputData[i].inputState = !inputData[i].inputState;
          // Only print and publish if true
          //  if (inputData[i].inputState == true) {                 // Uncomment if true only needed
          // Publish MQTT input trigger high message
 

          // Join string and int delimited by a "/"
          char msg[20];
          snprintf(msg, sizeof msg, "%s/%d", inputData[i].inputName, inputData[i].inputState);          // Copy char array to a char pointer so return a pointer

          strcpy(stringToReturn, msg);
         // Serial.print(msg);
         // Serial.println("End");
          // return result;
       //   return inputData[i].inputName;
          //   }
        }
      }
      strcpy(stringToReturn, "none");
    }

  public:

};

extern _Input Input;
