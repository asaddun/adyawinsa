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

#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <WebSocketsClient.h>
#include <Hash.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define  pinTemp D5           //Get Temp Data
#define  pinClam D6           //Clamp Process
#define  pinInject D7         //Injection Process
#define  pinHeater D8         //Heater ON

//ESP8266WiFiMulti WiFiMulti;
WiFiManager wifiManager;
ESP8266WebServer server(80);
WebSocketsServer webSocket_server = WebSocketsServer(81);
WebSocketsClient webSocket;
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
char versi[6]="4.0.0"; // System Version
String versionNum="4.0.0"; // System Version
//char mcid[8]="1000078"; // A_Asset_ID or Machine ID API 78

unsigned long timenow;
unsigned long lastTime;
unsigned long cycleTime;
unsigned long lastActivity;
unsigned long lastUpdated;
unsigned long maxCycleTime = 300 ; //maxmimum normal Cycle Time

//char response[64]
int heaterState = 0;     // previous state of the button
int pumpState = 0;     // previous state of the button
char heaterON[2]="1"; // Heater State
char pumpON[2]="1"; // Pump State
int timerSensor=0;  // sensor update per 2 detik
int numct, staInj, staCla, shoot;
unsigned long runtime;
int run_second, run_minute, run_hour, run_day;
int c_day, c_month, c_year;
String JSON_Data, formattedTime, currentDate;
boolean sendws = false, wifiConnected = true, sendData = false;
bool shouldSaveConfig = false;
char deviceId[10], deviceName[50], WSaddress[16], chPort[5];
String strPort;
int WSport;
String ipAddress;


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
        var socket = new WebSocket("ws://" + window.location.hostname + ":81");
        socket.onmessage  = 
        function(event) {  
          var full_data = event.data;
          console.log(full_data);
          var data = JSON.parse(full_data);
          var id_data = data.id;
          var name_data = data.name;
          var cla_data = data.cla;
          var inj_data = data.inj;
          var cyc_data = data.cyc;
          var shoot_data = data.shoot;
          var day_data = data.day;
          var hour_data = data.hour;
          var minute_data = data.minute;
          var date_data = data.date;
          var time_data = data.time;

          if (inj_data == 1){ // take the timestamp when inject
            document.getElementById("date_value").innerHTML = date_data;
            document.getElementById("time_value").innerHTML = time_data;;
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
          document.title = name_data;
          document.getElementById("cla_value").innerHTML = cla_data;
          document.getElementById("inj_value").innerHTML = inj_data;
          document.getElementById("cyc_value").innerHTML = cyc_data;
          document.getElementById("shoot_value").innerHTML = shoot_data;
          document.getElementById("day_value").innerHTML = day_data;
          document.getElementById("hour_value").innerHTML = hour_data;
          document.getElementById("minute_value").innerHTML = minute_data;
        };
      </script>
      <style>
        .box {
          width: 375px;
          height: 250px;
          position: relative;
          border-radius: 10px;
          margin: 10px;
          text-align: center;
          font-size: 18px;
        }
        .cycle {
          width: 200px;
          height: 130px;
          background: green;
          position: absolute;
          border-radius: 10px 0px 0px 0px;
        }
        .cyctext {
          font-weight: bold;
          font-size: 56px;
          text-align: center;
          margin-top: 16px;
        }
        .runtime {
          width: 200px;
          height: 72.5px;
          background: aquamarine;
          position: absolute;
          margin-top: 130px;
        }
        .clamp {
          width: 100px;
          height: 62.5px;
          background: blue;
          position: absolute;
          margin-top: 187px;
          border-radius: 0px 0px 0px 10px;
        }
        .inject {
          width: 100px;
          height: 62.5px;
          background: violet;
          position: absolute;
          margin-top: 187px;
          margin-left: 100px;
        }
        .onoff {
          font-weight: bold;
          font-size: 20px;
          text-align: center;
          margin-top: 5px;
        }
        .shoot {
          width: 175px;
          height: 115px;
          background: yellow;
          position: absolute;
          margin-left: 200px;
          border-radius: 0px 10px 0px 0px;
        }
        .shootext {
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
          margin-left: 200px;
          margin-top: 115px;
        }
        .idbox {
          width: 175px;
          height: 62.5px;
          background: blueviolet;
          position: absolute;
          margin-left: 200px;
          margin-top: 187px;
          border-radius: 0px 0px 10px 0px;
        }
      </style>
   </head>
   <body>
      <div class="box">

        <div class="cycle">
          Last Cycle Duration:<br>
          <div class="cyctext" id="cyc_value"></div>
        </div>
        
        <div class="runtime">
          Runtime:<br>
          <span id="day_value">0</span> day 
          <span id="hour_value">0</span> hour 
          <span id="minute_value">0</span> minute 
        </div>

        <div class="clamp">
          Clamp:<div class="onoff" id="cla_value">OFF</div>
        </div>

        <div class="inject">
          Inject:<div class="onoff" id="inj_value">OFF</div>
        </div>

        <div class="shoot">
          Shoot:<div class="shootext" id="shoot_value"></div>
        </div>
        <div class="time">
          Last Cycle On:<br>
          <span id="date_value"> </span><br>
          <span id="time_value"> </span>
        </div>
        <div class="idbox">
          ID:<br>
          <span id="id_value"></span>
        </div>
      </div>
      <button onclick="location.href='/reset'">Reset</button>
   </body>
</html>
)=====";

