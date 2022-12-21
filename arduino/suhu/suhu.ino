#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <WebSocketsClient.h>
#include <Hash.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define ONE_WIRE_BUS D5  //D5 pin

WiFiManager wifiManager;
ESP8266WebServer server(80);
WebSocketsServer webSocket_server = WebSocketsServer(81);
WebSocketsClient webSocket;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

int run_second, run_minute, run_hour, run_day;
String JSON_Data;
boolean sendws = false, wifiConnected = true;
bool shouldSaveConfig = false;
char deviceId[10], deviceName[50], WSaddress[16], chPort[5], loc[50];
String strPort;
int WSport;
String ipAddress;

char html_template[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
   <head>
      <meta charset="utf-8">
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <title>Dashboard</title>

      <style>
        .center{
        display: flex;
        justify-content: center;
        }

        .gauge {
        width: 100%;
        max-width: 250px;
        font-family: "Roboto", sans-serif;
        font-size: 32px;
        color: #004033;
        }

        .gauge_body {
        width: 100%;
        height: 0;
        padding-bottom: 50%;
        background: #b4c0be;
        position: relative;
        border-top-left-radius: 100% 200%;
        border-top-right-radius: 100% 200%;
        overflow: hidden;
        }

        .gauge_fill {
        position: absolute;
        top: 100%;
        left: 0;
        width: inherit;
        height: 100%;
        background: #009578;
        transform: rotate(0deg);
        transform-origin: center top;
        transition: transform 1s ease-out;
        }

        .gauge_cover {
        width: 75%;
        height: 150%;
        background: #ffffff;
        border-radius: 50%;
        position: absolute;
        top: 25%;
        left: 50%;
        transform: translateX(-50%);

        /* Text */
        display: flex;
        align-items: center;
        justify-content: center;
        padding-bottom: 25%;
        box-sizing: border-box;
        }
      </style> 
  </head>
  <body>
      <h2 style="text-align: center;">Temperature<br><span id="loc_value"></span></h2><br>
      <div class="center">
        <div class="gauge" >
          <div class="gauge_body">
              <div class="gauge_fill"></div>
              <div class="gauge_cover"></div>
          </div>
        </div>
      </div>
      <h3 style="text-align: center;"><span id="date_value"></span><br><span id="time_value"></span></h3>
      
      <script>
        var socket = new WebSocket("ws://" + window.location.hostname + ":81");
        socket.onmessage  = 
        function(event) {  
          var full_data = event.data;
          console.log(full_data);
          var data = JSON.parse(full_data);
          var temp_data = data.temp;
          var loc_data = data.loc;
          var name_data = data.name;
          var date_data = data.date;
          var time_data = data.time;
          const gaugeElement = document.querySelector(".gauge");

          function setGaugeValue(gauge, value) {
            if (value < 0 || value > 100) {
                return;
            }

            var degree = value * 3.6;
            
            if (degree < 181) {
              gauge.querySelector(".gauge_fill").style.transform = `rotate(${degree}deg)`;
              
              if (degree <= 36){ 0-10
                  gauge.querySelector(".gauge_fill").style.background = `rgb(22, 109, 222)`;
              }
              if (degree > 36){
                  gauge.querySelector(".gauge_fill").style.background = `rgb(22, 222, 199)`;
              }
              if (degree > 72){
                  gauge.querySelector(".gauge_fill").style.background = `rgb(22, 222, 32)`;
              }
              if (degree > 108){
                  gauge.querySelector(".gauge_fill").style.background = `rgb(205, 222, 22)`;
              }
              if (degree > 133){
                  gauge.querySelector(".gauge_fill").style.background = `rgb(222, 132, 22)`;
              }
              if (degree > 158){
                  gauge.querySelector(".gauge_fill").style.background = `rgb(222, 69, 22)`;
              }
            } else if (degree > 180) {
                gauge.querySelector(".gauge_fill").style.transform = `rotate(180deg)`;
                gauge.querySelector(".gauge_fill").style.background = `rgb(222, 69, 22)`;
            }
              gauge.querySelector(".gauge_cover").textContent = `${(temp_data).toFixed(1)}°C`;
          }

          document.getElementById("loc_value").innerHTML = loc_data;
          document.getElementById("date_value").innerHTML = date_data;
          document.getElementById("time_value").innerHTML = time_data;
          document.title = name_data;
          setGaugeValue(gaugeElement, temp_data);
          
        };
      </script>
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
          strcpy(loc, json["Loc"]);
        } else {
          Serial.println("failed to load json config");
        }
      }
      // configFile.close();
      // SPIFFS.remove("/config.json");
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
  
  // Wifi Manager Setup
  // id/name placeholder/prompt default length
  WiFiManagerParameter customDeviceId("deviceId", "Device Id", deviceId, 10);
  WiFiManagerParameter customDeviceName("deviceName", "Device Name", deviceName, 50);
  WiFiManagerParameter customLoc("loc", "Location", loc, 50);
  WiFiManagerParameter customWSaddress("WSaddress", "Websocket Address", WSaddress, 16);
  WiFiManagerParameter customPort("Port", "Port (ws//:xxx.xxx.xxx.xxx:'xxxx'/)", chPort, 5);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&customDeviceId);
  wifiManager.addParameter(&customDeviceName);
  wifiManager.addParameter(&customLoc);
  wifiManager.addParameter(&customWSaddress);
  wifiManager.addParameter(&customPort);
  // wifiManager.resetSettings();
  // AP esp if can't connect to wifi (each board mac)  
  wifiManager.autoConnect();
  strcpy(deviceId, customDeviceId.getValue());
  strcpy(deviceName, customDeviceName.getValue());
  strcpy(loc, customLoc.getValue());
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
    json["Loc"] = loc;

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

  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());

  // Webserver Setup
  server.on("/", handleMain);
  server.on("/reset", handleReset);
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
  
  sensors.begin();

  ipAddress = WiFi.localIP().toString();
  Serial.println(ipAddress);
}

void loop() {
  server.handleClient();
  webSocket_server.loop();
  webSocket.setReconnectInterval(5000); // If disconnected from websocket will try reconnect every 5 seconds
  webSocket.loop();
  ArduinoOTA.handle();
  timeClient.update();
  sensors.requestTemperatures();
  float data = sensors.getTempCByIndex(0);

  // NTP get data
  time_t epochTime = timeClient.getEpochTime();
  String formattedTime = timeClient.getFormattedTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  int c_day = ptm->tm_mday;
  int c_month = ptm->tm_mon+1;
  int c_year = ptm->tm_year+1900;
  String currentDate = String(c_day) + "/" + String(c_month) + "/" + String(c_year);

  String JSON_Data;
  JSON_Data = "{";
  JSON_Data += "\"action\":";
  JSON_Data += "\"temp\"";
  JSON_Data += ",\"id\":";
  JSON_Data += deviceId;
  JSON_Data += ",\"name\":\"";
  JSON_Data += deviceName;
  JSON_Data += "\"";
  JSON_Data += ",\"temp\":";
  JSON_Data += data;
  JSON_Data += ",\"loc\":\"";
  JSON_Data += loc;
  JSON_Data += "\"";
  JSON_Data += ",\"ip\":\"";
  JSON_Data += ipAddress;
  JSON_Data += "\"";
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
}
