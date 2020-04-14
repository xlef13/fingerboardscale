#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <DNSServer.h>
#include <FS.h>
#include "HX711.h"


#define LOADCELL_OFFSET 50682624
#define LOADCELL_DIVIDER 5895655

// scale
HX711 scale;
const uint8_t DOUT = D1;
const uint8_t CLK = D2;

//  Wifi
const byte DNS_PORT = 53;
IPAddress staticIP(192,168,178,125);
IPAddress gateway(192,168,178,1);
IPAddress subnet(255,255,255,0);
const char* SSID = "Fingerficker";
const char* PSK = "";
AsyncWebServer server(80);
DNSServer dnsServer;

// Websocket
WebSocketsServer webSocket(49160);
uint8_t currentnum;
bool websocketconnected = false;
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      currentnum = 0;
      websocketconnected = false;
      server.begin();
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        currentnum = num;
        websocketconnected = true;
        server.end();
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT: {
      if(num == currentnum){
        // switch case for different instructions
      }
    }
    break;
  }
}
//SETUP
void setup_wifi() {
  Serial.print("Setting soft-AP ... ");
  WiFi.softAPConfig(staticIP, gateway, subnet);
  boolean result = WiFi.softAP(SSID, PSK);

  if(result == true)
  {
    Serial.println("Ready " + WiFi.softAPIP());
  }
  else
  {
    Serial.println("Failed!");
    return;
  }

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
//  WiFi.mode(WIFI_STA);
//  WiFi.config(staticIP, gateway, subnet);
//  WiFi.begin(SSID, PSK);
//  while (WiFi.status() != WL_CONNECTED) {
//      delay(500);
//      Serial.print(".");
//  }
//  Serial.println("");
//  Serial.println("WiFi connected");
//  Serial.println("IP address: ");
//  Serial.println(WiFi.localIP());
}

void setup() {    
  //  SPIFFS
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // setup serial
  Serial.begin(9600);  
   
  // setup wifi & mqtt
  setup_wifi();  

  // setup scale
  scale.begin(DOUT, CLK);
  scale.set_scale();
  scale.tare();

  // Servercallbacks
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("client connected ");
    request->send(SPIFFS, "/index.html");
  }); 
  server.begin();
  
  //  socket callbacks
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

//LOOP
void loop()
{
  webSocket.loop();
  webSocket.sendTXT(currentnum,String(scale.get_units(1)).c_str());
}
