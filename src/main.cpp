#include <Arduino.h>
#include <ArduinoOTA.h> // for OTA
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include "LittleFS.h" // Include the LittleFS library
#include <time.h>
#include <ArduinoJson.h> // json
#include <StreamString.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <PZEM004Tv30.h>
#include <Mqtt.h>

// En el fichero serverHtmlPages.cpp se encuentran las respuestas que da el servidor http
// #include <serverHtmlPages.h>

// Parámetros del wifimanager para establecer el mqtt
// #include <wifiManagerParams.h>

// El websocket para enviar los datos instantáneos y gráficas se encuentra en
// #include <controlWebSocket.h>

// Las funciones del sensor de temperatura DS18B20 se encuentran en
// #include <leerTemperatura.h>

WiFiManager wm;
extern void addParametersToWm();
extern bool forceStartConfigPortalWm;     // para arrancar el portal de configuración de wifimanager obligándolo
extern void grabarParametrosFileSystem(); // para grabar cambios en los parámetros de configuración
extern Mqtt mqtt;

// Funciones con los datos del sensor y las páginas a servir en el servidor
// Se encuentran en serverHtmlPages.cpp
extern String smartData();
extern String jsonFeedGet();
extern void serveHtml();

// Funciones para manejar el websocket se encuentran en controlaWebSocket.cpp
extern WebSocketsServer webSocketSRVR;
extern void startWebSocket();

// Funciones para leer la temperatura con el DS18B20
extern void startDS18B20();
extern void loopDS18B20();

/*
    Hardware Serial. Porque el otro Serial que tiene sólo es de envío TXD1
*/
PZEM004Tv30 pzem(&PZEM_PORT);

// For wifi beacons
extern "C"
{
#include "user_interface.h"
}

WiFiClient client;

bool WifiOnlineStateFlag = false; //  false - offline, true - online

const uint32_t UpTimeReportInt = 60000;  // Set ms intervals to report upTime via Serial
uint32_t previousMillisUpTimeReport = 0; // will store last time when was upTime report

uint32_t wifiApCheckMillLast = 0; // for wifi in offline mode

uint32_t loopsPerSec = 0;
uint32_t loopsCount = 0;
uint32_t prevMillisLoops = 0;

float AC_Voltage = 0;
float Power = 0;
float AC_Current = 0;

float Frequency = 0;
float Energy = 0;
float PowerFactor = 0;

uint32_t showInfoSet = 0;

uint32_t prevMillisBeacon = 0;

uint32_t prevMillisMeasure = 0;

uint32_t prevMillisSwitchSetMill = 0;

const uint8_t indicPinBLUE = 2; // pin for indication

ESP8266WebServer server(80);

int rssi; // for signal level

bool PublicAccess = DEFAULT_PUBLIC_ACCESS; // authorization

// UptimeVaribles_START
uint32_t days = 0;
uint32_t hours = 0;
uint32_t mins = 0;
uint32_t secs = 0;
// UptimeVaribles_END

// variable para hacer saltar el diferencial y hacer varios reintentos en el loop, sin bloquear el esp
uint32_t timerPowerOffDifferential = 0;
uint8 powerOffRetry = -1; // para tener una secuencia a la hora de hacer saltar el diferencial

uint32_t timerButtonFactoryResetPress=0;
bool buttonFactoryResetPulsed=false;

//Para saber si tenemos el portal de configuración bajo demanda arrancado
bool configPortalRunningOnDemmand=false;

// Lee del pzem
void handleMeasurements()
{

    // ===== Measure
    if (millis() > prevMillisMeasure + 800)
    {
        AC_Voltage = isnan(pzem.voltage()) ? 0 : pzem.voltage();
        AC_Current = isnan(pzem.current()) ? 0 : pzem.current();
        Power = isnan(pzem.power()) ? 0 :pzem.power();
        Frequency = isnan(pzem.frequency()) ? 0 :pzem.frequency();
        Energy = isnan(pzem.energy()) ? 0 :pzem.energy();
        PowerFactor = isnan(pzem.pf()) ? 0 :pzem.pf();
        prevMillisMeasure = millis();
    } // ===== Measure
}

