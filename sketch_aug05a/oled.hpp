#pragma once

#include <Wire.h>
#include "defs.hpp"
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

class Oled {
  public:

    void begin() {
     // Wire.begin(SDA_PIN, SCL_PIN);
      oled.begin(&Adafruit128x64, 0x3C);
      oled.setFont(System5x7);
      oled.setScrollMode(SCROLL_MODE_AUTO);
      oled.clear();
    }

    void displayln (char* text) {
      oled.println(text);
    }

    void display (char* text) {
      oled.print(text);
    }

  public:
    SSD1306AsciiWire oled;
};
