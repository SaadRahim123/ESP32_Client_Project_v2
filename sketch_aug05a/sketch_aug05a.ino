#include <PubSubClient.h>
#include "WiFi.h"
#include "settings.hpp"
#include "oled.hpp"
#include "outputs.hpp"
#include "inputs.hpp"
#include "buttons.hpp"
#include "wifi.hpp"
#include "defs.hpp"
#include "memory.hpp"
#include "main.h"

_Wifi Wifi;
_Output Output;
_Input Input;
_Memory Memory;
Oled Oled;
Buttons buttons;

#define DELAY_500MS pdMS_TO_TICKS(500)
#define DELAY_10MS pdMS_TO_TICKS(10)

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

int rpm = 0;

// Timer Variables
bool isTimerTwoExpired = false;
bool isTimerStartRequested = false;
// Variables
bool isMQTTConnectionEstablished = false;
bool isPublishInputMessageEnable = false;
bool isWiFiConnected = false;

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const int mqtt_port = 1883;
const char *mqtt_ID = "esp32a01";
// MQTT Credentials
const char *mqtt_username = "remote2";
const char *mqtt_password = "password2";
const char *mqtt_client = "ESP32A01";

/*
    Callback Struct
*/
struct callbackStruct callback_data;

t_struct Time_t;
// MQTT Queue
QueueHandle_t mqttQueue;
// Handles for Queues
QueueHandle_t serialWriteQueue;
// oled Queue
QueueHandle_t oledQueue;
// output Queue
QueueHandle_t outputQueue;

// Timers Handler
TimerHandle_t timerTwoOneShotHandler = NULL;

// Timer Callbacks
void timerTwoCallback(TimerHandle_t timerTwoOneShotHandler) {
  // This is the Callback Function
  // The callback function is used to count the x number of seconds and signals the task if the timer expires
  Serial.println("Timer Expired");
  isTimerTwoExpired = true;
}


// Task Prototypes
void InitializationTask(void *pvParam);
void OutputTask(void *pvParam);
void MQTT_Task(void *pvParam);
void OLED_DisplayTask(void *pvParam);
void CallbackTask(void *pvParam);
void InputTask(void *pvParam);
void InternetTask(void *pvParam);




Struct_Output outputDataCallback , outputStructDataInitialization;

// Task Handlers
TaskHandle_t Initialization_Task_Handler, Input_Task_Handler, Output_Task_Handler, Display_Task_Handler;
TaskHandle_t MQTT_Task_Handler, Callback_Task_Handler, Internet_Task_Handler;

// MQTT Structs
Struct_MQTT mqttSendDataBuffer, mqttSendDataPeriodicBuffer, mqttSendInputMessage;



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);  // this should be after Sensors.begin()

  // Initializing the peripherals
  Memory.begin();
  // Starting OLED Display
  Oled.begin();
  // Starting outputs
  Output.begin();
  // Starting Inputs
  Input.begin();
  // Button Initialization
  buttons.begin();

#ifdef DYNAMIC_WIFI
  isWiFiConnected = Wifi.connect();
  if (isWiFiConnected) {
    Serial.println("isWiFiConnected");
  } else {
    Serial.println("WiFi not conneccted");
  }

#else
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi..");
  ////    writeQueue("Connecting to WiFi ..");
  sprintf(oledMessage.body, "Connecting WiFi");
  xQueueSend(oledQueue, (void *)&oledMessage, portMAX_DELAY);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    //  //    writeQueue(".");
    delay(1000);
  }
  SetIsWiFiConnected(true);
