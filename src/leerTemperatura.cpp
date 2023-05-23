#include <Arduino.h>
#include <leerTemperatura.h>

//Se inicializa el ds18b20
void startDS18B20(){
    DS18B20.begin();
    numberOfDevices = DS18B20.getDeviceCount();
    DEBUG_PORT.print(F( "Found DS18B20 Devices: " ));
    DEBUG_PORT.println( numberOfDevices );
    //========CountEnd==========///
    if (numberOfDevices>0){
      
      // conseguimos y rellenamos un array con las direcciones de los ds18b20 detectados y le indicamos la resolución
      for (int i=0;i<numberOfDevices;i++){

          DS18B20.getAddress(devAddr[i], i);
          DS18B20.setResolution(devAddr[i], 12);

      }

      // This line disables delay in the library DallasTemperature
      DS18B20.setWaitForConversion(false);
      
      //Le decimos que empiece a medir, y cada 800ms es el mínimo tiempo que necesita para coger la temperatura
      DS18B20.requestTemperatures(); // Request temperature mesurments
      previousMillisSenseUp = millis();
    }
}

void loopDS18B20(){
  //Make requst
  if (numberOfDevices>0){
    if (millis() - previousMillisSenseUp >= waitSenseInt) {
      
      ///Meausure temperature

      DEBUG_PORT.println(F("=================================="));
      DEBUG_PORT.println(F("Request to measure temperatures... "));
      DEBUG_PORT.println(F("=================================="));
      DS18B20.requestTemperatures(); // Request temperature mesurments
      //MillisOneWireTempRequst = millis();
      DEBUG_PORT.println("Done...");
      //TempRequstDone = true;

      // save the last time temperature was updated
      previousMillisSenseUp = millis();
    }
  }
}

//===================for one wire=========================//
//Convert device id to String / for one wire adress server
String GetAddressToString(DeviceAddress deviceAddress){
  String str = "";
  for (uint8_t i = 0; i < 8; i++){
    str += "0x";    
    if( deviceAddress[i] < 16 ){
    str += String(0, HEX);
    }
    str += String(deviceAddress[i], HEX);
           if(i < 7)
       {
        str += ", ";
       }
  }
  return str;
}

//===============================DS_readCheck==============================//
bool DsReadGood(float DS_reding) {
  if (DS_reding > -50 && DS_reding != 85  && DS_reding < 110) {
    return true;
  }
  return false;
}
//===============================DS_readCheck==============================//