char html_reset[] PROGMEM = R"=====(
  <!DOCTYPE html>
  <html>
  <script>
    function reset() {
      var x = document.getElementById("reset").value;
      var text = "";
      if (x == "1234"){
        alert("ESP will reset");
        window.location.pathname = ('/confirm_reset');
      } else {
        alert("Incorrect Password!!");
      }
    }
    function showPass() {
      var x = document.getElementById("reset");
      if (x.type === "password") {
        x.type = "text";
      } else {
        x.type = "password";
      }
    }
  </script>
  <body>
    Password to Reset:<br>
    <input id="reset" type="password"><br>
    <input type="checkbox" onclick="showPass()">Show Password
    <br><br>
    <button onclick="reset()">Reset</button>
  </body>
  </html>
)=====";

void webSocketEvent_server(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket_server.remoteIP(num);
                Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
                // sendws = true;
        
        // send message to client
        //webSocket_server.sendTXT(num, "Connected");
            }
            break;
        case WStype_TEXT:
            Serial.printf("[%u] get Text: %s\n", num, payload);

            // send message to client
            // webSocket.sendTXT(num, "message here");

            // send data to all connected clients
            // webSocket.broadcastTXT("message here");
            break;
        case WStype_BIN:
            Serial.printf("[%u] get binary length: %u\n", num, length);
            hexdump(payload, length);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
            break;
    }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[WSc] Disconnected!\n");
            break;
        case WStype_CONNECTED: {
            Serial.printf("[WSc] Connected to url: %s\n", payload);
 
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
void handleReset() {
  server.send_P(200, "text/html", html_reset );
}
void ConfirmReset() {
  server.send_P(200, "text/html", "<html><body><p>ESP has been reset</p></body></html>" );
  delay(3000);
  wifiManager.resetSettings();
  ESP.restart();
}
void handleRestart() {
  server.send_P(200, "text/html", "<html><body><p>ESP has been restart</p></body></html>" );
  ESP.restart();
}
void handleNotFound() {
  server.send(404,   "text/html", "<html><body><p>404 Error</p></body></html>" );
}

void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() {
  Serial.begin(115200);

  for(uint8_t t = 4; t > 0; t--) {
        Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
  }

  Serial.println("Mounting FS...");    //read configuration from FS json

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          strcpy(deviceId, json["deviceId"]);
          strcpy(deviceName, json["deviceName"]);
          strcpy(WSaddress, json["WSaddress"]);
          strcpy(chPort, json["Port"]);
        } else {
          Serial.println("failed to load json config");
        }
      }
