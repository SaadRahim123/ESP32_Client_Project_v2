#pragma once
#include <stdint.h>
#include <stdbool.h>

#define DYNAMIC_WIFI

// For Static Configuration Please enter the credentials here
const char *ssid = "Star";
const char *password = "12345678";


/*
    Defines
*/
#define DEBUG_ENABLE

#define TEN_MILL        10
#define FIVE_HUN_MILL   500
#define ONE_SEC         1000
#define THREE_SEC       ONE_SEC * 3
#define FIVE_SEC        5 * ONE_SEC
#define THIRTY_SEC      30 * ONE_SEC
#define FIFTY_SEC       50 * ONE_SEC
#define ONE_MIN         60 * ONE_SEC
#define TWO_MIN         60 * ONE_MIN
#define FIVE_MIN        5 * ONE_MIN

// MQTT ID's
#define IDPUBLISHMQTT       0
#define IDSUBSCRIBEMQTT     1
#define IDPUBLISH_INPUT     2
#define IDPUBLISH_SYSTEM    3

#define MAX_STRING_SIZE     30




#define WDT_TIMEOUT     5
#define SD_CS_PIN       5
#define SDA_PIN         16
#define SCL_PIN         17
//#define FC              4     // Needs to be high for GPIO32 to work on AE-04
#define BUTTONS         32    // Analog input AE-02 pin 34, AE-06 pin 36, AE-01 pin 32
//#define TEMP_DATA_PIN   7     // DS18B20 data pin

/*
  RTOs defines
*/
//
#define PERIODIC_MESSAGE_TIMEOUT        THREE_SEC
#define PERIODIC_RECONNECT_TIMEOUT      FIVE_SEC

/*
   Button enum
*/
enum Button {
  NONE,
  UP,
  DOWN,
  SELECT,
};

/*
    ENUM
*/
enum enumOutTask
{
  PROCESS_OUT = 0,
  START_OUT,
  STOP_OUT,
  UPDATE_OUT
};

/*
    Structures
*/

struct callbackStruct
{
  char payload[500];
  char topic[100];
  bool dataArrives;
};

typedef struct time_struct
{
  char timeStr[100];
  unsigned long lastUpdate, upSeconds;
  int hours, minutes, seconds, days , rollover;
} t_struct;


typedef struct MQTT_Data
{
  char topic[100];
  char payload[100];
  //    enum_MQTT query;
  uint8_t ID;
} Struct_MQTT;

typedef struct OUTPUT_Data
{
  char topic[100];
  char payload[100];
  enum enumOutTask ID;
} Struct_Output;

typedef struct Message
{
  char body[100];
  int count;
} gUartMessage;


// MQTT Queue
QueueHandle_t mqttQueue;
// Handles for Queues
QueueHandle_t serialWriteQueue;
// oled Queue
QueueHandle_t oledQueue;
// output Queue
QueueHandle_t outputQueue;









// Function prototypes
void reconnect(void);

// This function is used to set the MQTT Connection Status
// @true means that the connection is established
// @false means that connection refused
void SetMQTTConnectionStatus(bool);

// This function is used to get the MQTT Connection Status
bool GetMQTTConnectionStatus(void);


// This function is used to get the WiFi Connection Status
bool GetIsWiFiConnected();

// This function is used to set the WiFi Connection Status
void SetIsWiFiConnected(bool value);

// This function is used to send OLED Messages from other files
void SendOLEDMessageFromInit(char body[]);
