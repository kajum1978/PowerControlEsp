#ifndef WIFI_MANAGER_PARAMS_H
    #define WIFI_MANAGER_PARAMS_H
    #include <ArduinoJson.h> 
    #include "LittleFS.h" // Include the LittleFS library
    #include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
    #include <Mqtt.h>

    extern WiFiManager wm;

    extern Mqtt mqtt;
    
    //añadir parámetros al portal wifi manager
    //para poder configurar el mqtt

    char mqtt_server[16] = MQTT_SERVER; 
    char mqtt_port[6] = MQTT_PORT;
    char mqtt_user[15] = MQTT_USER; 
    char mqtt_password[20] = MQTT_PASSWORD; 
    char mqtt_topic[30] = MQTT_TOPIC;

    //Como no caben en el wifimanager mucho, estos siguientes van por libre
    char api_token[30] = ""; //Este no se añade porque es muy grande y no lo soporta el wifi manager. Se genera automáticamente al grabarse por primera vez el fichero de configuración
    char powerOffPinCode[5] = POWER_OFF_DIFFERENTIAL_PIN_CODE;
    uint32 mqttSendInterval =  DEFAULT_MQTT_SEND_INTERVAL;

    //vamos a usar esta variable para indicar si al arrancar el esp. Arrancamos la configuración del wifimanager.
    bool forceStartConfigPortalWm=false;

    //Parámetros Wifi manager
    //Tienen que ser globales. También se puede hacer con unos punteros y definirlas en una función.
    //Si se meten muchos tamaño del parámetro da un out of scope
    //WiFiManagerParameter custom_text("<p style='font-weight:bold'>Insert mqtt server address, port, user, pass and topic</p>");
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 20);
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
    WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, 20);
    WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password, 20);
    WiFiManagerParameter custom_mqtt_topic("topic", "mqtt topic", mqtt_topic, 50);
    //WiFiManagerParameter custom_api_token("token", "Api token", api_token, 60);

/*    extern WiFiManagerParameter custom_mqtt_server;
    extern WiFiManagerParameter custom_mqtt_port;
    extern WiFiManagerParameter custom_mqtt_user;
    extern WiFiManagerParameter custom_mqtt_password;
    extern WiFiManagerParameter custom_mqtt_topic;
    //extern WiFiManagerParameter custom_api_token;
*/

    void addParametersToWm();
    void leerParametrosFileSystem();
    void grabarParametrosFileSystem();
    void generateNewApiToken();
    

#endif