//========================Smart delay function==========================================///
void delaySmart(unsigned long x)
{
    unsigned long startM = millis();
    unsigned long t = millis();
    while (t - startM < x)
    {
        t = millis();
        //==========Place code here that shoud be exucute while we in delaySmart
        yield();
        server.handleClient();
        ArduinoOTA.handle();
        webSocketSRVR.loop();
        //==========Place code
    }
}
//========================Smart delay function==========================================///


void setup()
{

    // preparing GPIOs
    pinMode(indicPinBLUE, OUTPUT);   // indication works as an output
    digitalWrite(indicPinBLUE, LOW); // Indication pin set for low.

    pinMode(BUTTON_FACTORY_RESET, INPUT_PULLUP);  //Por defecto es pullup el gpio0 que es el botón flash del esp8266

    // PowerOffPin. Relé para hacer saltar diferencial
    pinMode(POWER_OFF_DIFFERENTIAL_PIN, OUTPUT);
    digitalWrite(POWER_OFF_DIFFERENTIAL_PIN, LOW);

    DEBUG_PORT.begin(9600);

    ArduinoOTA.onEnd([]() { // do a fancy thing with our board led at end
        for (int i = 0; i < 30; i++)
        {
            analogWrite(indicPinBLUE, (i * 100) % 1001);
            delay(25);
        }

        // restart after ota
        ESP.restart();
    });

    // Inicializamos el Filesystem
    if (!LittleFS.begin())
    {
        DEBUG_PORT.println(F("LittleFS: Failed to mount file system!"));
    }
    else
    {
        DEBUG_PORT.println(F("LittleFS: Ok, File system mouted!"));
    }

    WiFi.hostname(AP_NAME);
    delay(500);
    WiFi.mode(WIFI_STA); // STA mode.
    delay(600);

    // Iniciar mDNS a direccion esp8266.local
    if (!MDNS.begin(AP_NAME))
    {
        DEBUG_PORT.println("Error iniciando mDNS");
    }
    else
    {
        DEBUG_PORT.println("mDNS iniciado");
    }
    DEBUG_PORT.println("===============================");
    DEBUG_PORT.println(F("WIFI_STA mode"));

    // añadimos los parámetros de configuración de mqtt al wifimanager
    addParametersToWm();

    

    // automatically connect using saved credentials if they exist
    // If connection fails it starts an access point with the specified name
    char ApNameOffline[30];
    snprintf(ApNameOffline, 30, "%s | %s", AP_NAME, "Offline");

    
    // Nos han pedido que forcemos la entrada en el portal de configuración del wifimanager
    if (forceStartConfigPortalWm)
    {
        configPortalRunningOnDemmand=true;

        wm.setBreakAfterConfig(true);

        wm.setConfigPortalBlocking(true);


        DEBUG_PORT.println("Arrancando portal de configuración a demanda.");
        // Si por tres minutos no se hace nada se reiniciará
        wm.setConfigPortalTimeout(180);

        // Hemos arrancado el portal de configuración del wifimanager. En el próximo reinicio se hará una ejecución normal.
        forceStartConfigPortalWm = false; // al siguiente reinicio entramos normalmente
        grabarParametrosFileSystem();

        wm.startWebPortal(); // Este permite que se ponga la configuración en modo STATION
        // wm.startConfigPortal(AP_NAME,AP_PASSWORD);  //Este monta un punto de acceso pero no el modo STATION

        delay(500);

    }
    else
    {
        // Modo por defecto. Se autoconecta si hay credenciales o arranca el modo portal si no hay credenciales wifi
        wm.setConfigPortalBlocking(true);
        if (wm.autoConnect(ApNameOffline, AP_PASSWORD))
        {
            DEBUG_PORT.println("connected...yeey :)");
            delay(500);
            WifiOnlineStateFlag = true; // wifi in online state

            // wm.startConfigPortal("PowerControlESP");
            //  Indicate the start of connectrion to AP
            digitalWrite(indicPinBLUE, HIGH);
            delay(600);
            digitalWrite(indicPinBLUE, LOW);
            delay(175);
            digitalWrite(indicPinBLUE, HIGH);
            delay(175);
            digitalWrite(indicPinBLUE, LOW);
            delay(175);
            digitalWrite(indicPinBLUE, HIGH);
            //
        }
        else
        {
            DEBUG_PORT.println(F("Config portal running:"));
            DEBUG_PORT.println(ApNameOffline);
            WifiOnlineStateFlag = false;
            DEBUG_PORT.println(F("==============================="));
            DEBUG_PORT.println(F("OFFLINE MODE !!!"));
        }
    }

    ArduinoOTA.setHostname(AP_NAME);
    ArduinoOTA.begin(); // for OTA

    delay(500);

    // time config for spain
    configTime(1 * 3600, 0, "pool.ntp.org");
    
    // Websocket
    startWebSocket();

    // donde tenemos las páginas web que servimos en el servidor
    serveHtml();

    // These 3 lines tell esp to collect User-Agent and Cookie in http header when request is made
    // Sin estas tres líneas no se puede listar las cookies correctamente con las funciones:
    // server.hasHeader("Cookie") (está para saber si existe la cabecera Cookie)
    // server.header("Cookie")  que son todas las cookies
    // lo usamos para saber si tenemos la api-key en el navegador

    const char *headerkeys[] = {"User-Agent", "Cookie"};
    size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
    server.collectHeaders(headerkeys, headerkeyssize);

    server.begin();
    DEBUG_PORT.println(F("HTTP server started"));

    mqtt.setup();

    // Inicialimos los ds18b20 disponibles
    startDS18B20();

}

