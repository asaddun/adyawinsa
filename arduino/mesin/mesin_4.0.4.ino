/*
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
 * Update: Muhammad Asad- 28 Agustus 2023 v4.0.3
 */

void (*resetFunc)(void) = 0;  // declare reset function @ address 0

#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <WebSocketsClient.h>
#include <Hash.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include <ArduinoOTA.h>

#define pinClamp D6   // Clamp Process
#define pinInject D7  // Injection Process

WiFiManager wifiManager;
ESP8266WebServer server(80);
WebSocketsServer webSocket_server = WebSocketsServer(81);
WebSocketsClient webSocket;

// DECLARE VARIABLE
const char* versionUrl = "https://apik.adyawinsa.com/smsd/api/update-arduino/version.txt";
const char* firmwareUrl = "https://apik.adyawinsa.com/smsd/api/update-arduino/firmware.bin";
String versionNum = "4.0.4";                  // System Version
int laststateInject = 0, laststateClamp = 0;  // previous state of the button
unsigned long timenow;
unsigned long cycleTime;
int maxCycleTime = 300;  // maxmimum normal Cycle Time
int stateClamp = 0;      // clamp process started
int stateInject = 0;     // inject process started
unsigned long lastClamp;
unsigned long lastInject;
int heaterState = 0;  // previous state of the button
int pumpState = 0;    // previous state of the button
int staCla, staInj = 0, numct, shoot, numdt;
unsigned long runtime;
int run_second, run_minute, run_hour, run_day;
int c_day, c_month, c_year;
String JSON_Data, formattedTime, currentDate;
bool sendws = false, wifiConnected = true, sendData = false;
bool shouldSaveConfig = false;
char deviceId[10], deviceName[50], WSaddress[16], chPort[5];
String ipAddress, action = "shoot";
bool wiFiConnected = true, websocketConnected = false;
unsigned long wifiMillis, wifiDownSecond, wifiDownMinute;
unsigned long unixTime, startTime, startIdle;
struct tm timeinfo;
const int timeZone = 7 * 3600;
unsigned long previousCheckSeconds;
bool is_update = false;
int port = 1880;

