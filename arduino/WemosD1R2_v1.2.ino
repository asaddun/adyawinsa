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

#define  pinTemp D5           //Get Temp Data
#define  pinClam D6           //Clamp Process
#define  pinInject D7         //Injection Process
#define  pinHeater D8         //Heater ON

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsClient.h>
#include <Hash.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

//ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);
WebSocketsClient webSocket;
WiFiManager wifiManager;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// process state
int stateClamp = 0;         // clam process started
int stateInject = 0;         // inject process started 
unsigned long lastClamp;
unsigned long lastInject;

int laststateInject = 0,laststateClamp = 0;     // previous state of the button
boolean semiAuto=false; // True: Single Sensor for Clam and Injection

char buff[64];
char versi[6]="1.0.0"; // System Version
String versionNum="1.0.0"; // System Version
char mcid[8]="1000077"; // A_Asset_ID or Machine ID API 78

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
int numct, staInj, staCla, shut;
int c_day, c_month, c_year;
int c_hour, c_minute, c_second;
String WS_address = "192.168.#.###"; // Websocket server address
String JSON_Data;
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
      <title>Dashboard</title>
      <script>
        var socket = new WebSocket("ws://192.168.#.###:7000");
        socket.onmessage  = 
        function(event) {  
          var full_data = event.data;
          console.log(full_data);
          var data = JSON.parse(full_data);
          var id_data = data.id;

          if(id_data == #######){  // machine id
            var cla_data = data.cla;
            var inj_data = data.inj;
            var cyc_data = data.cyc;
            var shut_data = data.shut;
            var day_data = data.day;
            var mon_data = data.mon;
            var year_data = data.year;
            var hour_data = data.hour;
            var min_data = data.min;
            var sec_data = data.sec;
  
            if (inj_data == 1){ // take the timestamp when inject
              document.getElementById("day_value").innerHTML = day_data;
              document.getElementById("mon_value").innerHTML = mon_data;
              document.getElementById("year_value").innerHTML = year_data;
              document.getElementById("hour_value").innerHTML = hour_data;
              document.getElementById("min_value").innerHTML = min_data;
              document.getElementById("sec_value").innerHTML = sec_data;
            }
  
            if(cla_data == 0){
              cla_data = "OFF";
              cla_data = cla_data.fontcolor("red");
            }else{
              cla_data = "ON";
              cla_data = cla_data.fontcolor("green");
            }
            if(inj_data == 0){
              inj_data = "OFF";
              inj_data = inj_data.fontcolor("red");
            }else{
              inj_data = "ON";
              inj_data = inj_data.fontcolor("green");
            }
  
            document.getElementById("id_value").innerHTML = id_data;
            document.getElementById("cla_value").innerHTML = cla_data;
            document.getElementById("inj_value").innerHTML = inj_data;
            document.getElementById("cyc_value").innerHTML = cyc_data;
            document.getElementById("shut_value").innerHTML = shut_data;
          }
        };
      </script>
      <style>
        .box {
          width: 350px;
          height: 250px;
          position: relative;
          border-radius: 10px;
          margin: 10px;
          text-align: center;
          font-size: 18px;
        }
        .cycle {
          width: 175px;
          height: 187.5px;
          background: green;
          position: absolute;
          border-radius: 10px 0px 0px 0px;
        }
        .cyctext {
          font-weight: bold;
          font-size: 56px;
          text-align: center;
          margin-top: 25%;
        }
        .clamp {
          width: 87.5px;
          height: 62.5px;
          background: blue;
          position: absolute;
          margin-top: 187px;
          border-radius: 0px 0px 0px 10px;
        }
        .inject {
          width: 87.5px;
          height: 62.5px;
          background: violet;
          position: absolute;
          margin-top: 187px;
          margin-left: 25%;
        }
        .onoff {
          font-weight: bold;
          font-size: 20px;
          text-align: center;
          margin-top: 5px;
        }
        .shut {
          width: 175px;
          height: 115px;
          background: yellow;
          position: absolute;
          margin-left: 50%;
          border-radius: 0px 10px 0px 0px;
        }
        .shutext {
          font-weight: bold;
          font-size: 56px;
          text-align: center;
          margin-top: 10px;
        }
        .time {
          width: 175px;
          height: 72.5px;
          background: orange;
          position: absolute;
          margin-left: 50%;
          margin-top: 115px;
        }
        .idbox {
          width: 175px;
          height: 62.5px;
          background: blueviolet;
          position: absolute;
          margin-left: 50%;
          margin-top: 187px;
          border-radius: 0px 0px 10px 0px;
        }
      </style>
   </head>
   <body>
      <div class="box">

        <div class="cycle">
          Last Cycle Duration:<div class="cyctext" id="cyc_value"></div>
        </div>

        <div class="clamp">
          Clamp:<div class="onoff" id="cla_value">OFF</div>
        </div>

        <div class="inject">
          Inject:<div class="onoff" id="inj_value">OFF</div>
        </div>

        <div class="shut">
          Shut:<div class="shutext" id="shut_value"></div>
        </div>
        <div class="time">
          Last Cycle On:<br>
          <span id="day_value">0</span>/<span id="mon_value">0</span>/<span id="year_value">0</span><br>
          <span id="hour_value">0</span>:<span id="min_value">0</span>:<span id="sec_value">0</span>
        </div>
        <div class="idbox">
          ID:<br>
          <span id="id_value"></span>
        </div>

      </div>
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

