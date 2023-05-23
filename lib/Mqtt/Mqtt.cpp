/* Librería para manejar el Mqtt */
#include "Arduino.h"
#include "Mqtt.h"

Mqtt mqtt;
Mqtt::Mqtt():mqttClient(client){
  
}

//Esta función publica los datos a través de mqtt
void Mqtt:: publicaSmartData(){

  char buffer[10];
  sprintf(buffer,"%.2f",AC_Voltage);
  snprintf (msg, MAX_MSG_LENGTH, "%s%sac_voltage",mqtt_topic,OUT_TOPIC);
  mqttClient.publish(msg,buffer);

  sprintf(buffer,"%.2f",Power);
  snprintf (msg, MAX_MSG_LENGTH, "%s%spower",mqtt_topic,OUT_TOPIC);
  mqttClient.publish(msg,buffer);

  sprintf(buffer,"%.2f",AC_Current);
  snprintf (msg, MAX_MSG_LENGTH, "%s%sac_current",mqtt_topic,OUT_TOPIC);
  mqttClient.publish(msg,buffer);

  sprintf(buffer,"%.2f",Frequency);
  snprintf (msg, MAX_MSG_LENGTH, "%s%sfrecuency",mqtt_topic,OUT_TOPIC);
  mqttClient.publish(msg,buffer);

  sprintf(buffer,"%.0f",PowerFactor*100);
  snprintf (msg, MAX_MSG_LENGTH, "%s%spower_factor",mqtt_topic,OUT_TOPIC);
  mqttClient.publish(msg,buffer);

  sprintf(buffer,"%.2f",Energy);
  snprintf (msg, MAX_MSG_LENGTH, "%s%senergy",mqtt_topic,OUT_TOPIC);
  mqttClient.publish(msg,buffer);

  time_t TimeNow = time(nullptr);
  sprintf(buffer,"%s",ctime(&TimeNow));
  snprintf (msg, MAX_MSG_LENGTH, "%s%stime_now",mqtt_topic,OUT_TOPIC);
  mqttClient.publish(msg,buffer);

  //publicamos las temperaturas
  for (int i=0;i<numberOfDevices;i++){
    sprintf(buffer,"%.2f",DS18B20.getTempCByIndex(i));
    snprintf (msg, MAX_MSG_LENGTH, "%s%stemp%i",mqtt_topic,OUT_TOPIC,i+1);
    mqttClient.publish(msg,buffer);
  }

}


int Mqtt::convertToNumber(byte *payload, uint length){
  int value=0;
  uint i,j,aux;
  bool error=false;
  
  //el payload lo convertimos a número y lo dejamos en value  
  for(i=0;i<length && !error;i++){
    if (payload[i]>47 && payload[i]<=57){
      aux=1;
      for(j=length-i-1;j>0;j--)
        aux=aux*10;
      value=value+((payload[i]-48)*aux);
    }else{
      //DEBUG_PORT.println("MqttError: No puedo convertir a número");
      error=true;
    }
  }
  if (error)
    value=-1;
  return value;
}



void Mqtt::getSubTopic(char * topic,char * subTopic){
  uint8 i,j;
  //sacamos el subTopic (quitamos la raiz y me quedo con el resto)
  i=mqttLengthTRaiz;
  j=0;
  while (topic[i]!='\0'){
     subTopic[j++]=topic[i++];
  }
  subTopic[j]='\0';
  
}

