#ifndef MQTT_H
#define MQTT_H
#include "Arduino.h"
#include "WiFiManager.h"
#include <PubSubClient.h>

//para el sensor de temperatura
#include <DallasTemperature.h> // for DS18B20 

class Mqtt
{
    public:
        Mqtt();
        void setup();
        void loop();
        void publicaSmartData();
        void setTimeEveryPublishMillis(uint32 newTime);
        uint32 getTimeEveryPublishMillis();
        
        PubSubClient mqttClient;
        //Variable auxiliar para montar cadenas
        #define MAX_MSG_LENGTH 50
        char msg[MAX_MSG_LENGTH+1];

        
    private:
        const char * OUT_TOPIC="out/";
        const char * IN_TOPIC="in/#";
        #define IN_SUBTOPIC_RESET "reset"

        //Si no se puede conectar cada cuanto lo intenta
        uint32_t lastAttempToReconnect=0;
        uint32 timeEveryPublishMillis=5000;  
        uint32_t publishMqttCheckMillLast = 0; 

        void reconnect();
        void callback(char* topic, byte* payload, unsigned int length);
        void getSubTopic(char * topic,char * subTopic);
        int convertToNumber(byte *payload, uint length);
        long lastMsg = 0;
        int value = 0;
        int mqttLengthTRaiz=0;
};
extern Mqtt mqtt;
extern WiFiClient client;
extern char mqtt_server[]; //#define SERVER "192.168.0.104" 
extern char mqtt_port[];
extern char mqtt_topic[];
extern char mqtt_user[];
extern char mqtt_password[];

//Las variables del lector de corriente, que están declaradas e informadas en el main
extern float AC_Voltage;
extern float Power;
extern float AC_Current;
extern float Frequency;
extern float Energy;
extern float PowerFactor;
//Las variables de los sensores de temperatura
extern DallasTemperature DS18B20;
extern uint8_t numberOfDevices;

#endif