#endif
  Serial.println("Connected ");
  client.setServer(mqtt_broker, 1883);
  client.setCallback(callback);




  /*
    Creation of Queues
  */
  serialWriteQueue = xQueueCreate(100, sizeof(gUartMessage));
  oledQueue = xQueueCreate(10, sizeof(gUartMessage));
  mqttQueue = xQueueCreate(200, sizeof(Struct_MQTT));
  outputQueue = xQueueCreate(100, sizeof(Struct_Output));

  /*
    Creation of Timers
  */
  timerTwoOneShotHandler = xTimerCreate("Timer two", pdMS_TO_TICKS(3000), 0, (void *)0, timerTwoCallback);


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
  // Call back task
  xTaskCreatePinnedToCore(CallbackTask, "callback Task ", 2048, NULL, 3, &Callback_Task_Handler, 0);
  // Internet Task

}

unsigned long timeNow;
void getUptime() {
  timeNow = millis();
  Time_t.upSeconds = timeNow / 1000;
  Time_t.seconds = Time_t.upSeconds % 60;
  Time_t.minutes = (Time_t.upSeconds / 60) % 60;
  Time_t.hours = (Time_t.upSeconds / (60 * 60)) % 24;
  Time_t.days = (Time_t.rollover * 50) + (Time_t.upSeconds / (60 * 60 * 24));  // 50 day rollover

  sprintf(Time_t.timeStr, "%02d %02d:%02d:%02d", Time_t.days, Time_t.hours, Time_t.minutes, Time_t.seconds);
  // Serial.println(Time_t.timeStr);  // Uncomment to serial print
}


