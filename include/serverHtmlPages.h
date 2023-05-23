#pragma once
#include <ESP8266WebServer.h>
#include "LittleFS.h" // Include the LittleFS library
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <DallasTemperature.h> // for DS18B20 
#include <Mqtt.h>
#include <ArduinoJson.h> // json

//Variables/funciones ya declaradas en otro .cpp (en el main.cpp)
extern bool PublicAccess;
extern ESP8266WebServer server;
extern const uint8_t indicPinBLUE;

extern float AC_Voltage;
extern float Power;
extern float AC_Current;

extern float Frequency;
extern float Energy;
extern float PowerFactor;
extern int rssi;
extern bool WifiOnlineStateFlag;
extern uint32_t loopsPerSec;

//Wifimanager para cuando queremos hacer un factory reset.
extern WiFiManager wm;

//El pin del led, que se encuentra definido en el main, en c++ la constante debe ser con la definición completa
extern const uint8_t indicPinBLUE = 2; // pin for indication;

//UptimeVaribles_START
extern uint32_t days;
extern uint32_t hours;
extern uint32_t mins;
extern uint32_t secs;
//UptimeVaribles_END

//DS18B20
extern String GetAddressToString(DeviceAddress deviceAddress);
extern DallasTemperature DS18B20;
extern DeviceAddress devAddr[];
extern char api_token[];
extern bool DsReadGood(float DS_reding);
extern uint8_t numberOfDevices;

//Mqtt
extern Mqtt mqtt;

//Para grabar los parámetros. Si me cambian el sendMqttInterval con /mqttIntervalData?setUpdt=10
extern void grabarParametrosFileSystem();
extern uint32 mqttSendInterval;

//para generar un nuevo api_token.
extern void generateNewApiToken();

//Definidas en el main, para hacer saltar el diferencial sin bloquear la ejecución
extern uint32 timerPowerOffDifferential;
extern uint8 powerOffRetry;

//código para hacer saltar el diferencial
extern char powerOffPinCode[5];

//Flag para indicar si en el reinicio hay que lanzar el portal de configuración
extern bool forceStartConfigPortalWm;

//Variables/funciones en este .cpp
String smartData();
String jsonFeedGet();
void serveHtml();