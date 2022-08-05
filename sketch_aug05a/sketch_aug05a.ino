#include <PubSubClient.h>
#include "WiFi.h"
#include "settings.hpp"
#include "oled.hpp"
#include "outputs.hpp"
#include "inputs.hpp"
#include "timers.hpp"
#include "buttons.hpp"
#include "wifi.hpp"
#include "MQTT_Task.h"
#include "watchdog.hpp"
#include "defs.hpp"
#include "sdcard.hpp"
#include "memory.hpp"
#include "main.h"

_Wifi Wifi;
_Output Output;
_Input Input;
_Nvs Nvs;
Oled Oled;
Buttons buttons;

#define DELAY_500MS pdMS_TO_TICKS(500)
#define DELAY_10MS pdMS_TO_TICKS(10)
const char *ssid = "PTCL-I80";
const char *password = "sherlocked";
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

int rpm = 0;

// Variables
bool isMQTTConnectionEstablished = false;
bool isPublishInputMessageEnable = false;


// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const int mqtt_port = 1883;
const char *mqtt_ID = "esp32a01";
// MQTT Credentials
const char *mqtt_username = "remote2";
const char *mqtt_password = "password2";
const char *mqtt_client = "ESP32A01";

//// MQTT Broker
//const char *mqtt_broker = "broker.hivemq.com";//"broker.emqx.io";
//const int mqtt_port = 1883;
//const char *mqtt_ID = "lens_kcAqgK1sIkf8sEFd538kqZTr2hZ";//"esp32a01";
//// MQTT Credentials
//const char *mqtt_username = "remote2";
//const char *mqtt_password = "password2";
//const char *mqtt_client = "lens_kcAqgK1sIkf8sEFd538kqZTr2hZ";

t_struct Time_t;

QueueHandle_t mqttQueue;
// Handles for Queues
QueueHandle_t serialWriteQueue;
// oled Queue
QueueHandle_t oledQueue;

// output Queue
QueueHandle_t outputQueue;
// MQTT Queue

// Tasks Priority

// Task Prototypes

void InitializationTask(void *pvParam);
void InputTask(void *pvParam);
void OutputTask(void *pvParam);
void MQTT_Task(void *pvParam);
void OLED_DisplayTask(void *pvParam);

// Task Handlers
TaskHandle_t Initialization_Task_Handler, Input_Task_Handler, Output_Task_Handler, Display_Task_Handler;
TaskHandle_t MQTT_Task_Handler;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);  // this should be after Sensors.begin()

  // Initializing the peripherals
  Nvs.begin();
  // Starting OLED Display
  Oled.begin();
  // Starting outputs
  Output.begin();
  // Starting Inputs
  Input.begin();
  // Button Initialization
  buttons.begin();


  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi..");
  ////    writeQueue("Connecting to WiFi ..");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    //  //    writeQueue(".");
    delay(1000);
  }
  Serial.println("Connected ");
  client.setServer(mqtt_broker, 1883);
  client.setCallback(callback);


  /*
 * Creation of Queues
 */
  serialWriteQueue = xQueueCreate(100, sizeof(gUartMessage));
  oledQueue = xQueueCreate(10, sizeof(gUartMessage));
  mqttQueue = xQueueCreate(200, sizeof(Struct_MQTT));
  outputQueue = xQueueCreate(100, sizeof(Struct_Output));

  // Initialization Task
  xTaskCreatePinnedToCore(InitializationTask, "Initialization Task", 5120, NULL, 5, &Initialization_Task_Handler, 0);
  // MQTT Task
  xTaskCreatePinnedToCore(MQTT_Task, "MQTT Task", 4096, NULL, 3, &MQTT_Task_Handler, 0);
  // Display Task
  xTaskCreatePinnedToCore(OLED_DisplayTask, "Display Oled Task", 2046, NULL, 3, &Display_Task_Handler, 0);
  // Input Task
  xTaskCreatePinnedToCore(InputTask, "Input Task", 5120, NULL, 3, &Input_Task_Handler, 0);
  // Output Task
  xTaskCreatePinnedToCore(OutputTask, "Output Task", 2048, NULL, 3, &Output_Task_Handler, 0);
}

unsigned long timeNow;
void getUptime() {
  timeNow = millis();
  Time_t.upSeconds = timeNow / 1000;
  Time_t.seconds = Time_t.upSeconds % 60;
  Time_t.minutes = (Time_t.upSeconds / 60) % 60;
  Time_t.hours = (Time_t.upSeconds / (60 * 60)) % 24;
  Time_t.days = (Time_t.rollover * 50) + (Time_t.upSeconds / (60 * 60 * 24)); // 50 day rollover

  sprintf(Time_t.timeStr, "%02d %02d:%02d:%02d", Time_t.days, Time_t.hours, Time_t.minutes, Time_t.seconds);
 // Serial.println(Time_t.timeStr);  // Uncomment to serial print
}

