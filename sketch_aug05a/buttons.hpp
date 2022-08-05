#include "defs.hpp"
#include "oled.hpp"

class Buttons {
  public:
    enum Button {
      NONE,
      UP,
      DOWN,
      SELECT,
    };

    void begin() {
      pinMode(BUTTONS, INPUT);
      //  pinMode(FC, OUTPUT);       // AE-06 Settings
      //  digitalWrite(FC, HIGH);    // AE-06 Settings
    }

    Button pressed() {
      timeNow = millis();
      buttonValue = analogRead(BUTTONS);

     // Serial.print("AnalogRead: ");
     // Serial.println(buttonValue);              // Uncomment to print button value

      if (buttonValue > lastButtonValue + 100 || buttonValue < lastButtonValue - 100) {     // if the button state has changed
        lastDebounceTime = timeNow;     // Start the timer
      }

      if (buttonValue == 0) {
        pushed = false;           // Reset the button press
      }

      if ((timeNow - lastDebounceTime) > settings["debounce"]) {
        if (pushed == false && buttonValue > 0) {
          // Then handle button logic
          if (buttonValue > 3000 && buttonValue < 3300) {     // AE-01 Settings
            Serial.println("Button SELECT pressed");
            //  Oled.displayln("Button SELECT pressed");
            pushed = true;
            return Button::SELECT;
          }
          else if (buttonValue > 2000 && buttonValue < 2300) {     // AE-01 Settings
            Serial.println("Button DOWN pressed");
            // Oled.displayln("Button DOWN pressed");
            pushed = true;
            return Button::DOWN;
          }
          else if (buttonValue > 1200 && buttonValue < 1600) {    // AE-01 Settings
            Serial.println("Button UP pressed");
            // Oled.displayln("Button UP pressed");
            pushed = true;
            return Button::UP;
          }
          // Serial.println("Button NONE");
          return Button::NONE;
        }    
      }
      lastButtonValue = buttonValue;      // Update      
    }

  public:
    bool pushed = false;
    unsigned long timeNow, lastDebounceTime;
    int buttonValue, lastButtonValue;
};
