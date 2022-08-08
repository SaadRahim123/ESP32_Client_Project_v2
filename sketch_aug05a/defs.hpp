#pragma once
#include "main.h"

#define DYNAMIC_WIFI
// For Static Configuration Please enter the credentials here
const char *ssid = "PTCL-I80";
const char *password = "sherlocked";

#define WDT_TIMEOUT     5
#define SD_CS_PIN       5
#define SDA_PIN         16
#define SCL_PIN         17
//#define FC              4     // Needs to be high for GPIO32 to work on AE-04
#define BUTTONS         32    // Analog input AE-02 pin 34, AE-06 pin 36, AE-01 pin 32
//#define TEMP_DATA_PIN   7     // DS18B20 data pin 

/*
* RTOs defines
*/
// 
#define PERIODIC_MESSAGE_TIMEOUT        THREE_SEC