const char rootCACertificate[] PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
MIIGEzCCA/ugAwIBAgIQfVtRJrR2uhHbdBYLvFMNpzANBgkqhkiG9w0BAQwFADCB
iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl
cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV
BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTgx
MTAyMDAwMDAwWhcNMzAxMjMxMjM1OTU5WjCBjzELMAkGA1UEBhMCR0IxGzAZBgNV
BAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4GA1UEBxMHU2FsZm9yZDEYMBYGA1UE
ChMPU2VjdGlnbyBMaW1pdGVkMTcwNQYDVQQDEy5TZWN0aWdvIFJTQSBEb21haW4g
VmFsaWRhdGlvbiBTZWN1cmUgU2VydmVyIENBMIIBIjANBgkqhkiG9w0BAQEFAAOC
AQ8AMIIBCgKCAQEA1nMz1tc8INAA0hdFuNY+B6I/x0HuMjDJsGz99J/LEpgPLT+N
TQEMgg8Xf2Iu6bhIefsWg06t1zIlk7cHv7lQP6lMw0Aq6Tn/2YHKHxYyQdqAJrkj
eocgHuP/IJo8lURvh3UGkEC0MpMWCRAIIz7S3YcPb11RFGoKacVPAXJpz9OTTG0E
oKMbgn6xmrntxZ7FN3ifmgg0+1YuWMQJDgZkW7w33PGfKGioVrCSo1yfu4iYCBsk
Haswha6vsC6eep3BwEIc4gLw6uBK0u+QDrTBQBbwb4VCSmT3pDCg/r8uoydajotY
uK3DGReEY+1vVv2Dy2A0xHS+5p3b4eTlygxfFQIDAQABo4IBbjCCAWowHwYDVR0j
BBgwFoAUU3m/WqorSs9UgOHYm8Cd8rIDZsswHQYDVR0OBBYEFI2MXsRUrYrhd+mb
+ZsF4bgBjWHhMA4GA1UdDwEB/wQEAwIBhjASBgNVHRMBAf8ECDAGAQH/AgEAMB0G
A1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjAbBgNVHSAEFDASMAYGBFUdIAAw
CAYGZ4EMAQIBMFAGA1UdHwRJMEcwRaBDoEGGP2h0dHA6Ly9jcmwudXNlcnRydXN0
LmNvbS9VU0VSVHJ1c3RSU0FDZXJ0aWZpY2F0aW9uQXV0aG9yaXR5LmNybDB2Bggr
BgEFBQcBAQRqMGgwPwYIKwYBBQUHMAKGM2h0dHA6Ly9jcnQudXNlcnRydXN0LmNv
bS9VU0VSVHJ1c3RSU0FBZGRUcnVzdENBLmNydDAlBggrBgEFBQcwAYYZaHR0cDov
L29jc3AudXNlcnRydXN0LmNvbTANBgkqhkiG9w0BAQwFAAOCAgEAMr9hvQ5Iw0/H
ukdN+Jx4GQHcEx2Ab/zDcLRSmjEzmldS+zGea6TvVKqJjUAXaPgREHzSyrHxVYbH
7rM2kYb2OVG/Rr8PoLq0935JxCo2F57kaDl6r5ROVm+yezu/Coa9zcV3HAO4OLGi
H19+24rcRki2aArPsrW04jTkZ6k4Zgle0rj8nSg6F0AnwnJOKf0hPHzPE/uWLMUx
RP0T7dWbqWlod3zu4f+k+TY4CFM5ooQ0nBnzvg6s1SQ36yOoeNDT5++SR2RiOSLv
xvcRviKFxmZEJCaOEDKNyJOuB56DPi/Z+fVGjmO+wea03KbNIaiGCpXZLoUmGv38
sbZXQm2V0TP2ORQGgkE49Y9Y3IBbpNV9lXj9p5v//cWoaasm56ekBYdbqbe4oyAL
l6lFhd2zi+WJN44pDfwGF/Y4QA5C5BIG+3vzxhFoYt/jmPQT2BVPi7Fp2RBgvGQq
6jG35LWjOhSbJuMLe/0CjraZwTiXWTb2qHSihrZe68Zk6s+go/lunrotEbaGmAhY
LcmsJWTyXnW0OMGuf1pGg+pRyrbxmRE1a6Vqe8YAsOf4vmSyrcjC8azjUeqkk+B5
yOGBQMkKW+ESPMFgKuOXwIlCypTPRpgSabuY0MLTDXJLR27lk8QyKGOHQ+SwMj4K
00u/I5sUKUErmgQfky3xxzlIPK1aEn8=
-----END CERTIFICATE-----
)CERT";
X509List cert(rootCACertificate);

