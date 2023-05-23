#include <Arduino.h>
#include <serverHtmlPages.h>

//Comprueba si el api_token que tenemos grabado en el fichero de configuración es el mismo o no que en la cookie del navegador
//Devuelve true si PublicAccess=true
//Devuelve true si api_token==api_token cookie navegador
//En caso contrario devuelve falso y envia al navegador la respuesta
bool verifyAccessCookieToken(){
    String sessionToken = api_token; //pasamos nuestra variable char array a string
    bool retorno=false; //por defecto false

    server.sendHeader("access-control-allow-origin", "*");

    //Si está el acceso público autorizado. Todo ok
    if (PublicAccess){
        retorno=true;
    }else{ 
        //Si hay cookie alojada en el navegador
        if ( server.hasHeader("Cookie")){
            //si existe la cookie y coincide con el api_token grabado en el fichero de configuración
            if ( server.header("Cookie").indexOf(sessionToken)!=-1)
                retorno=true; //Todo ok
            else 
                server.send(200, "text/html", "Oh, Crap! Your cookie is not valid!");  // Error no autorizado con ese api_token
            
        }else{
            //No hay cookie
            server.send(200, "text/html", "Oh, Crap! You not supposed to be here!");   // Error no autorizado. No hay cookie
        }

    }
    
    return retorno;
}

