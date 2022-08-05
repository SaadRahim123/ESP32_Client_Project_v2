/* 
 *  Add variables to logSD() to pass data
 */

# pragma once
#include <SPI.h>
#include "FS.h"
#include "SD.h"
#include "defs.hpp"

class Sdcard {
  public:

    void begin() {
      // Initialize SD card
      SD.begin(SD_CS_PIN);
      if (!SD.begin(SD_CS_PIN)) {
        Serial.println("Card Mount Failed");
        return;
      }
      uint8_t cardType = SD.cardType();
      if (cardType == CARD_NONE) {
        Serial.println("No SD card attached");
        return;
      }
      Serial.println("Initializing SD card...");
      if (!SD.begin(SD_CS_PIN)) {
        Serial.println("ERROR - SD card initialization failed!");
        return;    // init failed
      }

      File file = SD.open("/data.txt");
      if (!file) {
        Serial.println("File doens't exist");
        Serial.println("Creating file...");
        writeFile(SD, "/data.txt", "Date, Time, Temperature, Humidity \r\n");
      }
      else {
        Serial.println("File already exists");
      }
      file.close();
    }

    // Log to SD card
    void logSD(int dayStamp, int timeStamp) {
    //  dataMessage =  String(dayStamp) + "," + String(timeStamp) + "," + String(Temperature) + "," + String(Humidity) + "\r\n";
      Serial.print("Save data: ");
      Serial.println(dataMessage);
      appendFile(SD, "/data.txt", dataMessage.c_str());
    }

    // Write to the SD card (DON'T MODIFY THIS FUNCTION)
    void writeFile(fs::FS &fs, const char * path, const char * message) {
      Serial.printf("Writing file: %s\n", path);
      File file = fs.open(path, FILE_WRITE);
      if (!file) {
        Serial.println("Failed to open file for writing");
        return;
      }
      if (file.print(message)) {
        Serial.println("File written");
      } else {
        Serial.println("Write failed");
      }
      file.close();
    }

    // Append data to the SD card (DON'T MODIFY THIS FUNCTION)
    void appendFile(fs::FS &fs, const char * path, const char * message) {
      Serial.printf("Appending to file: %s\n", path);
      File file = fs.open(path, FILE_APPEND);
      if (!file) {
        Serial.println("Failed to open file for appending");
        return;
      }
      if (file.print(message)) {
        Serial.println("Message appended");
      } else {
        Serial.println("Append failed");
      }
      file.close();
    }

  public:
    String dataMessage;
};