char html_template[] PROGMEM = R"=====(
  <!DOCTYPE html>
  <html>
    <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>Dashboard</title>
        <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
        <style>
          .loading-overlay {
            display: none;
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: rgba(255, 255, 255, 0.8);
            z-index: 9999;
            text-align: center;
            padding-top: 20%;
          }
        </style>
        <script>
          function updateRuntime() {
            const currentUnixTime = Math.floor(Date.now() / 1000); // Current Unix time in seconds
            const runtime = injectTime_data - startTime_data; // Difference in seconds
            let displayTime;
            if (runtime < 3600) {
              const minutes = Math.floor(runtime / 60);
              displayTime = minutes + ' minute' + (minutes > 1 ? 's' : '');
            } else if (runtime < 86400) {
              const hours = Math.floor(runtime / 3600);
              const minutes = Math.floor((runtime % 3600) / 60);
              displayTime = hours + ' hour' + (hours > 1 ? 's' : '') + ' ' + minutes + ' minute' + (minutes > 1 ? 's' : '');
            } else {
              const days = Math.floor(runtime / 86400);
              const hours = Math.floor((runtime % 86400) / 3600);
              displayTime = days + ' day' + (days > 1 ? 's' : '') + ' ' + hours + ' hour' + (hours > 1 ? 's' : '');
            }

            document.getElementById('runtime').innerText = displayTime;
          }
          let injectTime_data;
          let startTime_data;
          let socket = new WebSocket("ws://" + window.location.hostname + ":81");
          socket.onmessage  = 
          function(event) {  
            let full_data = event.data;
            console.log(full_data);
            let data = JSON.parse(full_data);
            let id_data = data.id;
            let name_data = data.name;
            let clamp_data = data.cla;
            let inject_data = data.inj;
            let cycle_data = data.cyc;
            let shoot_data = data.shoot;
            injectTime_data = data.time;
            startTime_data = data.startTime;
            let version_data = data.version;

            if (inject_data == 1){ // take the timestamp when inject
              let unixTime = injectTime_data;
              let date = new Date(unixTime * 1000); // Convert to milliseconds
              let options = { day: 'numeric', month: 'numeric', year: 'numeric' };
              let formattedDate = date.toLocaleDateString("en-GB", options);
              let formattedTime = date.toLocaleTimeString("en-GB");

              document.getElementById("date_value").innerHTML = formattedDate;
              document.getElementById("time_value").innerHTML = formattedTime;

              // showing runtime
              updateRuntime();
            }

            if (clamp_data == 0) {
              clamp_data = "X";
              clamp_data = clamp_data.fontcolor("red");
            } else {
              clamp_data = "O";
              clamp_data = clamp_data.fontcolor("green");
            }
            if (inject_data == 0) {
              inject_data = "X";
              inject_data = inject_data.fontcolor("red");
            } else {
              inject_data = "O";
              inject_data = inject_data.fontcolor("green");
            }

            document.getElementById("id_value").innerHTML = id_data;
            document.getElementById("ver_value").innerHTML = version_data;
            document.getElementById("name_value").innerHTML = name_data;
            document.title = name_data;
            document.getElementById("cla_value").innerHTML = clamp_data;
            document.getElementById("inj_value").innerHTML = inject_data;
            document.getElementById("cyc_value").innerHTML = cycle_data;
            document.getElementById("shoot_value").innerHTML = shoot_data;
          };
        </script>
    </head>
    <body>
      <div class="loading-overlay" id="loadingOverlay">
        <div class="spinner-border text-primary" role="status">
          <span class="visually-hidden">Loading...</span>
        </div>
        <div>Loading...</div>
      </div>
      <script>
        document.getElementById('loadingOverlay').style.display = 'block';
      </script>
      <div class="container-fluid mt-2 text-center">
        <div class="card col-md-4">
          <div class="card-header bg-primary text-white text-center">
            <h5 id="name_value" class="card-title mb-0">Loading..</h5>
          </div>
          <div class="card-body">
            <div class="row mb-2">
              <div class="col-6">
                <h5>Cycletime:</h5>
                <span id="cyc_value" class="fs-2">...</span>
              </div>
              <div class="col-6">
                <h5>Total Shoot:</h5>
                <span id="shoot_value" class="fs-2">...</span>
              </div>
            </div>
            <div class="row mb-2">
              <div class="col-6">
                <h5>Last Shoot:</h5>
                <span id="date_value" class="fs-5">...</span> <span id="time_value" class="fs-5"></span>
              </div>
              <div class="col-6">
                <h5>Uptime:</h5>
                <span id="runtime" class="fs-5">..</span>
              </div>
            </div>
            <hr>
            <div class="row">
              <div class="col-3">
                <h6>Clamp</h6>
                <p id="cla_value" class="fs-4 fw-bold">-</p>
              </div>
              <div class="col-3">
                <h6>Inject</h6>
                <p id="inj_value" class="fs-4 fw-bold">-</p>
              </div>
              <div class="col-3">
                <h6>MC ID:</h6>
                <p id="id_value" class="fs-6">...</p>
              </div>
              <div class="col-3">
                <h6>Version:</h6>
                <p id="ver_value" class="fs-6">...</p>
              </div>
            </div>
          </div>
          <div class="card-footer">
            <button class="btn btn-danger" onclick="location.href='/reset'">Reset</button>
          </div>
        </div>
      </div>
      <script>
        window.addEventListener('load', function() {
            document.getElementById('loadingOverlay').style.display = 'none';
        });
      </script>
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

// BEGIN OF WEBSOCKET PART
// as WEBSOCKET SERVER
void webSocketEvent_server(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket_server.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        // sendws = true;

        // send message to client
        // webSocket_server.sendTXT(num, "Connected");
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

// as WEBSOCKET CLIENT
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WS CLIENT] Disconnected!\n");
      websocketConnected = false;
      break;
    case WStype_CONNECTED:
      {
        Serial.printf("[WS CLIENT] Connected to ws://%s:%d%s\n", WSaddress, port, payload);

        // send message to server when Connected
        // Serial.println("[WS CLIENT] SENT: Connected");
        // webSocket.sendTXT("Connected");
        websocketConnected = true;
      }
      break;
    case WStype_TEXT:
      Serial.printf("[WS CLIENT] RESPONSE: %s\n", payload);
      break;
    case WStype_BIN:
      Serial.printf("[WS CLIENT] get binary length: %u\n", length);
      hexdump(payload, length);
      break;
    case WStype_PING:
      // pong will be send automatically
      Serial.printf("[WS CLIENT] get ping\n");
      break;
    case WStype_PONG:
      // answer to a ping we send
      Serial.printf("[WS CLIENT] get pong\n");
      break;
  }
}
// END OF WEBSOCKET PART

