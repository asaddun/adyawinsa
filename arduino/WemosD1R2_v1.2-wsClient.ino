/**
 * New Script for WeMos D1 R2 with ESP 8266 
 * Based on MC_Arduino v3.1.1 -- LAST USED VERSION : APIK@STMI 2 JUNI 2021
 * Capability:
 * A. Capture Data:
 *    - Temperatur pin INPUT D5 -- MAX66675/DS8126
 *    - Clamp Status pin INPUT D6
 *    - Inject Status pin INPUT D7
 * B. WebServer
 *    - To monitor Status Board
 *    - Last data
 * C. REST  
 *    - return data to SERVER
 * D. OTA (Over the air) - WIFI update    
 *    - update software/firmware from network
 * E. WIFI MANAGER   
 *    - setting to ACCESS POINT at first time running
 *    
 * @Author: Abraham Sulaeman- 19 Mei 2022
 */
 
void(* resetFunc) (void) = 0; //declare reset function @ address 0

char buff[64];
char versi[6]="1.0.0"; // System Version
String versionNum="1.0.0"; // System Version
char mcid[8]="1000077"; // A_Asset_ID or Machine ID API 78 

#define  pinTemp D5           //Get Temp Data
#define  pinClam D6           //Clamp Process
#define  pinInject D7         //Injection Process
#define  pinHeater D8         //Heater ON

#include <ESP8266WiFi.h>
//#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <WebSocketsClient.h>
#include <Hash.h>
#include <ArduinoJson.h>

//ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);
WebSocketsClient webSocket;

const char *ssid     = "API-HOTSPOT";
const char *password = "nevergiveup";

// process state
int stateClamp = 0;         // clam process started
int stateInject = 0;         // inject process started 
unsigned long lastClamp;
unsigned long lastInject;

int laststateInject = 0,laststateClamp = 0;     // previous state of the button
boolean semiAuto=false; // True: Single Sensor for Clam and Injection

unsigned long timenow;
unsigned long lastTime;
unsigned long cycleTime;
unsigned long lastActivity;
unsigned long lastUpdated;
unsigned long maxCycleTime = 300 ; //maxmimum normal Cycle Time

//char response[64]
int heaterState = 0;     // previous state of the button
int pumpState = 0;     // previous state of the button
char heaterON[2]="0"; // Heater State
char pumpON[2]="0"; // Pump State
int timerSensor=0;  // sensor update per 2 detik
int numct, staInj, staCla, product;
String value, JSON_Data;
boolean sendws = false, wifiConnected = true, sendData = false, connected = false;

int helpButtonState = 0,helpButtonLastState = 0,helpStatus=0;
boolean helpNotification=false;
char helpON[2]="0"; // Help State
int helpBlink=0; 
char response[64];
int sensorUpdated=0;


char html_template[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
   <head>
      <meta charset="utf-8">
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <title>Socket Example</title>
      <script>
        var socket = new WebSocket("ws:/" + "/192.168.1.132:7000");
        socket.onmessage  = function(myFunction) {  
                              var full_data = myFunction.data;
                              console.log(full_data);
                              var data = JSON.parse(full_data);
                              id_data = data.id;
                              cla_data = data.cla;
                              inj_data = data.inj;
                              cyc_data = data.cyc;
                              pro_data = data.pro;
                              if(cla_data == 0){
                                cla_data = "OFF";
                              }else{
                                cla_data = "ON";
                              }
                              if(inj_data == 0){
                                inj_data = "OFF";
                              }else{
                                inj_data = "ON";
                              }
                              document.getElementById("id_value").innerHTML = id_data;
                              document.getElementById("cla_value").innerHTML = cla_data;
                              document.getElementById("inj_value").innerHTML = inj_data;
                              document.getElementById("cyc_value").innerHTML = cyc_data;
                              document.getElementById("pro_value").innerHTML = pro_data;
                            };
      </script>
   </head>
   <body>
      <p>id:</p><p id="id_value"></p>
      <p>Clamp:</p><p id="cla_value">OFF</p>
      <p>Inject:</p><p id="inj_value">OFF</p>
      <p>Cycle:</p><p id="cyc_value"></p>
   </body>
</html>
)=====";


void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[WSc] Disconnected!\n");
            connected = false;
            break;
        case WStype_CONNECTED: {
            Serial.printf("[WSc] Connected to url: %s\n", payload);
            connected = true;
 
            // send message to server when Connected
            Serial.println("[WSc] SENT: Connected");
            //webSocket.sendTXT("Connected");
        }
            break;
        case WStype_TEXT:
            Serial.printf("[WSc] RESPONSE: %s\n", payload);
            break;
        case WStype_BIN:
            Serial.printf("[WSc] get binary length: %u\n", length);
            hexdump(payload, length);
            break;
                case WStype_PING:
                        // pong will be send automatically
                        Serial.printf("[WSc] get ping\n");
                        break;
                case WStype_PONG:
                        // answer to a ping we send
                        Serial.printf("[WSc] get pong\n");
                        break;
    }
 
}