//      configFile.close();
//      SPIFFS.remove("/config.json");
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
  
  // Wifi Manager Setup
  // id/name placeholder/prompt default length
  WiFiManagerParameter customDeviceId("deviceId", "Device Id", deviceId, 10);
  WiFiManagerParameter customDeviceName("deviceName", "Device Name", deviceName, 50);
  WiFiManagerParameter customWSaddress("WSaddress", "Websocket Address", WSaddress, 16);
  WiFiManagerParameter customPort("Port", "Port (ws//:xxx.xxx.xxx.xxx:'xxxx'/)", chPort, 5);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&customDeviceId);
  wifiManager.addParameter(&customDeviceName);
  wifiManager.addParameter(&customWSaddress);
  wifiManager.addParameter(&customPort);
  //wifiManager.resetSettings();
  // AP esp if can't connect to wifi (each board mac)  
  wifiManager.autoConnect();
  strcpy(deviceId, customDeviceId.getValue());
  strcpy(deviceName, customDeviceName.getValue());
  strcpy(WSaddress, customWSaddress.getValue());
  strcpy(chPort, customPort.getValue());

  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["deviceId"] = deviceId;
    json["deviceName"] = deviceName;
    json["WSaddress"] = WSaddress;
    json["Port"] = chPort;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
  
  Serial.println("\nWiFi Connected..");

  String versiSW = "ARDUINO: setup rest v";
  versiSW+=versionNum;
  Serial.println(versiSW);
  Serial.println(F("ARDUINO: system started"));

  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());

  // Webserver Setup
  server.on("/", handleMain);
  server.on("/reset", handleReset);
  server.on("/confirm_reset", ConfirmReset);
  server.on("/restart", handleRestart);
  server.onNotFound(handleNotFound);
  server.begin();

  // Websocket Server Setup
  Serial.println("[WSs] Starting WebSocket server ");
  webSocket_server.begin();
  webSocket_server.onEvent(webSocketEvent_server);
  
  // Websocket Client Setup
  Serial.print("[WSc] Try connect to WS server ");
  Serial.print(WSaddress);
  Serial.print(":");
  for(int i=0; i<=4; i++){
    strPort += chPort[i];
  }
  WSport = strPort.toInt();
  Serial.println(WSport);
  webSocket.begin(WSaddress, WSport, "/"); // Websocket server address
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
  timeClient.setTimeOffset(25202); // GMT +7 (60(sec) * 60(min) * 7(hour))
  
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
  //upTime = millis();
  //runtime = millis();
  lastInject = millis();
  lastClamp = lastInject;

  ipAddress = WiFi.localIP().toString();
  Serial.println(ipAddress);
  //ipAddress = ipAddress.toString();
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
        shoot += 1;
        // Serial.println("1,1,");
        
        timenow=millis();
        cycleTime=(timenow-lastInject)/1000;
        lastInject = timenow;

        runtime += cycleTime;
        run_second = runtime;
        run_minute = run_second / 60;
        run_hour = run_minute / 60;
        run_day = run_hour / 24;
        run_second %= 60;
        run_minute %= 60;
        run_hour %= 24;

        // NTP get data
        time_t epochTime = timeClient.getEpochTime();
        formattedTime = timeClient.getFormattedTime();
        struct tm *ptm = gmtime ((time_t *)&epochTime);
        c_day = ptm->tm_mday;
        c_month = ptm->tm_mon+1;
        c_year = ptm->tm_year+1900;
        currentDate = String(c_day) + "/" + String(c_month) + "/" + String(c_year);
        
        
        if (cycleTime <= maxCycleTime){
          //sendData = true;
          numct = cycleTime;
          lastActivity=millis();
        }
        else{
          //sendData = false;
          cycleTime = 0;
          numct = cycleTime;
        }
        sendws = true;
        
      }
      else {                      // Inject = OFF
        staInj = 0;
        // Serial.println("1,0");
        sendws = true;
      }
    }
    laststateInject=stateInject;
  }

/*
 * The previous code
 * Put after NTP get data ||
                          \/
                      
          sendws = true;
          if (cycleTime <= maxCycleTime){
            sendData = true;
          }
          else{
            cycleTime = 0;
            sendData = false;
          }
        }
        else {                      // Inject = OFF
          staInj = 0;
          // Serial.println("1,0");
          sendws = true;
        }
      }
      laststateInject=stateInject;
    }
  
    if(sendData == true){
      numct = cycleTime;
      //Serial.println(numct);
      sendData=false;
      lastActivity=millis();
    }
*/
}

void loop() {
  server.handleClient();
  webSocket_server.loop();
  webSocket.setReconnectInterval(5000); // If disconnected from websocket will try reconnect every 5 seconds
  webSocket.loop();
  ArduinoOTA.handle();
  timeClient.update();

  monitorCycleTime();
  //getRuntime();

  // send data to websocket as JSON format
  if (sendws == true){
    JSON_Data = "{";
    JSON_Data += "\"action\":";
    JSON_Data += "\"shoot\"";
    JSON_Data += ",\"id\":";
    JSON_Data += deviceId;
    JSON_Data += ",\"name\":\"";
    JSON_Data += deviceName;
    JSON_Data += "\"";
    JSON_Data += ",\"cla\":";
    JSON_Data += staCla;
    JSON_Data += ",\"inj\":";
    JSON_Data += staInj;
    JSON_Data += ",\"cyc\":";
    JSON_Data += numct;
    JSON_Data += ",\"shoot\":";
    JSON_Data += shoot;
    JSON_Data += ",\"ip\":\"";
    JSON_Data += ipAddress;
    JSON_Data += "\"";
    JSON_Data += ",\"heat\":";
    JSON_Data += heaterON;
    JSON_Data += ",\"pump\":";
    JSON_Data += pumpON;
    JSON_Data += ",\"day\":";
    JSON_Data += run_day;
    JSON_Data += ",\"hour\":";
    JSON_Data += run_hour;
    JSON_Data += ",\"minute\":";
    JSON_Data += run_minute;
    JSON_Data += ",\"date\":\"";
    JSON_Data += currentDate;
    JSON_Data += "\"";
    JSON_Data += ",\"time\":\"";
    JSON_Data += formattedTime;
    JSON_Data += "\"";
    JSON_Data += "}";
    Serial.println(JSON_Data);
    webSocket.sendTXT(JSON_Data);
    webSocket_server.broadcastTXT(JSON_Data);
    sendws = false;
  }
}