// BEGIN OF WEBSERVER HANDLING PART
void handleMain() {
  server.send_P(200, "text/html", html_template);
}
void handleReset() {
  server.send_P(200, "text/html", html_reset);
}
void ConfirmReset() {
  server.send_P(200, "text/html", "<html><body><p>ESP has been reset</p></body></html>");
  delay(3000);
  wifiManager.resetSettings();
  ESP.restart();
}
void handleRestart() {
  server.send_P(200, "text/html", "<html><body><p>ESP has been restart</p></body></html>");
  ESP.restart();
}
void handleNotFound() {
  server.send(404, "text/html", "<html><body><p>404 Error</p></body></html>");
}
// END OF WEBSERVER HANDLING PART

// FUNCTION PART OF WIFIMANAGER
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void connectWifi() {
  wifiManager.setConfigPortalTimeout(60);
  if (!wifiManager.autoConnect()) {
    Serial.println("*wm:Failed to connect and hit timeout");
    delay(1000);
    connectWifi();
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("");
  String versiSW = "[ARDUINO] Setup v";
  versiSW += versionNum;
  Serial.println(versiSW);

  Serial.println("*wm:Mounting FS...");
  // read configuration from FS json
  if (SPIFFS.begin()) {
    Serial.println("*wm:Mounted file system");
    if (SPIFFS.exists("/config.json")) {
      // file exists, reading and loading
      Serial.println("*wm:Reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("*wm:Opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        // DynamicJsonBuffer jsonBuffer;
        // JsonObject& json = jsonBuffer.parseObject(buf.get());
        // json.printTo(Serial);
        // if (json.success()) {
        DynamicJsonDocument json(512);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if (!deserializeError) {
          Serial.println("\n*wm:parsed json");
          strcpy(deviceId, json["deviceId"]);
          strcpy(deviceName, json["deviceName"]);
          strcpy(WSaddress, json["WSaddress"]);
        } else {
          Serial.println("*wm:Failed to load json config");
        }
      }
    }
  } else {
    Serial.println("*wm:Failed to mount FS");
  }
  // end read

  wifiMillis = millis();

  // WIFIMANAGER SETUP
  // WiFiManagerParameter <function name>(id/name, placeholder/prompt, default, length)
  WiFiManagerParameter customDeviceId("deviceId", "Device Id", deviceId, 10);
  WiFiManagerParameter customDeviceName("deviceName", "Device Name", deviceName, 50);
  WiFiManagerParameter customWSaddress("WSaddress", "Websocket Address", WSaddress, 16);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&customDeviceId);
  wifiManager.addParameter(&customDeviceName);
  wifiManager.addParameter(&customWSaddress);

  connectWifi();

  // wifiManager.resetSettings();
  // AP esp if can't connect to wifi
  // wifiManager.autoConnect();
  strcpy(deviceId, customDeviceId.getValue());
  strcpy(deviceName, customDeviceName.getValue());
  strcpy(WSaddress, customWSaddress.getValue());

  if (shouldSaveConfig) {
    Serial.println("*wm:Saving config");
    DynamicJsonDocument json(512);
    json["deviceId"] = deviceId;
    json["deviceName"] = deviceName;
    json["WSaddress"] = WSaddress;

    // save inputted parameter to config file
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("*wm:Failed to open config file for writing");
    }
    serializeJson(json, Serial);
    serializeJson(json, configFile);
    configFile.close();
    // end save
  }

  Serial.println("\nWiFi Connected..");
  wifiDownSecond = (millis() - wifiMillis) / 1000;  // Wifi Downtime in second
  wifiDownMinute = wifiDownSecond / 60;             // Wifi Downtime in minute
  delay(100);
  wifiMillis = 0;
  wifiDownSecond = 0;
  wifiDownMinute = 0;  

  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  ipAddress = WiFi.localIP().toString();

  // WEBSERVER SETUP
  server.on("/", handleMain);
  server.on("/reset", handleReset);
  server.on("/confirm_reset", ConfirmReset);
  server.on("/restart", handleRestart);
  server.onNotFound(handleNotFound);
  server.begin();

  // NTP SETUP
  configTime(0, 0, "pool.ntp.org");
  Serial.print("[NTP] Waiting for NTP time sync.");
  unixTime = time(nullptr);
  int toRestart = 0;
  while (unixTime < 3600) {
    delay(1000);
    Serial.print(".");
    toRestart++;
    if (toRestart == 30) {
      ESP.restart();
    }
    unixTime = time(nullptr);
  }
  Serial.println("");
  Serial.println("[NTP] NTP time has been synchronized");

  // WEBSOCKET SERVER SETUP
  Serial.println("[WS SERVER] Starting WebSocket Server");
  webSocket_server.begin();
  webSocket_server.onEvent(webSocketEvent_server);

  // WEBSOCKET CLIENT SETUP
  Serial.print("[WS CLIENT] Trying connect to ws://");
  Serial.print(WSaddress);
  Serial.println(":1880/");
  
  webSocket.begin(WSaddress, port, "/");  // Websocket server address
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

  // PIN SETUP
  pinMode(pinClamp, INPUT);
  pinMode(pinInject, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  stateClamp = digitalRead(pinClamp);
  lastInject = millis();

  startTime = time(nullptr);
  startIdle = millis();

  ipAddress = WiFi.localIP().toString();
  Serial.println("[ARDUINO] System ready");

  checkFirmwareUpdate();
}

