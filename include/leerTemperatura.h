#pragma once
#include <Arduino.h>
#include <OneWire.h>           // for DS18B20
#include <DallasTemperature.h> // for DS18B20
#include <WebSocketsServer.h>

// DS18B20
#define ONE_WIRE_MAX_DEV 10 // The maximum number of devices

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

uint8_t numberOfDevices; // Number of temperature devices found

const uint32_t waitSenseInt = 8000; // my variable to set ms intervals to measure temperature,
uint32_t previousMillisSenseUp = 0; // will store last time when rempareture was updated

// ds18b20 functions
bool DsReadGood(float DS_reding);
DeviceAddress devAddr[ONE_WIRE_MAX_DEV];

// Sensor adreses
//No se necesita, lo detecta la librería en teoría
//DeviceAddress EXT_SENSOR1 = {0x28, 0x91, 0xcf, 0xfc, 0x08, 0x00, 0x00, 0x93}; // inside sensor
// Sensor adreses end

// Convierte la dirección del ds18b20 en un String para mostrarlo entendible cuando se le solicita
String GetAddressToString(DeviceAddress deviceAddress);

// variables/funciones externas necesarios
extern const uint8_t indicPinBLUE;
extern void delaySmart(unsigned long x);
extern WebSocketsServer webSocketSRVR;
