#include "EthCommunication.h"
#include <Arduino.h>

#include <SPI.h>

enum enumLink 
{
    UNKNOWN = 0,
    LINK_ETHERNET_UP ,
    LINK_ETHERNET_DOWN
};


typedef struct ethernetStruct
{
  bool linkStatus;
  char hardwareIdentifier[50];
  enum enumLink link;  
  bool isHardwareFound;
  bool isEthComError;
}ethStruct;


uint8_t eth_MAC[] = { 0x02, 0xF0, 0x0D, 0xBE, 0xEF, 0x01};
char testServer[] = "www.google.com";

ethStruct gEthernetDriver;
EthernetClient clientETH;
//------------------------------------------------------------------------------
//  void WizReset(void)
//
//  This is used to reset the W5500
//------------------------------------------------------------------------------
void W5500Reset(void) {
  Serial.print("Resetting Wiz W5500 Ethernet Board...  ");
  pinMode(RESET_P, OUTPUT);
  digitalWrite(RESET_P, HIGH);
  delay(250);
  digitalWrite(RESET_P, LOW);
  delay(50);
  digitalWrite(RESET_P, HIGH);
  delay(350);
  Serial.println("Done.");
}

//------------------------------------------------------------------------------
//  void LinkCheck(void)
//
//  This is used to check the Link of W5500
//------------------------------------------------------------------------------
void LinkCheck(void) {
  uint8_t retLink = Ethernet.linkStatus();
  switch (retLink) {
    case 0:
      Serial.println("Uknown status.");
      gEthernetDriver.linkStatus = false;
      gEthernetDriver.link = UNKNOWN;
      break;
    case 1:
      Serial.println("Link flagged as UP.");
      gEthernetDriver.linkStatus = true;
      gEthernetDriver.link = LINK_ETHERNET_UP;
      break;
    case 2:
      Serial.println("Link flagged as DOWN. Check cable connection.");
      gEthernetDriver.linkStatus = false;
      gEthernetDriver.link = LINK_ETHERNET_DOWN;
      break;
    default:
      Serial.println("UNKNOWN - Update espnow_gw.ino to match Ethernet.h");
  }
}

//------------------------------------------------------------------------------
//  void HardwareIdentification(void)
//
//  This is identify Hardware attached to ESP32
//------------------------------------------------------------------------------
void HardwareIdentification(void) {
  uint8_t retHardwareIden = Ethernet.hardwareStatus();
  switch (retHardwareIden) {
    case 0:
      Serial.println("No hardware detected.");
      gEthernetDriver.isHardwareFound = false;
      break;
    case 1:
      Serial.println("WizNet W5100 detected.");
      gEthernetDriver.isHardwareFound = true;
      sprintf(gEthernetDriver.hardwareIdentifier, "WizNet W5100 detected");
      break;
    case 2:
      Serial.println("WizNet W5200 detected.");
      gEthernetDriver.isHardwareFound = true;
      sprintf(gEthernetDriver.hardwareIdentifier, "WizNet W5200 detected");
      break;
    case 3:
      Serial.println("WizNet W5500 detected.");
      gEthernetDriver.isHardwareFound = true;
      sprintf(gEthernetDriver.hardwareIdentifier, "WizNet W5500 detected");
      break;
    default:
      Serial.println("UNKNOWN - Update espnow_gw.ino to match Ethernet.h");
  }
}


//------------------------------------------------------------------------------
//  uint16_t InitEthernet(void)
//
//  This is used to initialize Ethernet interface
//------------------------------------------------------------------------------
uint16_t InitEthernet(void) {
  // Reseting the Ethernet PHY
  W5500Reset();
  // initializing W5500
  Ethernet.init(CS_PIN);
  // Starting Ethernet Communication
  Serial.println("Starting ETHERNET connection...");
  Ethernet.begin(eth_MAC, 60000, 4000);
  Serial.print("Ethernet IP is: ");
  Serial.println(Ethernet.localIP());


  // Checking Hardware Status
  HardwareIdentification();

  if (gEthernetDriver.isHardwareFound = false) {
    return ERRORCODE_NO_HARDWARE_FOUND;
  }
  Serial.println("Hardware Found ");

  // Checking Link
  for (uint8_t retries = 0; retries < 8; retries++) {
    LinkCheck();
    if (gEthernetDriver.linkStatus == true) {
        break;
    }
  }
  
  if (gEthernetDriver.linkStatus == false)
  {
    return ERRORCODE_NO_LINK_DETECTED;
  } 

  return 0;
  
}

//------------------------------------------------------------------------------
//  void TestEthernetConnection(void)
//
//  This is used to test Ethernet Connection
//------------------------------------------------------------------------------
bool TestEthernetConnection(void)
{
  if (clientETH.connect(testServer, 80)) {
    Serial.println("ping connection success");
    // Make a HTTP request:
    clientETH.println("GET /search?q=arduino HTTP/1.1");
    clientETH.println("Host: www.google.com");
    clientETH.println("Connection: close");
    clientETH.println();
    gEthernetDriver.isEthComError = false;
    return true;
  }
  else {
    // kf you didn't get a connection to the server:
    Serial.println("ping Connection failed");
    gEthernetDriver.isEthComError = true;
    return false;
  }  
}
