



// Functions
//------------------------------------------------------------------------------
//  static void InitMQTTClient(void )
//
//  This function is used to initialized the MQTT Client
//------------------------------------------------------------------------------
void InitMQTTClient(void);
//------------------------------------------------------------------------------
//  bool GetMQTTClientConnectionStatus(void)
//
//  This function is used to Get the status of MQTT Connection
//------------------------------------------------------------------------------
bool GetMQTTClientConnectionStatus(void);
//------------------------------------------------------------------------------
//  void MQTTRefreshConnection(void)
//
//  This function is used to refresh MQTT Connection
//------------------------------------------------------------------------------
void MQTTRefreshConnection(void);
//------------------------------------------------------------------------------
//  void MQTTreconnect(void)
//
//  This function is used to reconnect to the MQTT broker
//------------------------------------------------------------------------------
void MQTTreconnect(void);