void serveHtml(){
    server.on("/startWifiManagerPortal",[](){
        if ( verifyAccessCookieToken() ){
         forceStartConfigPortalWm=true;
         grabarParametrosFileSystem();
         //intentamos tras unos segundos cargar la página principal
         server.send(200, "text/html", "<script>setTimeout(function(){document.location.href='/';},5000); // 5000 milliseconds = 5 seconds </script><p>Restarting server. This page will be reloaded in 5 seconds with the config portal.</p>");
         delay(300);
         ESP.restart();
        }
    });
    
    server.on("/factoryReset",[](){
        if ( verifyAccessCookieToken() ){
         server.send(200, "text/html", "Factory reset done!. Restarting Esp in AP MODE with AP name: " + String(AP_NAME) + " and password:"+String(AP_PASSWORD)+"</p>");
         LittleFS.remove("/config.json");
         wm.erase();
         ESP.restart();
        }
    });
    
    server.on("/newApiToken",[](){
        //Si estamos autorizados se ejecuta, si no la función envía el error al navegador
        if ( verifyAccessCookieToken() ){
            generateNewApiToken();
            server.send(200, "text/html", "<p>New api_token generated: "+String(api_token)+"</p>");
            grabarParametrosFileSystem();
        }
    });
    server.on("/PublicAccessTrue", []() {
        //Si estamos autorizados se ejecuta, si no la función envía el error al navegador
        if ( verifyAccessCookieToken() ){
            server.send(200, "text/html", "<p>PublicAccess: enabled!</p>");
            PublicAccess = true;
        }
    });

    server.on("/PublicAccessFalse", []() {
        //Si estamos autorizados se ejecuta, si no la función envía el error al navegador
        if ( verifyAccessCookieToken() ){
            server.send(200, "text/html", "<p>PublicAccess: disabled!</p>");
            PublicAccess = false;
        }
    });

    //Power off house. A través de un relé y el diferencial
    //también permite cambiar el password para activar esta función
    // Ejemplo  /powerOffDifferential?pin=1234    Activa la secuencia para hacer saltar el diferencial
    // Ejemplo  /powerOffDifferential?oldPin=1234&newPin=0000  Cambia el pin por uno nuevo sabiendo el anterior
    server.on("/powerOffDifferential", []() {
        //Si estamos autorizados se ejecuta, si no la función envía el error al navegador
        if ( verifyAccessCookieToken() ){
            //Existe el parámetro pin y concide con el pin que tenemos en el fichero de configuración
            if (server.hasArg("pin")){
                String pin = server.arg("pin");
                if (pin==powerOffPinCode){
                    server.send(200, "text/html", "<p>bye! bye!</p>");
                    //Iniciamos la secuencia para hacer saltar el diferencial
                    // Se intentará un par de veces, derivar a tierra para que salte el diferencial
                    digitalWrite(POWER_OFF_DIFFERENTIAL_PIN, HIGH);
                    timerPowerOffDifferential=millis();
                    powerOffRetry=1;
                    DEBUG_PORT.println(F("POWER OFF DIFFERENTIAL SEQUENCE: 0. Secuencia para hacer saltar diferencial activada. Vamos a encender el rele 2000ms"));
                }else{
                    server.send(200, "text/html", "<p>Nada que hacer.</p>");
                }
            //cambio de pin
            }else if (server.hasArg("oldPin") && server.hasArg("newPin")){
                //si el anterior pin es el que nos han pasado en antiguo continuamos
                if (server.arg("oldPin")==powerOffPinCode){
                    //Si el nuevo pin tiene 4 dígitos vale, sino no.
                    if (server.arg("newPin").length()==4){
                        server.arg("newPin").toCharArray(powerOffPinCode,5);
                        server.send(200, "text/html", "<p>Pin cambiado a " + server.arg("newPin") + "</p>");
                        grabarParametrosFileSystem();
                    }else{
                        server.send(200, "text/html", "<p>La longitud del nuevo pin tiene que ser de 4 números</p>");
                    }

                }else{
                    //El anterior pin no coincide por lo tanto no continuamos
                    server.send(200, "text/html", "<p>Código pin pasado en par&aacute;metro oldPin no concuerda con el actual del dispositivo</p>");
                }
            }else{
                server.send(200, "text/html", "<p>Necesito los parámetros oldPin y newPin. Ejemplo: /powerOffDifferential?oldPin=1234&newPin=9999</p>");
            }
        }

    });

    server.on("/mqttIntervalData", []() {
         //Si estamos autorizados se ejecuta, si no la función envía el error al navegador
        if ( verifyAccessCookieToken() ){
            int updt;
            //miramos si existe el parámetro
            if (server.hasArg("setUpdt")) { 
                //Miramos que tenga un valor adecuado entre 3600 
                if(server.arg("setUpdt").toInt()>0 && server.arg("setUpdt").toInt() < 3601 ){
                    updt = server.arg("setUpdt").toInt();
                    server.sendHeader("access-control-allow-origin", "*");
                    server.send(200, "text/html", String("<p>Mqtt send data every: ") + String(updt) + String(" seconds</p>"));
                    mqtt.setTimeEveryPublishMillis(updt*1000); //establecemos el valor y lo pasamos a milisegundos a la librería mqtt
                    mqttSendInterval=updt*1000; //Guardamos el valor en la variable que guarda el parámetro en cuestión
                    grabarParametrosFileSystem(); //Grabamos los parámetros
                }else{
                    server.send(200, "text/html", "Mqtt interval data send value setting in setUpdt is incorrect. Min 1 second and max 3600!"); 
                }
            }else{
                //es lo mismo que coger mqttSendInterval, pero es mejor de la clase mqtt para saber el valor establecido
                server.send(200, "text/html", "Mqtt interval data send value is: " +String(mqtt.getTimeEveryPublishMillis()/1000) + String(" seconds."));
            }
        }
    });

    server.on("/", []() {
        //Si estamos autorizados se ejecuta, si no la función envía el error al navegador
        if ( verifyAccessCookieToken() ){
            //--------------Read_file_and_send-----------//
            if (LittleFS.exists("/cntrl.html")) {              // If the file exists
                server.sendHeader("Content-Encoding", "gzip");         // gziped header
                File file = LittleFS.open("/cntrl.html", "r");     // Open it
                server.streamFile(file, "text/html");    // And send it to the client
                file.close();                                          // Then close the file again
            }
            else {
                server.send(200, "text/html", "LttleFS: Error! can't open file, no such file");  // error
            }
            //-------------Read_file_and_send_finished-------//
        }
    });

    //url secreta para almacenar la cookie con el api_token en el navegador
    server.on("/me", []() { 
        String auxCookie="sessionToken=" + String(api_token) + "; Expires=Wed, 05 Jun 2069 10:18:14 GMT";
        server.sendHeader("Set-Cookie",auxCookie);
        //Queda algo así
        //server.sendHeader("Set-Cookie", "sessionToken=111v6566v3c363498y6g3qz; Expires=Wed, 05 Jun 2069 10:18:14 GMT");
        server.send(200, "text/html", "OK, authorized!");  // main page
    });
    

    //File Server
    server.onNotFound([]() {                              // If the client requests any URI
        //Si la encontramos en el filesystem
        if (LittleFS.exists(server.uri()))
        {
            //Si estamos autorizados se ejecuta, si no la función envía el error al navegador
            if ( verifyAccessCookieToken() ){

                String ContentType = "text/plain";
                if (server.uri().endsWith(".htm")) {
                    ContentType = "text/html";
                    server.sendHeader("Content-Encoding", "gzip");  // gziped header if html
                }
                else if (server.uri().endsWith(".html")) {
                    ContentType = "text/html";
                    server.sendHeader("Content-Encoding", "gzip");  // gziped header if html
                }
                else if (server.uri().endsWith(".css"))  ContentType = "text/css";
                else if (server.uri().endsWith(".js"))   ContentType = "application/javascript";
                else if (server.uri().endsWith(".png"))  ContentType = "image/png";
                else if (server.uri().endsWith(".gif"))  ContentType = "image/gif";
                else if (server.uri().endsWith(".jpg"))  ContentType = "image/jpeg";
                else if (server.uri().endsWith(".ico"))  ContentType = "image/x-icon";
                else if (server.uri().endsWith(".xml"))  ContentType = "text/xml";
                else if (server.uri().endsWith(".pdf"))  ContentType = "application/x-pdf";
                else if (server.uri().endsWith(".zip"))  ContentType = "application/x-zip";
                else if (server.uri().endsWith(".gz"))   ContentType = "application/x-gzip";


                File file = LittleFS.open(server.uri(), "r");     // Open it
                server.streamFile(file, ContentType);    // And send it to the client
                file.close();                                          // Then close the file again
            }
        }else{
            server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
        }

    });
    //========================FileServer======================================================//
    //Info Page of ESP8266
    server.on("/info", []() {
        //Si estamos autorizados se ejecuta, si no la función envía el error al navegador
        if ( verifyAccessCookieToken() ){
        
            String info = F("<!DOCTYPE html><html><head><style>body{background-color:#000;font-family:Arial;Color:#fff}</style><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><meta charset=\"utf-8\">");
            info += F("<meta name=\"theme-color\" content=\"#042B3C\"><style>hr{border:0;background-color:#570633;height:5px}table{font-family:arial,sans-serif;border-collapse:collapse;width:100%}th{background-color:#00473d;border:1px solid #006356;text-align:left;padding:8px}td{border:1px solid #004763;text-align:left;padding:8px}tr:nth-child(even){background-color:#0a0307}tr:nth-child(odd){background-color:#003e57}a,a:active,a:visited{color:#fff;text-decoration:none}a:hover{color:#ef0;text-decoration:none}</style></head><body>");

            info += F("<table>\n");

            info += F("<tr>\n");
            info += F("<td>\n");
            info += F("The last reset reason:");
            info += F("</td>\n");
            info += F("<td>\n");
            info += ESP.getResetReason();
            info += F("</td>\n");
            info += F("</tr>\n");

            info += F("<tr>\n");
            info += F("<td>\n");
            info += F("Core version:");
            info += F("</td>\n");
            info += F("<td>\n");
            info += ESP.getCoreVersion();
            info += F("</td>\n");
            info += F("</tr>\n");

            info += F("<tr>\n");
            info += F("<td>\n");
            info += F("SDK version:");
            info += F("</td>\n");
            info += F("<td>\n");
            info += ESP.getSdkVersion();
            info += F("</td>\n");
            info += F("</tr>\n");

            info += F("<tr>\n");
            info += F("<td>\n");
            info += F("Frequency:");
            info += F("</td>\n");
            info += F("<td>\n");
            info += ESP.getCpuFreqMHz();
            info += F(" MHz");
            info += F("</td>\n");
            info += F("</tr>\n");

            info += F("<tr>\n");
            info += F("<td>\n");
            info += F("Sketch size:");
            info += F("</td>\n");
            info += F("<td>\n");
            info += ESP.getSketchSize();
            info += F("</td>\n");
            info += F("</tr>\n");

            info += F("<tr>\n");
            info += F("<td>\n");
            info += F("Free sketch space:");
            info += F("</td>\n");
            info += F("<td>\n");
            info += ESP.getFreeSketchSpace();
            info += F("</td>\n");
            info += F("</tr>\n");

            info += F("<tr>\n");
            info += F("<td>\n");
            info += F("Flash chip size (as seen by the SDK):");
            info += F("</td>\n");
            info += F("<td>\n");
            info += ESP.getFlashChipSize();
            info += F("</td>\n");
            info += F("</tr>\n");

            info += F("<tr>\n");
            info += F("<td>\n");
            info += F("Flash chip size (based on the flash chip ID):");
            info += F("</td>\n");
            info += F("<td>\n");
            info += ESP.getFlashChipRealSize();
            info += F("</td>\n");
            info += F("</tr>\n");

            info += F("<tr>\n");
            info += F("<td>\n");
            info += F("Free Heap Size:");
            info += F("</td>\n");
            info += F("<td>\n");
            info += ESP.getFreeHeap();
            info += F("</td>\n");
            info += F("</tr>\n");


            info += F("</table>");
            info += F("</body>\n");
            info += F("</html>");

            server.send(200, "text/html", info);  // main page
        }
    });

    //============restart========================//
    server.on("/restart", []() {
        //Si estamos autorizados se ejecuta, si no la función envía el error al navegador
        if ( verifyAccessCookieToken() ){
            server.send(200, "text/html", "Ok, restarting system now...");
            delay(800);
            ESP.restart();
        }
    });
    //============restart========================//

    //==================================MainData=======================//
    server.on("/smartData", []() {               // jqury will get data from here
        //Si estamos autorizados se ejecuta, si no la función envía el error al navegador
        if ( verifyAccessCookieToken() ){
            server.send(200, "text/html", smartData());
            digitalWrite(indicPinBLUE, LOW); // indication
            delayMicroseconds(200);
            digitalWrite(indicPinBLUE, HIGH); // indication
        }
    });
  
    //JSON
    server.on("/Feed_JSON", []() {
        //Si estamos autorizados se ejecuta, si no la función envía el error al navegador
        if ( verifyAccessCookieToken() ){
            server.send(200, "application/json", jsonFeedGet());
        }
    });

  //Server ds18b20
    server.on("/OneWireServer", []() { //  authorzation
        //Si estamos autorizados se ejecuta, si no la función envía el error al navegador
        if ( verifyAccessCookieToken() ){
            int numberOfDevices = DS18B20.getDeviceCount();
        
            int autoUpdt = 60;

            if(server.arg("setUpdt").toInt()!=0){
                autoUpdt = server.arg("setUpdt").toInt();
            }
        
            String message = F("<!DOCTYPE html><html><head><style>body{background-color:#000;font-family:Arial;Color:#fff}</style><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><meta charset=\"utf-8\"><meta name=\"theme-color\" content=\"#042B3C\">");
            message += F("<meta http-equiv='refresh' content='"); //auto apdate web page
            message += String(autoUpdt);
            message += F("'/>"); 
            message += F("<style>hr{border:0;background-color:#570633;height:5px}table{font-family:arial,sans-serif;border-collapse:collapse;width:100%}th{background-color:#00473d;border:1px solid #006356;text-align:left;padding:8px}td{border:1px solid #004763;text-align:left;padding:8px}tr:nth-child(even){background-color:#0a0307}tr:nth-child(odd){background-color:#003e57}a,a:active,a:visited{color:#fff;text-decoration:none}a:hover{color:#ef0;text-decoration:none}</style></head><body>");
            message += F("<font color=\"#ffff00\"> Devices found: ");
            message += numberOfDevices;
            message += F("</font>");
            message += F(" | ");
            message += F("<font color=\"#32ff00\"> Updates every: ");
            message += String(autoUpdt);
            message += " Sec.";
            message += F("</font>");
            message += F("<hr>");

            if (numberOfDevices>0)
            {
            

                message += F("<table>\n");
                message += F("<tr>\n");
                message += F("<th>Device #</th>\n");
                message += F("<th>Address</th>\n");
                message += F("<th>Temp</th>\n");
                message += F("</tr>\n");

                // Loop through each device
                for(int i=0;i<numberOfDevices; i++)
                {
                    // Search the wire for address
                    if( DS18B20.getAddress(devAddr[i], i) ){

                        message += F("<tr>\n");
                        message += F("<td>\n");
                        message += String(i+1, DEC);
                        message += F("</td>\n");

                            
                        message += F("<td>\n");
                        message += GetAddressToString(devAddr[i]);
                        message += F("</td>\n");


                        message += F("<td>\n");
                        message += String(DS18B20.getTempC(devAddr[i])); //Measuring temperature in Celsius
                        message += F(" °C");
                        message += F("</td>\n");
                    }
                    else{
                        message += F("<tr>\n");
                        message += F("<td>\n");
                        message += String(i, DEC);
                        message += F("</td>\n");

                            
                        message += F("<td>\n");
                        message += "Ghost device";
                        message += F("</td>\n");


                        message += F("<td>\n");
                        message += "Ghost device";
                        message += F("</td>\n");
                    }
                }


                message += F("</tr>\n");
                message += F("</table>");
                message += F("</body>\n");
                message += F("</html>");

            
            }
            server.send(200, "text/html", message );
        }
    });
    

} //final de la función void serveHtml()

