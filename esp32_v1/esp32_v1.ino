#include <PubSubClient.h>
#include "WiFi.h"
#include "settings.hpp"
#include "outputs.hpp"
#include "inputs.hpp"
#include "wifi.hpp"
#include "main.h"
#include "mqttCommunication.h"
#include "EthCommunication.h"

// extern ethStruct gEthernetDriver;

_Wifi Wifi;
_Output Output;
_Input Input;

//------------------------------------------------------------------------------
// Preprocessors
//------------------------------------------------------------------------------
#define DELAY_500MS pdMS_TO_TICKS(500)
// #define DELAY_10MS pdMS_TO_TICKS(10)

// WiFiClient espClient;
// PubSubClient client(espClient);

//------------------------------------------------------------------------------
// Local Variables
//------------------------------------------------------------------------------
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
int rpm = 0;
// Timer Variables
bool isTimerTwoExpired = true;
bool isTimerStartRequested = false;
// Variables
bool isMQTTConnectionEstablished = false;
bool isPublishInputMessageEnable = false;
bool isWiFiConnected = false;


//------------------------------------------------------------------------------
// Structs
//------------------------------------------------------------------------------
struct callbackStruct callback_data;
t_struct Time_t;
gUartMessage oledMessage;
Struct_Output outputDataCallback, outputStructDataInitialization;
Struct_MQTT mqttSendDataBuffer, mqttSendDataPeriodicBuffer, mqttSendInputMessage;
Communication_struct ActiveInterface;
//------------------------------------------------------------------------------
// Timer Handlers
//------------------------------------------------------------------------------
TimerHandle_t timerTwoOneShotHandler = NULL;

//------------------------------------------------------------------------------
// Task Prototypes
//------------------------------------------------------------------------------
void InitializationTask(void *pvParam);
void OutputTask(void *pvParam);
void MQTT_Task(void *pvParam);
void OLED_DisplayTask(void *pvParam);
void CallbackTask(void *pvParam);
void InputTask(void *pvParam);

//------------------------------------------------------------------------------
// Task Handlers
//------------------------------------------------------------------------------
TaskHandle_t Initialization_Task_Handler, Input_Task_Handler, Output_Task_Handler, Display_Task_Handler;
TaskHandle_t MQTT_Task_Handler, Callback_Task_Handler;

//------------------------------------------------------------------------------
// Queues Handlers
//------------------------------------------------------------------------------
// MQTT Queue
QueueHandle_t mqttQueue;
// Handles for Queues
QueueHandle_t serialWriteQueue;
// oled Queue
QueueHandle_t oledQueue;
// output Queue
QueueHandle_t outputQueue;

//------------------------------------------------------------------------------
// This is the Setup Task for Arduino
//------------------------------------------------------------------------------
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);  // this should be after Sensors.begin()
  // Initializing the peripherals
  MemoryInit();
  // Starting OLED Display
  BeginOledDisplay();
  // Starting outputs
  Output.begin();
  // Starting Inputs
  Input.begin();

  ActiveInterface.interface = COMMUNICATION_NONE;
  // checking if Ethernet is available
  OLED_Displayln("Checking Communication Interface");
  if (InitEthernet() == 0) {
    Serial.println("Checking the Ethernet Interface");
    OLED_Displayln("No Issue with Ethernet Modem");
    OLED_Displayln("Checking Internet Connectivity");

    if (false == TestEthernetConnection()) {

    } else {
      ActiveInterface.interface = COMMUNICATION_ETHERNET;
      Serial.println("Ethernet Connected");
      OLED_Displayln("Ethernet Connected");
    }
  }

  if ((ActiveInterface.interface == NONE)) {
#ifdef DYNAMIC_WIFI
    isWiFiConnected = Wifi.connect();
    ActiveInterface.interface = COMMUNICATION_WIFI_DYNAMIC;
#else
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_password);
    Serial.println("Connecting to WiFi..");
    while (WiFi.status() != WL_CONNECTED) {
      //    Serial.print(".");
      //    delay(1000);
    }
    SetIsWiFiConnected(true);
    ActiveInterface.interface = COMMUNICATION_WIFI_STATIC;
#endif
    Serial.println("Connected ");
  }


  InitMQTTClient();

  // Creation of Queues
  serialWriteQueue = xQueueCreate(100, sizeof(gUartMessage));
  oledQueue = xQueueCreate(10, sizeof(gUartMessage));
  mqttQueue = xQueueCreate(200, sizeof(Struct_MQTT));
  outputQueue = xQueueCreate(100, sizeof(Struct_Output));

  // Creation of Timers
  timerTwoOneShotHandler = xTimerCreate("Timer two", pdMS_TO_TICKS(TIMER), 0, (void *)0, timerTwoCallback);

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

