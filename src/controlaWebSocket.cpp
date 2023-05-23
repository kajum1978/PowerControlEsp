#include <Arduino.h>
#include <controlaWebSocket.h>

//Iniciamos el websocket y ponemos un evento
void startWebSocket(){
    webSocketSRVR.begin();
    webSocketSRVR.onEvent(webSocketEvent);
}

//=======================webSocket==================================================//
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  String text = String((char *) &payload[0]);

  switch (type) {
    case WStype_TEXT:

      if (text.startsWith("smartDataGet")) {
        String readyData = smartData();
        webSocketSRVR.sendTXT(num, readyData);
      }

      if (text.startsWith("jsonDataGet")) {
        String readyData = jsonFeedGet();
        webSocketSRVR.sendTXT(num, readyData);
      }
      
      break;

    //===

    case WStype_DISCONNECTED: {  // if the client is disconnected
        DEBUG_PORT.println("===============================");
        DEBUG_PORT.printf("[%u] Disconnected!\n", num);
      }
      break;

    //===

    case WStype_CONNECTED: {    // if a new websocket connection is established
        IPAddress ip_rem = webSocketSRVR.remoteIP(num);
        DEBUG_PORT.println("===============================");
        DEBUG_PORT.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip_rem[0], ip_rem[1], ip_rem[2], ip_rem[3], payload);
      }
      break;

    //===

    case WStype_BIN:
      DEBUG_PORT.printf("[%u] get binary length: %u\n", num, length);
      webSocketSRVR.sendBIN(num, payload, length);
      break;

      //===

  }
}
//=======================webSocketServerEND============================================//