void setup() {
  Serial.begin(115200);

  for(uint8_t t = 4; t > 0; t--) {
        Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
    }
    
  //setup Wifi Port
  //WiFiMulti.addAP("API-HOTSPOT", "nevergiveup");
  WiFi.begin(ssid, password);
  delay(2000);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //Serial.println(WiFi.localIP());
  server.on("/", handleMain);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.print("Local IP: "); Serial.println(WiFi.localIP());
  // server address, port and URL
  //webSocket.begin("echo.websocket.org", 80, "/");

  webSocket.begin("192.168.1.139", 7000, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  
  delay(500);
  String versiSW = "ARDUINO: setup rest v";
  versiSW+=versionNum;
  Serial.println(versiSW);
  Serial.println(F("ARDUINO: system started"));

  //setup PIN
  pinMode(pinClam, INPUT);
  pinMode(pinInject, INPUT);
  pinMode(pinTemp, INPUT);
  pinMode(pinHeater, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  //By Pass Clam using  Jumper on PIN Clam
  //Semi Auto Machine : By Pass piClam (always ON) used for New Machine (HAITIAN) 
  stateClamp = digitalRead(pinClam); 
  semiAuto=false;
  if (stateClamp==HIGH){
    semiAuto=true;
  }
  // Init First Run
  lastInject = millis(); 
  lastClamp = lastInject; 
}

void monitorCycleTime(){
  // this is dependent on machine input type. Semi Auto=true for Old JSW machine. 
  if (semiAuto){
    stateClamp = HIGH;
  } else{
    stateClamp = digitalRead(pinClam); //Check Machine Clamp Status (for WOOJIN, HAITIAN)
  }  

  //Read input status from HEATER and PUMP
  heaterState = digitalRead(pinHeater); //Check Heater Status (ON/OFF)
  if (heaterState == HIGH) {
      sprintf(heaterON,"%d",1);
  }
  else {
      sprintf(heaterON,"%d",0);
  }

  if (stateClamp != laststateClamp){
    if (stateClamp == HIGH){
      lastClamp = millis(); 
      //Serial.println("State CLAMP=ON");
      staCla = 1;
      Serial.println("1,0");
      sendws = true;
    } 
    else {
      //Serial.println("State CLAMP=OFF");
      staCla = 0;
      Serial.println("0,0");
      sendws = true;
    }
    laststateClamp = stateClamp;
  }
  if (stateClamp == HIGH) {
    stateInject = digitalRead(pinInject);
    if (stateInject != laststateInject){
      if (stateInject == HIGH){
        //Serial.println("State INJECT=ON");
        staInj = 1;
        product += 1;
        Serial.print("1,1,");
        sendws = true;
        timenow=millis();
        cycleTime=(timenow-lastInject)/1000;
        lastInject = timenow; 
        if (cycleTime <= maxCycleTime){
          sendData=true;
        }
        else{
          cycleTime = 0;
          sendData=false;
        }
      } 
      else {
        //Serial.println("State INJECT=OFF");
        staInj = 0;
        Serial.println("1,0");
        sendws = true; 
      }
    }   
    laststateInject=stateInject;   
  }
  
/*
  if (stateInject==HIGH){
    digitalWrite(LED_BUILTIN, HIGH);
  }
  else {
    digitalWrite(LED_BUILTIN, LOW); 
  }
*/
  if(sendData == false){
    numct = cycleTime;
  }
  else if(sendData == true){
    //sprintf(str_ct,"%d",cycleTime);
    numct = cycleTime;
    
    Serial.println(numct);
    sendData=false;
    lastActivity=millis();  
  }
  
}


void handleMain() {
  server.send_P(200, "text/html", html_template ); 
}
void handleNotFound() {
  server.send(404,   "text/html", "<html><body><p>404 Error</p></body></html>" );
}

 
void loop() {
  webSocket.loop();
  webSocket.setReconnectInterval(5000);
  
  server.handleClient();
  
  monitorCycleTime();

  if (sendws == true){
    JSON_Data = "{";
    JSON_Data += "\"id\":";
    JSON_Data += mcid;
    JSON_Data += ",\"cla\":";
    JSON_Data += staCla;
    JSON_Data += ",\"inj\":";
    JSON_Data += staInj;
    JSON_Data += ",\"cyc\":";
    JSON_Data += numct;
    JSON_Data += ",\"pro\":";
    JSON_Data += product;
    JSON_Data += "}";
    Serial.println(JSON_Data);
    webSocket.sendTXT(JSON_Data);
    sendws = false;
  }
}
