#include "EthCommunication.h"

void setup() {
  // put your setup code here, to run once:
  uint16_t retEthernetInit;
  Serial.begin(115200);
  delay(500);
  Serial.println("This is an Ethernet Test Code with Esp32");
  retEthernetInit =  InitEthernet();
  if(retEthernetInit != 0 )
  {
      Serial.println("Unable to initialize Ethernet");
  }


  TestEthernetConnection();
}

void loop() {
  
}