String jsonFeedGet() {
  
    StaticJsonDocument<1024> root;

    root["AC_Voltage"] = AC_Voltage;
    root["AC_Current"] = AC_Current;
    root["Power"] = Power;
    root["Frequency"] = Frequency;
    root["PowerFactor"] = PowerFactor;
    root["Energy"] = Energy;
    root["PublicAccess"] =  PublicAccess;
    root["loopsPerSec"] =  loopsPerSec;
    root["FreeHeap"] = ESP.getFreeHeap();
    //Envíamos todos los sensores de temperatura disponibles
    for (int i=0;i<numberOfDevices;i++)
        root["temp"+String(i+1)] = DS18B20.getTempCByIndex(i);
    
    time_t TimeNow = time(nullptr);

    root["UnixTimeStamp"] = String(TimeNow);
    root["TimeNow"] = ctime(&TimeNow);
    root["Millis"] =  millis();      

    String output;
    serializeJson(root, output);

    return output;
}

//=================================//SmartDataFunction//=========================================================//
String smartData() // start of web function, it returns webpage string
{
    //////////////////////////////////////// page start

    // RED #fe3437
    // GREEN #08e18e
    // YELLOW #FFDE00
    // ORANGE #FF6300

    String  web =  "<style>body { background-color: #000000; font-family: Arial; Color: white; }</style>\n";
    web += "<meta charset=\"utf-8\">\n";
    web += "<hr>";
    web += "<li>Voltage: ";
    web += AC_Voltage;
    web += " V";
    web += "<li>Power: ";
    web += Power;
    web += " W";
    web += "<li>Current: ";
    web += AC_Current;
    web += " A";
    web += "</li>";
    web += "<li>Frequency: ";
    web += Frequency;
    web += " Hz";
    web += "</li>";
    web += "<li>Power Factor: ";
    web += String(PowerFactor*100,0) + " %";
    web += "</li>";
    web += "<li>Energy: ";
    web += Energy;
    web += " kWh";
    web += "</li>";

    //Por cada sensor de temperatura que tenemos imprimimos una línea
    for (int i=0;i<numberOfDevices;i++){
        //=========
        web += "<hr>";
        web += "<li>Temperature sensor "+ String(i+1,DEC)+ ": ";
        web += DS18B20.getTempCByIndex(i);
        web += " °C";

        if (!DsReadGood(DS18B20.getTempCByIndex(i))) {
            web += "<font color=\"#fe3437\">";
            web += " [ERROR]";
            web += "</font>";
        }
        web += "</li>";
    }
    
    //================
    web += "<hr>";
    //Wifi signal
    rssi = WiFi.RSSI();
    web += "<li>RSSI: ";
    if (rssi <= -85) {
        web += "<font color=\"#fe3437\">";
        web += rssi;
        web += " dbm";
        web += "</font>";
    }
    else{
        web += rssi;
        web += " dbm";
    }

    web += " | FreeHeap: ";
    web += ESP.getFreeHeap();
    web += " B";

    // === online report start
    // === report if one minute passed
    if (WifiOnlineStateFlag == true) {
        //
        web += " | ";

        
        web += "<font color=\"#08e18e\">";
        web += " [T] ";
        web += "</font>";
        

    }  // === online report end

    web += "</li>";

    //=======================================

    // runtime report
    web += "<hr>";
    web += "<li>UP: ";

    if (days > 0) // days will displayed only if value is greater than zero
    {
        web += days;
        web += " Days, ";
    }

    if (hours > 0)
    {
        web += hours;
        web += " Hrs, ";
    }
    web += mins;
    web += " Min, ";
    web += secs;
    web += " Sec.";



    web += "</li>"; // close line
    web += "<hr>";

    //
    web += "<li>Now: ";          // current time
    time_t TimeNow = time(nullptr);
    web += ctime(&TimeNow); // sec to date, time
    web += "</li>";
    web += "<hr>";

    //

    web += "<li>";
    web += "MS: ";
    web += millis();
    web += " | LPS/s: ";
    web += loopsPerSec;
    web += "</li>";
    web += "<hr>";


    //=======================================

        if (WifiOnlineStateFlag == false) {

        web += "<font color=\"#fe3437\">";
        web += " [ ! ] [ OFFLINE CONTROL MODE! ]";
        web += "</font>";
        web += "<hr>";
    }

    if (PublicAccess == true) {
        web += "<font color=\"#fe3437\">";
        web += " [ ! ]  [ PUBLIC ACCESS ENABLED ! ]";
        web += "</font>";
        web += "<hr>";
    }

  return (web);

} //end of web function
//===========================================SmartDataFunction//=====================================================//
