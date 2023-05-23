#include <Arduino.h>
#include <wifiManagerParams.h>

void addParametersToWm(){
  //Si muestra el portal de configuración, se añaden unos campos para configurar el servidor mqtt
  //Se añaden los parámetros a la interfaz de wifimanger
  //wm.addParameter(&custom_text);
  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_password);
  wm.addParameter(&custom_mqtt_topic);

  leerParametrosFileSystem();

  wm.setSaveParamsCallback(grabarParametrosFileSystem);
}

void leerParametrosFileSystem(){
    if (LittleFS.exists("/config.json")) {
      //file exists, reading and loading
      DEBUG_PORT.println("reading config file");
      File configFile = LittleFS.open("/config.json", "r");
      if (configFile) {
        DEBUG_PORT.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( ! deserializeError ) {

            DEBUG_PORT.println("\nparsed json");

            //Copiamos lo que tenemos en el json a nuestras variables de configuración
            strcpy(mqtt_server, json["mqtt_server"]);
            strcpy(mqtt_port, json["mqtt_port"]);
            strcpy(mqtt_user, json["mqtt_user"]);
            strcpy(mqtt_password, json["mqtt_password"]);
            strcpy(mqtt_topic, json["mqtt_topic"]);
            mqttSendInterval=json["mqttSendInterval"];
            strcpy(api_token, json["api_token"]);
            strcpy(powerOffPinCode,json["powerOffPinCode"]);
            forceStartConfigPortalWm=json["forceStartConfigPortalWm"];
            
            //también actualizamos los valores de los parámetros de wifimanager
            custom_mqtt_server.setValue(mqtt_server,16);
            custom_mqtt_port.setValue(mqtt_port,6);
            custom_mqtt_user.setValue(mqtt_user,15);
            custom_mqtt_password.setValue(mqtt_password,20);
            custom_mqtt_topic.setValue(mqtt_topic,30);

            //actualizamos el valor del tiempo, para indicar cada cuanto se actualizan los valores enviados por mqtt
            mqtt.setTimeEveryPublishMillis(mqttSendInterval);
           
            DEBUG_PORT.println("The values loaded in the config file are: ");
            DEBUG_PORT.println("\tmqtt_server : " + String(mqtt_server));
            DEBUG_PORT.println("\tmqtt_port : " + String(mqtt_port));
            DEBUG_PORT.println("\tmqtt_user : " + String(mqtt_user));
            DEBUG_PORT.println("\tmqtt_password : " + String(mqtt_password));
            DEBUG_PORT.println("\tmqtt_topic : " + String(mqtt_topic));
            DEBUG_PORT.println("\tmqttSendInterval : " + String(mqttSendInterval,DEC));
            DEBUG_PORT.println("\tapi_token : " + String(api_token));
            DEBUG_PORT.println("\tpowerOffPinCode :" + String(powerOffPinCode));
            DEBUG_PORT.println("\tforceStartConfigPortalWm :" + String(forceStartConfigPortalWm));

        } else {
            DEBUG_PORT.println("failed to load json config");
        }
        configFile.close();
      }
    }
    else{
        DEBUG_PORT.println("doesn´t exist config file. Creating default config:");
        generateNewApiToken();
        //Grabamos los parámetros iniciales
        grabarParametrosFileSystem();
    }
    
}

//Genera un api token automáticamente.
//Creo que rand funciona con millis() y la primera ejecución siempre genera el mismo api_token.
//Lo podría poner en platfomio.ini con un defines, pero lo pongo que se autogenere.
//Se puede cambiar a otro autogenerado con /newapitoken
void generateNewApiToken(){
  DEBUG_PORT.println("Generating new api token:");
  //Array para coger las letras, números,símbolos
  char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";
  
  //longitud del token que debemos generar
  uint8 length=sizeof(api_token)-1;
  //valor donde guardaremos el indice random generado
  uint8 auxKeyRand;
  for (int i=0; i<length;i++){
      //hacemos un random para que nos de un valor aleatorio del tamaño total de charset
      auxKeyRand = rand() % (int)(sizeof(charset) -1);
      //asignamos el valor a nuestra variable api_token
      api_token[i]=charset[auxKeyRand];
  }
  api_token[length]='\0'; //ponemos el final de cadena
  DEBUG_PORT.print(" api token: ");
  DEBUG_PORT.println(api_token);
}

void grabarParametrosFileSystem(){

    //Han podido ser actualizados por wifimanager o por la primera vez que se genera el fichero, con los valores por defecto
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(mqtt_user, custom_mqtt_user.getValue());
    strcpy(mqtt_password, custom_mqtt_password.getValue());
    strcpy(mqtt_topic, custom_mqtt_topic.getValue());
    
    DEBUG_PORT.println("Saving config: ");
    DEBUG_PORT.println("The values to save in the config file are: ");
    DEBUG_PORT.println("\tmqtt_server : " + String(mqtt_server));
    DEBUG_PORT.println("\tmqtt_port : " + String(mqtt_port));
    DEBUG_PORT.println("\tmqtt_user : " + String(mqtt_user));
    DEBUG_PORT.println("\tmqtt_password : " + String(mqtt_password));
    DEBUG_PORT.println("\tmqtt_topic : " + String(mqtt_topic));
    DEBUG_PORT.println("\tmqttSendInterval : " + String(mqttSendInterval,DEC));
    DEBUG_PORT.println("\tapi_token : " + String(api_token));
    DEBUG_PORT.println("\tpowerOffPinCode :" + String(powerOffPinCode));
    DEBUG_PORT.println("\forceStartConfigPortalWm :" + String(forceStartConfigPortalWm));

    //save the custom parameters to FS
  
    DEBUG_PORT.println("saving config");

    DynamicJsonDocument json(1024);

    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_password"] = mqtt_password;
    json["mqtt_topic"] = mqtt_topic;
    json["mqttSendInterval"] = mqttSendInterval;
    json["api_token"] = api_token;
    json["powerOffPinCode"] = powerOffPinCode;
    json["forceStartConfigPortalWm"]=forceStartConfigPortalWm;
    

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
      DEBUG_PORT.println("failed to open config file for writing");
    }

    serializeJson(json, DEBUG_PORT);
    serializeJson(json, configFile);

    configFile.close();
    //end save
    
    //Volvemos a reconfigurar mqtt
    //No pongo aquí el mqtt.setTimeEveryPublishMillis(xxxx), porque se realiza en la petición al servidor /mqttIntervalData?setUpdt=xxx
    mqtt.mqttClient.disconnect();
    mqtt.setup();

}