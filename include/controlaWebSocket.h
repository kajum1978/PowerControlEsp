#pragma once
#include <WebSocketsServer.h>

// WebsocketSesver
WebSocketsServer webSocketSRVR = WebSocketsServer(8092);

void startWebSocket();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);


extern String smartData();
extern String jsonFeedGet();