void getUptime() {
  unsigned long timeNow;
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
  Struct_Output outputStructData, outputStructDataInitialization;
  unsigned long periodicTimer = 0;
  unsigned long periodicTimerTwo = 0;
  unsigned long periodicTimerCheck;
  unsigned long periodicTimerOutput = 0;
  bool oledMessageAccessPointStartedSent = false;
  bool oledMessageWifiConnected = false;
  uint8_t index;
  char rpmChr[30];
  while (1) {
    periodicTimerCheck = millis();
    if ((GetIsWiFiConnected() == true ) || (ActiveInterface.interface == COMMUNICATION_ETHERNET)) {
      //  Serial.println(isTimerTwoExpired);
      if (periodicTimerCheck - periodicTimerTwo > PERIODIC_RECONNECT_TIMEOUT) {
        periodicTimerTwo = periodicTimerCheck;
       // Serial.println("Checking MQTT");
        if (!GetMQTTClientConnectionStatus()) {
          MQTTreconnect();
        }
      }

      // client.loop();
      MQTTRefreshConnection();

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

    // updating the outputs
    if (periodicTimerCheck - periodicTimerOutput > TEN_MILL) {
      periodicTimerOutput = periodicTimerCheck;
      SendMessageToOutputTaskInit(UPDATE_OUT);
    }
    if ((Wifi.GetWiFiBlockingState() == false) && GetIsWiFiConnected() == false && (ActiveInterface.interface != COMMUNICATION_ETHERNET)) {
      if (oledMessageAccessPointStartedSent == false) {
        SendOLEDMessageFromInit("Access Point Started");
        SendOLEDMessageFromInit("SSID: ESP32");
        SendOLEDMessageFromInit("IP: 192.168.4.1");
        oledMessageAccessPointStartedSent = true;
      }
      Wifi.process();
    }
    if ((ActiveInterface.interface != COMMUNICATION_ETHERNET)) {
      if (WiFi.status() == WL_CONNECTED) {
        if (oledMessageWifiConnected == false) {
          SendOLEDMessageFromInit("WiFi Connected");
          SetIsWiFiConnected(true);
          oledMessageWifiConnected = true;
        }
      } else {
        SetIsWiFiConnected(false);
        oledMessageWifiConnected = false;
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
      // Serial.println("Output Task Request");
      switch (outputStructData.ID) {
        case PROCESS_OUT:
          Output.process(outputStructData.topic, outputStructData.payload);
          break;

        case START_OUT:
          Output.start(outputStructData.payload);
          break;

        case STOP_OUT:
          Output.stop(outputStructData.payload);
          break;

        case UPDATE_OUT:
          Output.update();
          break;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}


void InputTask(void *pvParam) {
  Struct_Output outputQueueData;
  char inputMessage[100];
  uint8_t buttonCounter = 1;
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

    //------------------------------------------------------------------------------
    // Process Push Buttons
    //------------------------------------------------------------------------------

    timeNow = millis();
    buttonValue = analogRead(BUTTONS);
    //    Serial.print("AnalogRead: ");
    //    Serial.println(buttonValue);              // Uncomment to print button value
    if (buttonValue > lastButtonValue + 100 || buttonValue < lastButtonValue - 100) {  // if the button state has changed
      if (buttonValue == 0 && timeNow - lastDebounceTime > 5000) {                     // If button is released after a long press
        Wifi.resetAP();
      }
      lastDebounceTime = timeNow;
    }

    if (buttonValue == 0) {
      pushed = false;  // Reset the button press
    }
    if ((timeNow - lastDebounceTime) > settings["debounce"]) {
      if (pushed == false && buttonValue > 0) {
        if (buttonValue > 3000 && buttonValue < 3600) {  // AE-01 Settings
          Serial.println("Button SELECT pressed");
          PublishMQTTInputMessage("button", "SELECT");
          pushed = true;
          outputButton = SELECT;

        } else if (buttonValue > 2000 && buttonValue < 2500) {  // AE-01 Settings
          Serial.println("Button DOWN pressed");
          PublishMQTTInputMessage("button", "DOWN");
          pushed = true;
          outputButton = DOWN;
        } else if (buttonValue > 1000 && buttonValue < 1700) {  // AE-01 Settings
          Serial.println("Button UP pressed");
          PublishMQTTInputMessage("button", "UP");
          pushed = true;
          outputButton = UP;
        }
      } else {
        outputButton = NONE;
        //  Serial.println("Button NONE");
      }
    }
    lastButtonValue = buttonValue;  // Update

    if (outputButton == SELECT) {
      SendMessageToOutputTask("output", "transZero", START_OUT);
    }

    //------------------------------------------------------------------------------
    // Process GPIO Inputs
    //------------------------------------------------------------------------------

    Input.update(inputMessage);
    if (strcmp(inputMessage, "none") != 0) {
      // Publish the input message first
      PublishMQTTInputMessage("input", inputMessage);

      if (strcmp(inputMessage, "inFive/1") == 0) {
        if (buttonCounter > 3) {
          buttonCounter = 1;
        }
        if (buttonCounter == 1) {
          SendMessageToOutputTask("output", "relayTwo", START_OUT);
        }
        if (buttonCounter == 2) {
          SendMessageToOutputTask("output", "relayThree", START_OUT);
        }
        if (buttonCounter == 3) {
          SendMessageToOutputTask("output", "relayFour", START_OUT);
        }
        buttonCounter++;
      }

      if (strcmp(inputMessage, "inSix/1") == 0) {
        SendMessageToOutputTask("output", "transZero", START_OUT);
      }

      if (strcmp(inputMessage, "inOne/1") == 0) {
        SendMessageToOutputTask("output", "relayOne", START_OUT);
      }

      if (strcmp(inputMessage, "inSeven/1") == 0) {
        SendMessageToOutputTask("output", "relayZero", START_OUT);
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
    }

    //------------------------------------------------------------------------------
    // Process Timers
    //------------------------------------------------------------------------------


    // if ((isTimerTwoExpired == false) && (isTimerStartRequested == false)) {
    //   // Start the timer
    //   xTimerStart(timerTwoOneShotHandler, 1);
    //   isTimerStartRequested = true;
    // }

    // if (isTimerTwoExpired == true) {
    //   SendMessageToOutputTask("output", "transZero", START_OUT);
    //   isTimerTwoExpired = false;
    //   isTimerStartRequested = false;
    // }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void loop() {
}


void SendMessageToOutputTaskInit(enum enumOutTask y) {
  outputStructDataInitialization.ID = y;
  xQueueSend(outputQueue, (void *)&outputStructDataInitialization, portMAX_DELAY);
}


void SendMessageToOutputTask(char topic[], char payload[], enum enumOutTask x) {
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

  if ((GetIsWiFiConnected() == true) || (ActiveInterface.interface == COMMUNICATION_ETHERNET)) {
    xQueueSend(mqttQueue, (void *)&mqttSendInputMessage, portMAX_DELAY);
  }
}

void publishMQTTPeriodicMessage(char topic[], char payload[]) {
  memset(mqttSendDataPeriodicBuffer.topic, 0, 100);
  memset(mqttSendDataPeriodicBuffer.payload, 0, 100);
  sprintf(mqttSendDataPeriodicBuffer.topic, topic);
  sprintf(mqttSendDataPeriodicBuffer.payload, payload);
  mqttSendDataPeriodicBuffer.ID = IDPUBLISHMQTT;
  if ((GetIsWiFiConnected() == true) || (ActiveInterface.interface == COMMUNICATION_ETHERNET)) {
    xQueueSend(mqttQueue, (void *)&mqttSendDataPeriodicBuffer, portMAX_DELAY);
  }
}

void publishMQTTMessage(char topic[], char payload[]) {
  memset(mqttSendDataBuffer.topic, 0, 100);
  memset(mqttSendDataBuffer.payload, 0, 100);
  sprintf(mqttSendDataBuffer.topic, topic);
  sprintf(mqttSendDataBuffer.payload, payload);
  mqttSendDataBuffer.ID = IDPUBLISHMQTT;
  
  if ((GetIsWiFiConnected() == true) || (ActiveInterface.interface == COMMUNICATION_ETHERNET)) {
    xQueueSend(mqttQueue, (void *)&mqttSendDataBuffer, portMAX_DELAY);
  }
}

void SendOLEDMessageFromInit(char body[]) {
  sprintf(oledMessage.body, body);
  xQueueSend(oledQueue, (void *)&oledMessage, portMAX_DELAY);
}

void SetPublishInputMessageEnable(bool value) {
  isPublishInputMessageEnable = value;
}

bool GetPublishInputMessageEnable(void) {
  return isPublishInputMessageEnable;
}

bool GetIsWiFiConnected() {
  return isWiFiConnected;
}

void SetIsWiFiConnected(bool value) {
  isWiFiConnected = value;
}


void SetMQTTConnectionStatus(bool value) {
  isMQTTConnectionEstablished = value;
}

bool GetMQTTConnectionStatus(void) {
  return isMQTTConnectionEstablished;
}

CommunicationInterface GetActiveInterface(void) {
  return ActiveInterface.interface;
}
// Timer Callbacks
void timerTwoCallback(TimerHandle_t timerTwoOneShotHandler) {
  // This is the Callback Function
  // The callback function is used to count the x number of seconds and signals the task if the timer expires
  //  Serial.println("Timer Expired");
  isTimerTwoExpired = true;
}