void handleMain() {
  server.send_P(200, "text/html", html_template ); 
}
void handleNotFound() {
  server.send(404,   "text/html", "<html><body><p>404 Error</p></body></html>" );
}

void setup() {
  Serial.begin(115200);

  for(uint8_t t = 4; t > 0; t--) {
        Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
    }
  
  // Wifi Manager Setup
  // AP esp if can't connect to wifi (each board mac)  
  wifiManager.autoConnect("esp8266-######");
  Serial.println("Connected..");

  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  
  // Webserver Setup
  server.on("/", handleMain);
  server.onNotFound(handleNotFound);
  server.begin();

  // Websocket Setup
  webSocket.begin(WS_address, 7000, "/"); // Websocket server address
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);

  // OTA Setup
  ArduinoOTA.setPassword("1234");
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  // NTP Setup
  timeClient.begin();
  timeClient.setTimeOffset(25195); // GMT +7 ((60(sec) * 60(min) * 7(hour) - 5 seconds (tolerance, always 5 seconds ahead))
  
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
    if (stateClamp == HIGH){      // Clamp = ON
      lastClamp = millis(); 
      staCla = 1;
      sendws = true;
    } 
    else {                        // Clamp = OFF
      staCla = 0;
      sendws = true;
    }
    laststateClamp = stateClamp;
  }
  if (stateClamp == HIGH) {
    stateInject = digitalRead(pinInject);
    if (stateInject != laststateInject){
      if (stateInject == HIGH){   // Inject = ON
        staInj = 1;
        shut += 1;
        Serial.println("1,1,");
        sendws = true;
        timenow=millis();
        cycleTime=(timenow-lastInject)/1000;
        lastInject = timenow; 
        if (cycleTime <= maxCycleTime){
          sendData=true;
        }
        else{
          sendData=false;
        }
      } 
      else {                      // Inject = OFF
        staInj = 0;
        Serial.println("1,0");
        sendws = true; 
      }
    }   
    laststateInject=stateInject;   
  }
  
  if(sendData == true){
    numct = cycleTime;
    Serial.println(numct);

    // NTP get data
    time_t epochTime = timeClient.getEpochTime();
    c_hour = timeClient.getHours();
    c_minute = timeClient.getMinutes();
    c_second = timeClient.getSeconds();
    struct tm *ptm = gmtime ((time_t *)&epochTime);
    c_day = ptm->tm_mday;
    c_month = ptm->tm_mon+1;
    c_year = ptm->tm_year+1900;
    
    sendData=false;
    lastActivity=millis();  
  }
  
}

void loop() {
  ArduinoOTA.handle();
  timeClient.update();
  webSocket.loop();

  // If disconnected from websocket will try reconnect every 5 seconds
  webSocket.setReconnectInterval(5000);
  
  server.handleClient();
  
  monitorCycleTime();

  // send data to websocket as JSON format
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
    JSON_Data += ",\"shut\":";
    JSON_Data += shut;
    
    JSON_Data += ",\"day\":";
    JSON_Data += c_day;
    JSON_Data += ",\"mon\":";
    JSON_Data += c_month;
    JSON_Data += ",\"year\":";
    JSON_Data += c_year;
    
    JSON_Data += ",\"hour\":";
    JSON_Data += c_hour;
    JSON_Data += ",\"min\":";
    JSON_Data += c_minute;
    JSON_Data += ",\"sec\":";
    JSON_Data += c_second;
    
    JSON_Data += "}";
    Serial.println(JSON_Data);
    webSocket.sendTXT(JSON_Data);
    sendws = false;
  }
}