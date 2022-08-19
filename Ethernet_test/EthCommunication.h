#include <stdio.h>
#include <stdbool.h>

//------------------------------------------------------------------------------
//  Preprocessors Define
//------------------------------------------------------------------------------
#define RESET_P    26
#define CS_PIN      5
//------------------------------------------------------------------------------
//  ERRORCODE
//------------------------------------------------------------------------------
#define ERRORCODE_NO_HARDWARE_FOUND       700
#define ERRORCODE_NO_LINK_DETECTED        701

enum enumLink 
{
    UNKNOWN = 0,
    UP ,
    DOWN
};


typedef struct ethernetStruct
{
  bool linkStatus;
  char hardwareIdentifier[50];
  enum enumLink link;  
  bool isHardwareFound;
  bool isEthComError;
}ethStruct;

//------------------------------------------------------------------------------
//  void WizReset(void)
//
//  This is used to reset the W5500
//------------------------------------------------------------------------------
void W5500Reset(void);
//------------------------------------------------------------------------------
//  void LinkCheck(void)
//
//  This is used to check the Link of W5500
//------------------------------------------------------------------------------
void LinkCheck(void);
//------------------------------------------------------------------------------
//  void HardwareIdentification(void)
//
//  This is identify Hardware attached to ESP32
//------------------------------------------------------------------------------
void HardwareIdentification(void);

//------------------------------------------------------------------------------
//  uint16_t InitEthernet(void)
//
//  This is used to initialize Ethernet interface
//------------------------------------------------------------------------------
uint16_t InitEthernet(void);

//------------------------------------------------------------------------------
//  void TestEthernetConnection(void)
//
//  This is used to test Ethernet Connection
//------------------------------------------------------------------------------
void TestEthernetConnection(void);