// CAPTURE INJECT PROCESS
void monitorCycleTime() {
  stateClamp = digitalRead(pinClamp);

  if (stateClamp != laststateClamp) {
    if (stateClamp == HIGH) {  // Clamp = ON
      lastClamp = millis();
      staCla = 1;
      sendws = true;
    } else {  // Clamp = OFF
      staCla = 0;
      sendws = true;
    }
    laststateClamp = stateClamp;
  }
  if (stateClamp == HIGH) {
    stateInject = digitalRead(pinInject);
    if (stateInject != laststateInject) {
      if (stateInject == HIGH) {  // Inject = ON
        staInj = 1;
        shoot += 1;
        ipAddress = WiFi.localIP().toString();

        timenow = millis();
        cycleTime = (timenow - lastInject) / 1000;
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
        unixTime = time(nullptr);

        if (wiFiConnected == false || websocketConnected == false) {
          numct = cycleTime;
          writefile();
        } else if (wiFiConnected == true && websocketConnected == true) {
          File file = SPIFFS.open("/down.txt", "r");
          if (file && websocketConnected == true) {
            readfile();
          } else {
            numct = cycleTime;
          }
        }
        sendws = true;
        startIdle = millis();
      } else {  // Inject = OFF
        staInj = 0;
        sendws = true;
      }
    }
    laststateInject = stateInject;
  }
}

// CHECK TIME FOR CHECK FIRMWARE UPDATE
void timeToCheck() {
  time_t unix = time(nullptr);
  unix += timeZone;
  gmtime_r(&unix, &timeinfo);
  int currentDay = timeinfo.tm_wday;   // tm_wday: 0=Sunday, 1=Monday, ..., 6=Saturday
  int currentHour = timeinfo.tm_hour;  // Getting current Hour
  int currentMin = timeinfo.tm_min;    // Getting current Minute

  // Monday - Friday
  if (currentDay >= 1 && currentDay <= 5) {
    // 09:00 - 15:00
    if (currentHour >= 9 && currentHour <= 15) {
      // Boolean 'is_update' to check if device already updated today
      if (is_update == false) {
        // Capture second from last Inject activity (startIdle)
        int idleSeconds = (millis() - startIdle) / 1000;
        // Idle for 600 seconds (10 minutes)
        if (idleSeconds >= 600) {
          checkFirmwareUpdate();
        }
      }
    }
  }
  // Reset 'is_update' at beginning of the day to check if in that day there is an firmware update
  if (currentHour == 0) {
    is_update = false;
  }
}