void loop()
{
    //Lo pongo porque para el esp8266 parece que hace falta
    //aunque me ha funcionado sin el 
    #ifdef ESP8266
        MDNS.update();
    #endif

    //Si lo quito el portal bajo demanda no me funciona.
    //No hace falta para la primera ejecución de configuración del esp8266 porque tengo wm.setConfigPortalBlocking(true)
    //Pero si intento iniciar el portal bajo demanda con la petición http://xxx.xxx.xxx.xxx/startWifiManagerPortal no funciona si no hago wm.process()
    //Intenté poner una variable a todo el loop para que se lo saltara cuando está el portal inicial de configuración o el de bajo demanda y tampoco funcionaba
    //Por lo tanto la única manera de que funcione es esta. Al final está el portal arrancado de cualquiera de las dos maneras y funcionando toda la lógica del programa.
    if (configPortalRunningOnDemmand)
        wm.process();

    /*Leemos el botón y si nos pulsan un 15 segundos sin soltar hacemos un factory reset del esp*/
    //Para controlar los rebotes sólo se lo pongo al comprobar el estado alto.
    if (digitalRead(BUTTON_FACTORY_RESET)==LOW){
        if (!buttonFactoryResetPulsed){
            timerButtonFactoryResetPress=millis()+15000; //+ 15 segundos
            buttonFactoryResetPulsed=true;
            DEBUG_PORT.println(F("Botón de factory reset pulsado."));

        }else if (millis()> timerButtonFactoryResetPress ) {  //Si ha cumplido el tiempo con el botón pulsado hacemos reset
            DEBUG_PORT.println(F("****REALIZANDO FACTORY RESET*****"));
            delay(1000);
            LittleFS.remove("/config.json");
            wm.erase();
            ESP.restart();
        }
        
    }else if (buttonFactoryResetPulsed){
        delay(20); //para controlar los rebotes
        if (digitalRead(BUTTON_FACTORY_RESET)==HIGH){
            buttonFactoryResetPulsed=false;
            DEBUG_PORT.println(F("Botón de factory reset soltado antes de hacer el factory reset."));
        }
    }


    ArduinoOTA.handle(); // for OTA

    // Nos han dicho que intentemos hacer saltar el diferencial con un relé
    // via web con /powerOffDifferential?pin=xxxx
    if (powerOffRetry != -1)
    {
        switch (powerOffRetry)
        {
        case 1:
            // Si han pasado 2 segundos desde que encendí el relé para hacer saltar el diferencial
            if (millis() > timerPowerOffDifferential + 2000)
            {
                // apago relé
                digitalWrite(POWER_OFF_DIFFERENTIAL_PIN, LOW);
                powerOffRetry = 2;
                timerPowerOffDifferential = millis();
                DEBUG_PORT.println(F("POWER OFF DIFFERENTIAL SEQUENCE: 1. Vamos a apagar el rele 800ms"));
            }
            break;
        case 2:
            // si han pasado 800ms desde que apagué el relé
            if (millis() > timerPowerOffDifferential + 800)
            {
                // enciendo relé nuevamente
                digitalWrite(POWER_OFF_DIFFERENTIAL_PIN, HIGH);
                powerOffRetry = 3;
                timerPowerOffDifferential = millis();
                DEBUG_PORT.println(F("POWER OFF DIFFERENTIAL SEQUENCE: 2. Vamos a encender el rele otros 4000ms"));
            }
            break;
        case 3:
            // si han pasado 4000ms desde que encendí el relé. Es la segunda vez y llego aquí lo doy por imposible
            // fracaso absoluto
            if (millis() > timerPowerOffDifferential + 4000)
            {
                // apagamos el rele
                digitalWrite(POWER_OFF_DIFFERENTIAL_PIN, LOW);
                // reinicializamos el proceso de powerOff e
                powerOffRetry = -1;
                timerPowerOffDifferential = 0;
                DEBUG_PORT.println(F("POWER OFF DIFFERENTIAL SEQUENCE: 3. Hemos intentado hacer saltar el diferencial y no ha sido posible"));
            }
        }
    }

    handleMeasurements();

    server.handleClient();

    webSocketSRVR.loop();

    mqtt.loop();

    webSocketSRVR.loop();

    // Lee el/los sensores de temperatura
    loopDS18B20();

    //=========loopsCounter========================//
    loopsCount++;
    if (millis() > prevMillisLoops + 500)
    {
        prevMillisLoops = millis();
        loopsPerSec = loopsCount;
        loopsPerSec = loopsPerSec * 2;
        loopsCount = 0;
    }
    //=========loopsCounter========================//

    // in offline mode check for AP peridiodicly, if available then reboot.
    if (WifiOnlineStateFlag == false && millis() - wifiApCheckMillLast > 120000)
    {
        wifiApCheckMillLast = millis();
        DEBUG_PORT.println("===============================");
        DEBUG_PORT.print("Checking if "); // if AP available
        DEBUG_PORT.print(WiFi.SSID());
        DEBUG_PORT.println(" available");

        int n = WiFi.scanNetworks(); // n of networks
        bool foundCorrectSSID = false;
        DEBUG_PORT.println("Found " + String(n) + " networks");
        for (int x = 0; x != n; x++)
        {
            DEBUG_PORT.println(WiFi.SSID(x));
            if (WiFi.SSID(x) == WiFi.SSID())
            {
                foundCorrectSSID = true;
            }
        }
        DEBUG_PORT.println("----");
        if (foundCorrectSSID)
        {
            DEBUG_PORT.println("Found " + String(WiFi.SSID()));
            DEBUG_PORT.println(F("Restarting ESP!!!"));
            delay(1000);
            ESP.restart();
        }
        else
        {
            DEBUG_PORT.println("Not Found " + String(WiFi.SSID()));
        }
    }

    // Lost connection with wi-fi
    if ((WiFi.status() != WL_CONNECTED) && (WifiOnlineStateFlag == true))
    {
        DEBUG_PORT.println("===============================");
        DEBUG_PORT.println(F("Lost connection to wi-fi"));
        // if wi-fi is not reconnected within 60 second then reboot ESP
        for (int i = 60; i >= 0; i--)
        {

            DEBUG_PORT.print(F("Restarting in: "));
            DEBUG_PORT.print(i);
            DEBUG_PORT.println(F(" sec."));

            delaySmart(750);

            if (WiFi.status() == WL_CONNECTED)
            {
                i = -1;
                DEBUG_PORT.println(F("Reconnected to wi-fi"));
            }
        }
        if ((WiFi.status() != WL_CONNECTED) && (WifiOnlineStateFlag == true))
        {
            ESP.restart();
        }
    }
    ///==========AP State==========//

    ////====================UpTimeReportStart======================///
    // calculate

    secs = millis() / 1000;      // convect milliseconds to seconds
    mins = secs / 60;            // convert seconds to minutes
    hours = mins / 60;           // convert minutes to hours
    days = hours / 24;           // convert hours to days
    secs = secs - (mins * 60);   // subtract the coverted seconds to minutes in order to display 59 secs max
    mins = mins - (hours * 60);  // subtract the coverted minutes to hours in order to display 59 minutes max
    hours = hours - (days * 24); // subtract the coverted hours to days in order to display 23 hours max

}