Struct_MQTT mqttSendDataBuffer , mqttSendDataPeriodicBuffer;

void InitializationTask(void *pvParam) {
  //  Struct_MQTT mqttSendDataBuffer;
  unsigned long periodicTimer = 0;
  unsigned long periodicTimerCheck;
  gUartMessage oledMessage;
  uint8_t index;
  char rpmChr[30];
  while (1) {
    if (!client.connected()) {
      sprintf(oledMessage.body , "Attempting Re-connection ");
      xQueueSend(oledQueue, (void *) &oledMessage , portMAX_DELAY);
      reconnect();
      sprintf(oledMessage.body , "MQTT Connected ");
      xQueueSend(oledQueue, (void *) &oledMessage , portMAX_DELAY);
      sprintf(oledMessage.body , "Subscribing... ");
      xQueueSend(oledQueue, (void *) &oledMessage , portMAX_DELAY);
      
    }
    client.loop();

    periodicTimerCheck = millis();
    if (periodicTimerCheck - periodicTimer > THREE_SEC) {
      periodicTimer = periodicTimerCheck;
      getUptime();
      // Sending WiFi RSSI Value
      publishMQTTPeriodicMessage("wifi", Wifi.getRssiAsQuality());
      publishMQTTPeriodicMessage("uptime" , Time_t.timeStr);
      
      snprintf(rpmChr, sizeof(rpmChr), "%d", rpm);
      publishMQTTPeriodicMessage("tacho", rpmChr);            
    }


    vTaskDelay(pdMS_TO_TICKS(10));
  }
}


void MQTT_Task(void *pvParam) {
  unsigned long mqttReconnectionTime;
  Struct_MQTT mqttData;
  //gUartMessage mqttData;
  char dataArray[100];
  char result[100];
  while (1) {
    while (GetMQTTConnectionStatus() == true) {
      if (xQueueReceive(mqttQueue, (void *)&mqttData, portMAX_DELAY) == pdTRUE) {
        //        result = (char *)pvPortMalloc(strlen(dataArray) + 1);
        switch ((mqttData.ID)) {
          case IDPUBLISHMQTT:
            Serial.print("ID Publish MQTT");
            memset(dataArray, 0, 100);
            snprintf(dataArray, 100, "%s/%s", mqtt_ID, mqttData.topic);
            client.publish(dataArray, mqttData.payload);
            Serial.print(dataArray);
            Serial.print("/");
            Serial.println(mqttData.payload);
            //            vPortFree(result);          // Freeing the memory
            break;

          case IDSUBSCRIBEMQTT:

            Serial.println("ID Subscribe MQTT");
            memset(dataArray, 0, 100);

            snprintf(dataArray, sizeof dataArray, "%s/%s", mqtt_ID, mqttData.topic);
            client.subscribe(dataArray);
            Serial.println(dataArray);
            break;

          case IDPUBLISH_INPUT:
            break;

          case IDPUBLISH_SYSTEM:
            break;
        }
      }
    }
    // Yielding the task in case of Connection not established
    taskYIELD();
  }
}