// CHECK FIRMWARE UPDATE
void checkFirmwareUpdate() {
  WiFiClientSecure client;
  client.setTrustAnchors(&cert);
  HTTPClient https;
  https.begin(client, versionUrl);

  Serial.println("[UPDATE] Checking for new Firmware..");

  int httpCode = https.GET();
  if (httpCode == HTTP_CODE_OK) {
    String latestVersion = https.getString();
    latestVersion.trim();
    Serial.printf("[UPDATE] Current version: %s\n", versionNum);
    Serial.printf("[UPDATE] Latest version: %s\n", latestVersion);
    if (versionNum != latestVersion) {
      Serial.println("[UPDATE] New firmware available. Updating...");
      t_httpUpdate_return ret = ESPhttpUpdate.update(client, firmwareUrl);

      if (ret != HTTP_UPDATE_OK) {
        Serial.printf("[UPDATE] Update failed (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      } else {
        Serial.println("[UPDATE] Firmware update completed.");
      }
    } else {
      Serial.println("[UPDATE] Firmware is up to date.");
      // To prevent http requesting the rest of the day
      is_update = true;
    }
  } else {
    Serial.printf("[UPDATE] Failed to check for updates (%d)\n", httpCode);
  }
  https.end();
}

// SPIFFS READ DOWNTIME
void readfile() {
  File file = SPIFFS.open("/down.txt", "r");

  StaticJsonDocument<512> jsonArrayDoc;
  JsonArray jsonArray = jsonArrayDoc.to<JsonArray>();

  // Read the file contents
  while (file.available()) {
    String contents = file.readStringUntil('\n');
    contents.trim();
    String timeString;
    int commaPos = contents.indexOf(',');
    if (commaPos != -1) {
      numct = contents.substring(0, commaPos).toInt();
      timeString = contents.substring(commaPos + 2);

      unsigned long time = strtoul(timeString.c_str(), NULL, 10);

      // Create a JSON object for each data point
      JsonObject jsonData = jsonArray.createNestedObject();
      jsonData["id"] = deviceId;
      jsonData["cycletime"] = numct;
      jsonData["ip"] = ipAddress;
      jsonData["time"] = time;
    }
  }
  // Serialize the JSON array to a string
  String jsonString;
  serializeJson(jsonArray, jsonString);

  Serial.println(jsonString);
  webSocket.sendTXT(jsonString);

  file.close();
  SPIFFS.remove("/down.txt");
}

// SPIFFS WRITE DOWNTIME
void writefile() {
  File file = SPIFFS.open("/down.txt", "a");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  // Write cycletime data to file
  String data;
  data += cycleTime;
  data += ", ";
  data += unixTime;
  file.println(data);

  // Close file
  file.close();
}

// JSON DATA
void senddata() {
  JSON_Data = "{";
  JSON_Data += "\"action\":\"";
  JSON_Data += action;
  JSON_Data += "\",\"id\":";
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
  JSON_Data += ",\"time\":";
  JSON_Data += unixTime;
  JSON_Data += ",\"startTime\":";
  JSON_Data += startTime;
  JSON_Data += ",\"version\":\"";
  JSON_Data += versionNum;
  JSON_Data += "\"";
  JSON_Data += "}";
  Serial.println(JSON_Data);
  webSocket.sendTXT(JSON_Data);
  webSocket_server.broadcastTXT(JSON_Data);
  sendws = false;
}

void loop() {

  monitorCycleTime();

  if (!WiFi.isConnected() || WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    // Disconnect from Wifi
    if (wiFiConnected == true) {
      Serial.println("WiFi disconnected, reconnecting...");
      wifiMillis = millis();
      WiFi.begin();
      digitalWrite(LED_BUILTIN, HIGH);
      wiFiConnected = false;
      websocketConnected = false;
    }
  } else {
    // Connected to Wifi
    webSocket.setReconnectInterval(5000);
    webSocket.loop();
    server.handleClient();
    webSocket_server.loop();
    ArduinoOTA.handle();

    // Create Interval to run Check Update Firmware
    unsigned long currentCheckSeconds = millis() / 1000;
    if (currentCheckSeconds - previousCheckSeconds >= 300) {
      previousCheckSeconds = currentCheckSeconds;
      timeToCheck();
    }

    if (wiFiConnected == false) {
      wifiDownSecond = (millis() - wifiMillis) / 1000;  // Wifi Downtime in second
      wifiDownMinute = wifiDownSecond / 60;             // Wifi Downtime in minute
      Serial.println(wifiDownSecond);
      Serial.println(wifiDownMinute);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
      ipAddress = WiFi.localIP().toString();
      Serial.println("Connected to WiFi");
      wiFiConnected = true;
    }
  }

  // Send data via websocket as JSON format
  if (sendws == true) {
    senddata();
  }
}

/*
Update:
Menambahkan kemampuan update firmware otomatis, dengan mengecek versi pada server dan menarik update file firmware dari server.
Mengubah tampilan Webserver pada Wemos untuk menunjukan nomor versi. 
*/