//Si llega algún mensaje donde está suscrito
void Mqtt::callback(char* topic, byte* payload, unsigned int length) {
  char subTopic[15];
  int value;

  DEBUG_PORT.print("Message arrived [");
  DEBUG_PORT.print(topic);
  DEBUG_PORT.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    DEBUG_PORT.print((char)payload[i]);
  }
  DEBUG_PORT.println();

  getSubTopic(topic,subTopic);
  DEBUG_PORT.println(subTopic);
  value=convertToNumber(payload,length);
  
  //Si nos envían un reset por mqtt
  if (strcmp(IN_SUBTOPIC_RESET,subTopic)==0 && value==1){
      DEBUG_PORT.println("Indicado reset por Mqtt.");
      delayMicroseconds(1000);
      ESP.reset();
  }

  //Examples of use
  /*
  //value=convertToNumber(payload,length);
    if (strcmp(IN_SUBTOPIC_APERTURA,subTopic)==0){
      
      //valores validos son entre 0 y 100
      if (value>=0 && value <101){
      
        DEBUG_PORT.print("Poniendo la rejilla abierta un ");
        DEBUG_PORT.print(value);
        DEBUG_PORT.println(" por ciento.");
        
        
      }
    }else if (strcmp(IN_SUBTOPIC_RESET,subTopic)==0 && value==1){
      DEBUG_PORT.println("Indicado reset por Mqtt.");
      delayMicroseconds(1000);
      ESP.reset();
    }else if (strcmp(IN_SUBTOPIC_CALIBRAR,subTopic)==0 && value==1){
      DEBUG_PORT.println("Calibrando rejilla.");
      
    }else if (strcmp(IN_SUBTOPIC_STEPS_FOR_CLOSE,subTopic)==0 && value>=1000 && value<=3000){
      DEBUG_PORT.println("Establecido cantidad de pasos para cerrar rejilla.");
      
    }
  */
 
}

void Mqtt::setup(){
    mqttClient.setServer(mqtt_server, atoi(mqtt_port));
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) mutable { this->callback(topic,payload,length); return;});
    //calculamos cuanto ocupa el TOPIC, para luego usarlo y sacar el subtopic
    mqttLengthTRaiz=strlen(IN_TOPIC)-1;
    
    //Inicializamos el temporizador para publicar los datos por mqtt
    publishMqttCheckMillLast=millis();

}

void Mqtt::loop(){
    uint32 auxMillis;
     //For Mqtt
    if (!mqttClient.connected()) {
      reconnect();
    }
    mqttClient.loop();

    //Si ha trancurrido el tiempo preestablecido en timeEveryPublishMillis. Se publican los datos
    auxMillis=millis() - publishMqttCheckMillLast;
    if ( auxMillis > timeEveryPublishMillis ) {
      publicaSmartData();
      //actualizamos el contador
      publishMqttCheckMillLast=millis();
    }
}

//Establece la variable para indicar cada cuanto tiempo se envían datos
uint32 Mqtt::getTimeEveryPublishMillis(){
    return timeEveryPublishMillis;
}

//Establece la variable para indicar cada cuanto tiempo se envían datos
void Mqtt::setTimeEveryPublishMillis(uint32 newTime){
    timeEveryPublishMillis=newTime;
}
        
void Mqtt::reconnect() {
  // Originalmente teníamos un while, ahora lo quitamos por un if.
  // Se puede llegar a bloquear el microcontrolador (se queda en bucle infinito)
  // e incluso el OTA, si no nos conseguimos conectar
  if (!mqttClient.connected()) {
    //cada 5 segundos intentamos reconectar si no lo conseguimos
    if (lastAttempToReconnect + 5000 < millis()) {
      DEBUG_PORT.print("Attempting MQTT connection...");
      // Create a random mqttClient ID
      String mqttClientId = "PowerControlESP-";
      mqttClientId += String(random(0xffff), HEX);
      // Attempt to connect
      if (mqttClient.connect(mqttClientId.c_str(),mqtt_user,mqtt_password)) {
        DEBUG_PORT.println("connected to Mqtt");
        DEBUG_PORT.print("Mqtt sends data every: ");
        DEBUG_PORT.print(timeEveryPublishMillis);
        DEBUG_PORT.println(" milliseconds.");
        // Once connected, publish an announcement...
        snprintf (msg, MAX_MSG_LENGTH, mqtt_topic,OUT_TOPIC);
        mqttClient.publish(msg, "hello world");
        // ... and resubscribe
        snprintf (msg, MAX_MSG_LENGTH, mqtt_topic,IN_TOPIC);
        mqttClient.subscribe(msg);
      } else {
        DEBUG_PORT.print("   failed, to connect to Mqtt rc=");
        DEBUG_PORT.print(mqttClient.state());
        DEBUG_PORT.println("   try again in 5 seconds" );
        lastAttempToReconnect=millis();
        //Se quita para no bloquar las demás acciones de microcontrolador
        // Wait 5 seconds before retrying
        //delay(5000);
      }
    }
  }
}