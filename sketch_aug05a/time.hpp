#pragma once

class _Time {
  public:

    char* getUptime() {
      auto timeNow = millis();

      upSeconds = timeNow / 1000;
      seconds = upSeconds % 60;
      minutes = (upSeconds / 60) % 60;
      hours = (upSeconds / (60 * 60)) % 24;
      days = (rollover * 50) + (upSeconds / (60 * 60 * 24)); // 50 day rollover
      
      sprintf(timeStr, "%02d %02d:%02d:%02d",  days, hours, minutes, seconds);
      Serial.println(timeStr);    // Uncomment to serial print
      return timeStr;
    }

  public:
    char timeStr[30];
    unsigned long lastUpdate, upSeconds;
    int hours, minutes, seconds, days , rollover;


};

extern _Time Time;