void InitializationTask(void *pvParam) {
  //  Struct_MQTT mqttSendDataBuffer;
  Struct_Output outputStructData , outputStructDataInitialization;
  unsigned long periodicTimer = 0;
  unsigned long periodicTimerCheck;
  unsigned long periodicTimerOutput = 0;
  gUartMessage oledMessage;
  uint8_t index;
  char rpmChr[30];
  while (1) {
    if (GetIsWiFiConnected()) {
      if (!client.connected()) {
        sprintf(oledMessage.body, "Attempting Re-connection ");
        xQueueSend(oledQueue, (void *)&oledMessage, portMAX_DELAY);
        reconnect();
        sprintf(oledMessage.body, "MQTT Connected ");
        xQueueSend(oledQueue, (void *)&oledMessage, portMAX_DELAY);
        sprintf(oledMessage.body, "Subscribing... ");
        xQueueSend(oledQueue, (void *)&oledMessage, portMAX_DELAY);
      }


      client.loop();
      periodicTimerCheck = millis();

      // updating the outputs
      if (periodicTimerCheck - periodicTimerOutput > FIVE_HUN_MILL) {
        periodicTimerOutput = periodicTimerCheck;
        SendMessageToOutputTaskInit( UPDATE_OUT);   
      }


      if (periodicTimerCheck - periodicTimer > PERIODIC_MESSAGE_TIMEOUT) {
        periodicTimer = periodicTimerCheck;
        getUptime();
        // Sending WiFi RSSI Value
        publishMQTTPeriodicMessage("wifi", Wifi.getRssiAsQuality());
        publishMQTTPeriodicMessage("uptime", Time_t.timeStr);

        snprintf(rpmChr, sizeof(rpmChr), "%d", rpm);
        publishMQTTPeriodicMessage("tacho", rpmChr);
      }
    }
    if((Wifi.GetWiFiBlockingState() == false) && GetIsWiFiConnected() == false )  
    {
      Wifi.process();
    }
    if(WiFi.status() == WL_CONNECTED)
    {
        SetIsWiFiConnected(true);      
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
    while ((GetMQTTConnectionStatus() == true) && (GetIsWiFiConnected() == true)) {
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
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void CallbackTask(void *pvParam) {
  while (1) {

    if ((callback_data.dataArrives)) {

      Serial.println("Here in the CallbackTask");
      callback_data.dataArrives = false;


      char *payloadId = strtok(callback_data.topic, "/");
      char *payloadFunc = strtok(NULL, "/");
      // Break payload down
      char *payloadName = strtok(callback_data.payload, "/");
      char *payloadData = strtok(NULL, "/");

      Serial.print("ID: ");
      Serial.print(payloadId);
      Serial.print(" Function: ");
      Serial.print(payloadFunc);
      Serial.print(" Name: ");
      Serial.print(payloadName);
      Serial.print(" Data: ");
      Serial.println(payloadData);

      // If topic is a set
      if (strcmp(payloadFunc, "set") == 0) {
        Memory.set(payloadName, payloadData);
        char *reply = Memory.get(payloadName);
        //publishMQTTMessage("reply", reply);
      }
      // If topic is a get
      if (strcmp(payloadFunc, "get") == 0) {
        char *reply = Memory.get(payloadName);
        publishMQTTMessage("reply", reply);
      }

      if (strcmp(payloadFunc, "output") == 0) {
        SendMessageToOutputTask(callback_data.payload, payloadData, PROCESS_OUT);
        Serial.println("Callback Data output ");
      }

      if (strcmp(payloadFunc, "timer") == 0) {
        // Timer.start(payloadAsChar);
        return;
      }

      if (strcmp(payloadFunc, "system") == 0) {

        if (strcmp(payloadName, "publish") == 0) {
          SetPublishInputMessageEnable(!isPublishInputMessageEnable);
          Serial.print("Publish input messages: ");
          Serial.println(isPublishInputMessageEnable);
        }

        if (strcmp(payloadName, "restart") == 0) {
          Serial.println("Resetting ESP32");
          ESP.restart();
          return;
        }

        if (strcmp(payloadName, "save") == 0) {
          Memory.save();
        }

        if (strcmp(payloadName, "erase") == 0) {
          Memory.erase();
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void OutputTask(void *pvParam) {

  Struct_Output outputStructData;
  int payloadDataAsInt;
  unsigned long timeNow;
  while (1) {
    if (xQueueReceive(outputQueue, (void *)&outputStructData, portMAX_DELAY) == pdTRUE) {

      //Serial.println("Output Task Request");
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


void InputTask(void *pvParam) {
  Struct_Output outputQueueData;
  char inputMessage[100];
  uint8_t buttonCounter = 0;
  unsigned int revTime;
  unsigned long lastTachoTime = 0;
  unsigned long timeNow;
  bool inputBool = false;
  BaseType_t timerTwoState;
  bool pushed = false;
  unsigned long lastDebounceTime;
  int buttonValue, lastButtonValue;
  enum Button outputButton;
  while (1) {

    timeNow = millis();
    buttonValue = analogRead(BUTTONS);

    // Serial.print("AnalogRead: ");
    // Serial.println(buttonValue);              // Uncomment to print button value

    if (buttonValue > lastButtonValue + 100 || buttonValue < lastButtonValue - 100) {  // if the button state has changed
      lastDebounceTime = timeNow;                                                      // Start the timer
    }

    if (buttonValue == 0) {
      pushed = false;  // Reset the button press
    }

    if ((timeNow - lastDebounceTime) > settings["debounce"]) {
      if (pushed == false && buttonValue > 0) {
        // Then handle button logic
        if (buttonValue > 3000 && buttonValue < 3300) {  // AE-01 Settings
          Serial.println("Button SELECT pressed");
          //  Oled.displayln("Button SELECT pressed");
          pushed = true;
          outputButton = SELECT;
        } else if (buttonValue > 2000 && buttonValue < 2300) {  // AE-01 Settings
          Serial.println("Button DOWN pressed");
          // Oled.displayln("Button DOWN pressed");
          pushed = true;
          outputButton = DOWN;
        } else if (buttonValue > 1200 && buttonValue < 1600) {  // AE-01 Settings
          Serial.println("Button UP pressed");
          // Oled.displayln("Button UP pressed");
          pushed = true;
          outputButton = UP;
        }
        // Serial.println("Button NONE");
        outputButton = NONE;
      }
    }
    lastButtonValue = buttonValue;  // Update


    if (outputButton == SELECT) {
      PublishMQTTInputMessage("button", "SELECT");
    }


    if (GetPublishInputMessageEnable()) {  // Only publish if switched on
      Input.update(inputMessage);
      if (inputMessage != "none") {
        PublishMQTTInputMessage("input", inputMessage);
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



    if ((isTimerTwoExpired == false) && (isTimerStartRequested == false)) {
      // Start the timer
      xTimerStart(timerTwoOneShotHandler, 1);
      PublishMQTTInputMessage("input", "inZero/0");
      isTimerStartRequested = true;
    }

    if (isTimerTwoExpired == true) {
      PublishMQTTInputMessage("input", "inZero/1");
      isTimerTwoExpired = false;
      isTimerStartRequested = false;
    }


    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
void loop() {
}




char *payloadAsChar = "";
void callback(char *topic, byte *payload, unsigned int length) {

  // Clearing the global string buffers
  memset(callback_data.topic, 0, 100);
  memset(callback_data.payload, 0, 500);
  //Conver *byte to char*
  payload[length] = '\0';  //First terminate payload with a NULL
  payloadAsChar = (char *)payload;
  // Break topic down
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    callback_data.payload[i] = (char)payload[i];
  }
  Serial.println();

  memcpy(callback_data.payload, payload, length);
  memcpy(callback_data.topic, topic, strlen(topic));

  Serial.print("  Payload");
  Serial.println(callback_data.payload);
  Serial.print("  topic");
  Serial.println(callback_data.topic);
  callback_data.dataArrives = true;
}

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
  clientId += 90;
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

void SendMessageToOutputTaskInit(enum enumOutTask y)
{
  
  outputStructDataInitialization.ID = y;
  xQueueSend(outputQueue, (void *)&outputStructDataInitialization, portMAX_DELAY);
}


void SendMessageToOutputTask(char topic[] , char payload[], enum enumOutTask x)
{
    memset(outputDataCallback.topic, 0, 100);
    memset(outputDataCallback.payload, 0, 100);
    sprintf(outputDataCallback.topic, topic);
    sprintf(outputDataCallback.payload, payload);
    outputDataCallback.ID = x;
   xQueueSend(outputQueue, (void *)&outputDataCallback, portMAX_DELAY);
}

void PublishMQTTInputMessage(char topic[], char payload[]) {
  memset(mqttSendInputMessage.topic, 0, 100);
  memset(mqttSendInputMessage.payload, 0, 100);
  sprintf(mqttSendInputMessage.topic, topic);
  sprintf(mqttSendInputMessage.payload, payload);
  mqttSendInputMessage.ID = IDPUBLISHMQTT;
  xQueueSend(mqttQueue, (void *)&mqttSendInputMessage, portMAX_DELAY);
}



void publishMQTTPeriodicMessage(char topic[], char payload[]) {
  memset(mqttSendDataPeriodicBuffer.topic, 0, 100);
  memset(mqttSendDataPeriodicBuffer.payload, 0, 100);
  sprintf(mqttSendDataPeriodicBuffer.topic, topic);
  sprintf(mqttSendDataPeriodicBuffer.payload, payload);
  mqttSendDataPeriodicBuffer.ID = IDPUBLISHMQTT;
  xQueueSend(mqttQueue, (void *)&mqttSendDataPeriodicBuffer, portMAX_DELAY);
}

void publishMQTTMessage(char topic[], char payload[]) {
  memset(mqttSendDataBuffer.topic, 0, 100);
  memset(mqttSendDataBuffer.payload, 0, 100);
  sprintf(mqttSendDataBuffer.topic, topic);
  sprintf(mqttSendDataBuffer.payload, payload);
  mqttSendDataBuffer.ID = IDPUBLISHMQTT;
  xQueueSend(mqttQueue, (void *)&mqttSendDataBuffer, portMAX_DELAY);
}


void SetPublishInputMessageEnable(bool value) {
  isPublishInputMessageEnable = value;
}

bool GetIsWiFiConnected() {
  return isWiFiConnected;
}

void SetIsWiFiConnected(bool value) {
  isWiFiConnected = value;
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