void OutputTask(void *pvParam) {

  Struct_Output outputStructData;
  int payloadDataAsInt;
  unsigned long timeNow;
  while (1) {
    if (xQueueReceive(outputQueue, (void *)&outputStructData, portMAX_DELAY) == pdTRUE) {

          Serial.println("Output Task Request");      
          switch (outputStructData.ID) {
        case PROCESS_OUT:

          Output.process(outputStructData.topic, outputStructData.payload);
          break;

        case START_OUT:
          Output.start(outputStructData.topic);
          break;

        case STOP_OUT:
          Output.stop(outputStructData.topic);
          break;

        case UPDATE_OUT:
          timeNow = millis();
          // check if output is on before checking if its a 0
          for (uint8_t i = 0; i < NUM_OUTPUTS; i++) {

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
          break;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}


void InputTask(void *pvParam) {
  Struct_Output outputQueueData;
  char inputMessage[100];
  uint8_t buttonCounter = 0;
  unsigned int revTime;
  unsigned long lastTachoTime = 0;
  unsigned long timeNow;
  while (1) {

    // if (buttons.pressed() == Buttons::SELECT) {
    //   publishMQTTMessage("button", "SELECT" );
    // }

    if (GetPublishInputMessageEnable()) {  // Only publish if switched on
      Input.update(inputMessage);
      if (inputMessage != "none") {
        publishMQTTMessage("input", inputMessage);
      }
    }

    // Getting the time
    timeNow = millis();
    Input.update(inputMessage);
    if (inputMessage != "none") {

      if (strcmp(inputMessage, "inFive/1") == 0) {
        Serial.println("Button Pushed");
        if (buttonCounter > 3) {
          buttonCounter = 1;
        }
        if (buttonCounter == 1) {
          Serial.println("relayTwo triggered");
          outputQueueData.ID = START_OUT;
          sprintf(outputQueueData.topic, "replayTwo");
            xQueueSend(outputQueue, (void *)&outputQueueData, portMAX_DELAY);
          //Output.start("relayTwo");
        }
        if (buttonCounter == 2) {
          outputQueueData.ID = START_OUT;
          sprintf(outputQueueData.topic, "relayThree");
            xQueueSend(outputQueue, (void *)&outputQueueData, portMAX_DELAY);
          //Output.start("relayThree");
          Serial.println("relayThree triggered");
        }
        if (buttonCounter == 3) {
          outputQueueData.ID = START_OUT;
          sprintf(outputQueueData.topic, "relayFour");
            xQueueSend(outputQueue, (void *)&outputQueueData, portMAX_DELAY);
          //Output.start("relayFour");
          Serial.println("relayFour triggered");
        }
        buttonCounter++;
      }

      if (strcmp(inputMessage, "inSix/1") == 0) {
        outputQueueData.ID = START_OUT;
        sprintf(outputQueueData.topic, "transOne");
          xQueueSend(outputQueue, (void *)&outputQueueData, portMAX_DELAY);
        Serial.println("transOne triggered");
        // Output.start("transOne");
      }

      if (strcmp(inputMessage, "inOne/1") == 0) {
        outputQueueData.ID = START_OUT;
        sprintf(outputQueueData.topic, "relayOne");
          xQueueSend(outputQueue, (void *)&outputQueueData, portMAX_DELAY);
        Serial.println("relayOne triggered");
        //Output.start("relayOne");
      }

      if (strcmp(inputMessage, "inSeven/1") == 0) {
        outputQueueData.ID = START_OUT;
        sprintf(outputQueueData.topic, "relayZero");
          xQueueSend(outputQueue, (void *)&outputQueueData, portMAX_DELAY);
        Serial.println("relayZero triggered");
        //      Timer.start("timerOne");
        // Output.start("relayZero");
      }

      // Calculate RPM
      if (strcmp(inputMessage, "inZero/1") == 0) {
        revTime = timeNow - lastTachoTime;  // Calculate millis per rev
        if (revTime >= 60000) {
          rpm = 0;  // Limit rpm to 0
        } else {
          rpm = 60000 / revTime;  // Convert to rpm
        }
        lastTachoTime = timeNow;  // Update timer
      }
      // Only publish input message if switched on
      if (GetPublishInputMessageEnable()) {
        publishMQTTMessage("input", inputMessage);
        Serial.println(inputMessage);
      }
    }



    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
void OLED_DisplayTask(void *pvParam) {
  gUartMessage receiveMsg;
  char tempString[100];
  while (1) {
    if (xQueueReceive(oledQueue, (void *)&receiveMsg, portMAX_DELAY) == pdTRUE) {
      sprintf(tempString, "Writing oled %s", receiveMsg.body);
      //      writeQueue(tempString);
      Oled.displayln(receiveMsg.body);
    }

    vTaskDelay(DELAY_10MS);
  }
}


void loop() {
}

Struct_Output outputDataCallback;

struct callbackStruct
{
    char payload[500];
    char topic[100];
       
};

struct callbackStruct callback_data;
char *topic;
void callback(char *topic, byte *payload, unsigned int length) {
  
  const char NULL_terminator[2] = "\0";
 

  Serial.print("Message arrived [");
  Serial.print(callback_data.topic);
  Serial.print("] ");
  
  for(uint8_t i =0 ; i< length; i++ )
  {
    callback_data.payload[i] = (char)payload[i];
  }

    // Break topic down
  char *payloadId = strtok(callback_data.topic, "/");
  char *payloadFunc = strtok(NULL_terminator, "/");
  // Break payload down
  char *payloadName = strtok(payloadAsChar, "/");
  char *payloadData = strtok(NULL_terminator, "/");
  callback_data.payload[length] = NULL_terminator;

  Serial.print("ID: ");
  Serial.print(payloadId);
  Serial.print(" Function: ");
  Serial.print(payloadFunc);
  Serial.print(" Name: ");
  Serial.print(payloadName);
  Serial.print(" Data: ");
  Serial.println(payloadData);
  
  //    //Conver *byte to char* 
//   payload[length] = '\0';   //First terminate payload with a NULL
//  payloadAsChar = (char*)payload;
//   // Break topic down
//   char *payloadId = strtok(topic, "/");
//   char *payloadFunc = strtok("\0", "/");
//   // Break payload down
//   char *payloadName = strtok(payloadAsChar, "/");
//   char *payloadData = strtok("\0", "/");

//   Serial.print("ID: ");
//   Serial.print(payloadId);
//   Serial.print(" Function: ");
//   Serial.print(payloadFunc);
//   Serial.print(" Name: ");
//   Serial.print(payloadName);
//   Serial.print(" Data: ");
//   Serial.println(payloadData);

//   // If topic is a set
//   if (strcmp(payloadFunc, "set") == 0) {
//     Nvs.set(payloadName, payloadData);
//     char* reply = Nvs.get(payloadName);
//     //publishMQTTMessage("reply", reply);
//   }
//   // If topic is a get
//   if (strcmp(payloadFunc, "get") == 0) {
//     char* reply = Nvs.get(payloadName);
//     publishMQTTMessage("reply", reply);
//   }

//   if (strcmp(payloadFunc, "output") == 0) {
//     // outputDataCallback.ID = PROCESS_OUT;
//     // sprintf(outputDataCallback.topic , payloadAsChar);
//     // sprintf(outputDataCallback.payload ,payloadData);
//     // xQueueSend(outputQueue, (void*) &outputDataCallback , portMAX_DELAY);
//     Serial.println("Callback Data output ");


//   }

//   if (strcmp(payloadFunc, "timer") == 0) {
//    // Timer.start(payloadAsChar);
//     return;
//   }

//   if (strcmp(payloadFunc, "system") == 0) {

//     if (strcmp(payloadName, "publish") == 0) {
//       SetPublishInputMessageEnable(!isPublishInputMessageEnable);
//       Serial.print("Publish input messages: ");
//       Serial.println(isPublishInputMessageEnable);
//     }

//     if (strcmp(payloadName, "restart") == 0) {
//       Serial.println("Resetting ESP32");
//       ESP.restart();
//       return;
//     }

//     if (strcmp(payloadName, "save") == 0) {
//       Nvs.save();
//     }

//     if (strcmp(payloadName, "erase") == 0) {
//       Nvs.erase();
//     }
//   }
}



//
//void publishSystem () {
//  if (Timer.state("timerOne") == false) {
//    publishMQTT("wifi", Wifi.getRssiAsQuality());
//    publishMQTT("uptime", Time.getUptime());
//    Timer.start("timerOne");                              // Retrigger for next time
//  }
//}


void subscribeMQTT(char *topic) {
  char dataArray[30];
  snprintf(dataArray, sizeof dataArray, "%s/%s", mqtt_ID, topic);


  client.subscribe(dataArray);
  Serial.println(dataArray);
}

void reSubscribe() {
  subscribeMQTT("timer");
  subscribeMQTT("output");
  subscribeMQTT("system");
  subscribeMQTT("set");
  subscribeMQTT("get");
  //  Serial.println("Connected, subscribing... ");
  //  Oled.displayln("MQTT Connected ");
  //  Oled.displayln("Subscribing... ");
}

int reconnectCounter = 0;
void reconnect() {
  // Loop until we're reconnected
  Serial.println("Attempting MQTT connection... ");
  //  Oled.displayln("Attempting connection ");
  String clientId = "MQTTClient-";
  clientId += String(random(0xffff), HEX);
  // Attempt to connect
  if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {  //(client.connect(clientId.c_str())) {
    reSubscribe();
    reconnectCounter = 0;  // reset counter
    Serial.println("Connected!! ");
    SetMQTTConnectionStatus(true);
  } else if (reconnectCounter > 5) {
    Serial.println("Resetting ESP32");
    delay(500);
    // ESP.restart();
  } else {
    reconnectCounter++;
    Serial.print("Attempt: ");
    Serial.print(reconnectCounter);
    Serial.print(" failed, Error: ");
    Serial.print(client.state());
    Serial.print(" Retrying in 5 seconds");
  }
}

void publishMQTTPeriodicMessage(char topic[], char payload[])
{

  sprintf(mqttSendDataPeriodicBuffer.topic, topic);
  sprintf(mqttSendDataPeriodicBuffer.payload, payload);
  mqttSendDataPeriodicBuffer.ID = IDPUBLISHMQTT;
  xQueueSend(mqttQueue, (void *)&mqttSendDataPeriodicBuffer, portMAX_DELAY);  
}

void publishMQTTMessage(char topic[], char payload[]) {
  sprintf(mqttSendDataBuffer.topic, topic);
  sprintf(mqttSendDataBuffer.payload, payload);
  mqttSendDataBuffer.ID = IDPUBLISHMQTT;
  xQueueSend(mqttQueue, (void *)&mqttSendDataBuffer, portMAX_DELAY);
}


void SetPublishInputMessageEnable(bool value) {
  isPublishInputMessageEnable = value;
}

bool GetPublishInputMessageEnable() {
  return isPublishInputMessageEnable;
}
void SetMQTTConnectionStatus(bool value) {
  isMQTTConnectionEstablished = value;
}

bool GetMQTTConnectionStatus(void) {
  return isMQTTConnectionEstablished;
}