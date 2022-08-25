#include <Arduino.h>
#include "freertos/queue.h"
#include "main.h"
#include "oled.hpp"

#define DELAY_10MS pdMS_TO_TICKS(10)
extern QueueHandle_t oledQueue;
Oled Oled;


void OLED_DisplayTask(void *pvParam) {
  gUartMessage receiveMsg;
  char tempString[100];
  while (1) {
    if (xQueueReceive(oledQueue, (void *)&receiveMsg, portMAX_DELAY) == pdTRUE) {
      Serial.print("Oled print: ");
      Serial.println(receiveMsg.body);
      // sprintf(tempString, "Writing oled %s", receiveMsg.body);
      //      writeQueue(tempString);
      Oled.displayln(receiveMsg.body);
    }
    vTaskDelay(DELAY_10MS);
  }
}


void BeginOledDisplay(void)
{
  Oled.begin();
}
