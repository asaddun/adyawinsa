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
 * D. OTA (Over the air) 
 *    - WIFI update
 *    - update software/firmware from network
 * E. WIFI MANAGER
 *    - setting to ACCESS POINT at first time running
 *
 * @Author: Abraham Sulaeman- 19 Mei 2022
 * Update: Muhammad Asad- 05 Mei 2026 v4.2.1
 */

void (*resetFunc)(void) = 0;  // declare reset function @ address 0

#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
// #include <WiFiClientSecureBearSSL.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

#define pinClamp D6   // Clamp Process
#define pinInject D7  // Injection Process
#define pinLeader D1
#define pinQc D2
#define pinMesin D5
#define pinMold D8

// Custom Data Type
enum StatusType {
  STATUS_IDLE,
  STATUS_RUNNING,
  STATUS_RESTART,
  STATUS_RESET,
  STATUS_ERROR
};

enum AndonType {
  ANDON_LEADER,
  ANDON_QC,
  ANDON_MAINTENANCE,
  ANDON_TOOL
};

enum RequestType {
  REQ_NEW,
  REQ_SHOOT_FILE,
  REQ_ANDON_FILE
};

// DECLARE VARIABLE
const char* versionUrl = "https://apik.adyawinsa.com/smsd/api/update-arduino/version.txt";
const char* firmwareUrl = "https://apik.adyawinsa.com/smsd/api/update-arduino/firmware.bin";
String versionNum = "4.2.0"; // System Version
bool laststateInject = LOW, laststateClamp = LOW, stateClamp = LOW, stateInject = LOW;  // previous state of the button
unsigned long timenow;
unsigned long cycleTime;
unsigned long lastClamp;
unsigned long lastInject;
int staCla, staInj = 0, shoot;
unsigned long runtime;
int run_second, run_minute, run_hour, run_day;
int c_day, c_month, c_year;
bool wifiConnected = true, sendData = false, needResponse = false;
char deviceId[10], deviceName[50];
char subTopic[36], broadcastTopic[20], pubTopic[36], mqttUser[10], mqttPassword[20];
bool shouldSaveConfig = false, readConfig = false;
String ipAddress;
bool wiFiConnected = true, mqttConnected = false, savingDataToFile = false;
unsigned long wifiMillis, wifiDownSecond, wifiDownMinute, MqttMillis;
unsigned long unixTime, startTime, startIdle;
int timeToIdle = 5 * 60;
StatusType deviceStatus = STATUS_IDLE;
const char* statusText[] = {
  "idle",
  "running",
  "restarting",
  "resetting",
  "error"
};
struct tm timeinfo;
const int timeZone = 7 * 3600;
unsigned long previousCheckSeconds, previousStatusSeconds;
bool is_update = false;
bool stateLeader = 0, laststateLeader = 0;
bool stateQc = 0, laststateQc = 0;
bool stateMesin = 0, laststateMesin = 0;
bool stateMold = 0, laststateMold = 0;
String buttonAction;
AndonType andonType = ANDON_LEADER;
const char* andonAction[] = {
  "AL",
  "AQ",
  "AM",
  "AT"
};
bool buttonValue = 0, sendAndon = 0;
unsigned long previousButtonMillis;
unsigned long leaderLastDebounce = 0, qcLastDebounce = 0, mesinLastDebounce = 0, moldLastDebounce = 0;
unsigned long previousBlink = 0;
bool ledState;
uint8_t counterLeader, counterQc, counterMesin, counterMold;
struct Request {
  unsigned long id;
  String payload;
  unsigned long timeSent;
  bool waiting;
  RequestType type;
};
const uint8_t MAX_REQUESTS = 10;
const int REQUEST_TIMEOUT = 5000;

// OBJECTS
WiFiManager wifiManager;
WiFiManagerParameter customDeviceId("deviceId", "Device Id", deviceId, 10);
WiFiManagerParameter customDeviceName("deviceName", "Device Name", deviceName, 50);
ESP8266WebServer server(80);
// BearSSL::WiFiClientSecure client;
WiFiClient client;
PubSubClient mqttClient(client);
Request requests[MAX_REQUESTS];

// =========== MQTT ===========
void callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.println("Gagal parsing JSON");
    return;
  }

  const char* actionMessage = doc["action"];
  const char* valueMessage = doc["value"];

  if (strcmp(actionMessage, "power") == 0){
    if (strcmp(valueMessage, "restart") == 0) {
      deviceStatus = STATUS_RESTART;
      sendStatusData();
      delay(1000);
      ESP.restart();
    } else if (strcmp(valueMessage, "reset") == 0) {
      deviceStatus = STATUS_RESET;
      sendStatusData();
      delay(1000);
      wifiManager.resetSettings();
      ESP.restart();
    }

  } else if (strcmp(actionMessage, "wifi") == 0) {
    JsonObject wifi = doc["value"];
    if (wifi.isNull()) {
        Serial.println("[MQTT] WiFi config tidak valid");
        return;
    }

    const char* ssid = wifi["ssid"];
    const char* password = wifi["password"];

    if (WiFi.SSID() == ssid) {
      return;
    }

    if (saveWiFiConfig(ssid, password)) {
      delay(500);
      ESP.restart();
    }

  } else if (strcmp(actionMessage, "status") == 0) {
    unsigned long idMessage = doc["id"];
    bool found = false;

    for (int i = 0; i < MAX_REQUESTS; i++) {
      if (requests[i].waiting && requests[i].id == idMessage) {
        found = true;
        RequestType t = requests[i].type;

        if (strcmp(valueMessage, "success") == 0) {
          Serial.printf("[MQTT] Response ID %d received\n", idMessage);
          clearRequest(idMessage);  // hapus atau tandai sudah selesai
          if (t == REQ_SHOOT_FILE) {
            SPIFFS.remove("/down.txt");
            savingDataToFile = false;
          }
          if (t == REQ_ANDON_FILE) {
            SPIFFS.remove("/andon.txt");
            savingDataToFile = false;
          }
        } else {
          Serial.printf("Respon NG untuk ID %d\n", idMessage);
          // kamu bisa putuskan mau retry, simpan SPIFFS, dll
        }

        break;  // selesai, keluar dari loop
      }
    }

    if (!found) {
      // Serial.println("Respon tidak ditemukan di daftar request aktif!");
    }
  } else if (strcmp(actionMessage, "debug") == 0) {
    if (strcmp(valueMessage, "file") == 0) {
      bool setValue = doc["set"].as<bool>();
      if (setValue) {
        mqttConnected = true;
        Serial.println("file-true");
      } else {
        mqttConnected = false;
        Serial.println("file-false");
      }
    }
    if (strcmp(valueMessage, "clear-file") == 0) {
      SPIFFS.remove("/down.txt");
      SPIFFS.remove("/andon.txt");
      SPIFFS.remove("/wifi_config.json");
      Serial.println("file cleared");
    }      
  }
}

void reconnectMQTT() {
  StaticJsonDocument<256> doc;

  doc["action"] = "status";
  doc["id"] = deviceId;
  doc["name"] = deviceName;
  doc["value"] = "offline";
  doc["time"] = time(nullptr);

  char jsonBuffer[256];
  size_t len = serializeJson(doc, jsonBuffer);

  if (mqttClient.connected()) return;

  if (millis() - MqttMillis < 5000) return;
  MqttMillis = millis();

  Serial.println("[MQTT] Reconnecting...");

  // mqttClient.disconnect();

  if (mqttClient.connect(deviceId, mqttUser, mqttPassword, pubTopic, 1, true, jsonBuffer)) {
    mqttConnected = true;
    Serial.println("[MQTT] Connected");
    digitalWrite(LED_BUILTIN, LOW);

    mqttClient.subscribe(subTopic);
    mqttClient.subscribe(broadcastTopic);
    Serial.print("[MQTT] Subscribed to: ");
    Serial.println(subTopic);

    sendStatusData();
  } else {
    mqttConnected = false;
    Serial.print("[MQTT] Failed, rc=");
    Serial.println(mqttClient.state());
  } 
}
// =========== MQTT ===========


// BEGIN OF WEBSERVER HANDLING PART
void handleRoot() {
  // URL tujuan redirect
  String redirectUrl = "http://api.adyawinsa.com:1880/sensor?id=";
  redirectUrl += deviceId;

  // Kirim header redirect 302
  server.sendHeader("Location", redirectUrl, true);
  server.send(302, "text/plain", "Redirecting...");
}
// END OF WEBSERVER HANDLING PART

bool connectFromWiFiConfig()
{
    File file = SPIFFS.open("/wifi_config.json", "r");

    if (!file) {
        Serial.println("[SYSTEM] wifi_config.json tidak ditemukan");
        return false;
    }

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.print("[SYSTEM] Gagal membaca wifi_config.json: ");
        Serial.println(error.c_str());
        return false;
    }

    const char* ssid = doc["ssid"];
    const char* password = doc["password"];

    if (!ssid || !password) {
        Serial.println("[SYSTEM] SSID/password tidak valid");
        return false;
    }

    Serial.print("[SYSTEM] Connecting to WiFi: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    unsigned long start = millis();

    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < 10000) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }

    Serial.println("[SYSTEM] Gagal connect dari wifi_config.json");
    return false;
}

bool saveWiFiConfig(const String& ssid, const String& password)
{
    StaticJsonDocument<256> doc;

    doc["ssid"] = ssid;
    doc["password"] = password;

    File file = SPIFFS.open("/wifi_config.json", "w");

    if (!file) {
        Serial.println("[SYSTEM] Gagal membuka wifi_config.json untuk ditulis");
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.println("[SYSTEM] Gagal menulis wifi_config.json");
        file.close();
        return false;
    }

    file.close();

    Serial.println("[SYSTEM] WiFi config berhasil disimpan");
    return true;
}

// FUNCTION PART OF WIFIMANAGER
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// void connectWifi() {
//   wifiManager.setConfigPortalTimeout(90);
//   if (!wifiManager.autoConnect()) {
//     Serial.println("*wm:Failed to connect and hit timeout");
//     delay(1000);
//     connectWifi();
//   } else {
//     if (!readConfig){
//       strcpy(deviceId, customDeviceId.getValue());
//       strcpy(deviceName, customDeviceName.getValue());
//     }
//   }
// }

void connectWifi() {
  if (SPIFFS.exists("/wifi_config.json")) {

    Serial.println("[SYSTEM] wifi_config.json ditemukan");

    if (connectFromWiFiConfig()) {
      // Berhasil menggunakan config utama
      return;
    }

    Serial.println("[SYSTEM] Config WiFi gagal, mencoba WiFiManager...");
  }
  else {
    Serial.println("[SYSTEM] wifi_config.json belum ada");
    Serial.println("[SYSTEM] Menjalankan WiFiManager...");
  }

  // Fallback / provisioning
  wifiManager.setConfigPortalTimeout(90);

  if (!wifiManager.autoConnect()) {
    Serial.println("*wm: Failed to connect and hit timeout");
    delay(1000);
    connectWifi();
    return;
  }

  Serial.println("[SYSTEM] WiFiManager berhasil terhubung");

  // Simpan WiFi yang berhasil digunakan ke wifi_config.json
  String ssid = wifiManager.getWiFiSSID();
  String pass = wifiManager.getWiFiPass();

  saveWiFiConfig(ssid, pass);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("");
  Serial.println("[SYSTEM] Booting..");

  Serial.println("*wm:Mounting FS...");
  // read configuration from FS json
  if (SPIFFS.begin()) {
    Serial.println("*wm:Mounted file system");
    if (SPIFFS.exists("/config.json")) {
      // SPIFFS.remove("/config.json");
      // file exists, reading and loading
      Serial.println("*wm:Reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.print("*wm:Opened config file: ");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument json(512);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if (!deserializeError) {
          Serial.println("\n*wm:parsed json");
          strcpy(deviceId, json["deviceId"]);
          strcpy(deviceName, json["deviceName"]);
          readConfig = true;
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
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&customDeviceId);
  wifiManager.addParameter(&customDeviceName);

  connectWifi();
  // wifiManager.resetSettings();

  if (shouldSaveConfig) {
    Serial.println("*wm:Saving config");
    DynamicJsonDocument json(512);
    json["deviceId"] = deviceId;
    json["deviceName"] = deviceName;

    // save inputted parameter to config file
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("*wm:Failed to open config file for writing");
    }
    serializeJson(json, Serial);
    serializeJson(json, configFile);
    configFile.close();

    String ssid = wifiManager.getWiFiSSID();
    String pass = wifiManager.getWiFiPass();

    saveWiFiConfig(ssid, pass);
    // end save
  }

  Serial.println("[SYSTEM] WiFi Connected.");
  wifiDownSecond = (millis() - wifiMillis) / 1000;  // Wifi Downtime in second
  wifiDownMinute = wifiDownSecond / 60;             // Wifi Downtime in minute
  delay(100);
  wifiMillis = 0;
  wifiDownSecond = 0;
  wifiDownMinute = 0;

  Serial.print("[SYSTEM] Local IP: ");
  Serial.println(WiFi.localIP());
  ipAddress = WiFi.localIP().toString();

  // WEBSERVER SETUP
  server.on("/", handleRoot);
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

  // client.setInsecure();
  // client.setBufferSizes(512, 512);

  sprintf(subTopic, "sensor/injection/%s/response", deviceId);
  strcpy(broadcastTopic, "sensor/broadcast");
  sprintf(pubTopic, "sensor/injection/%s/request", deviceId);
  strcpy(mqttUser, "esp8266");
  strcpy(mqttPassword, "esp8266-mqtt");
  mqttClient.setBufferSize(4096);
  mqttClient.setServer("192.168.3.6", 1883);
  mqttClient.setCallback(callback);

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
  pinMode(pinLeader, INPUT);
  pinMode(pinQc, INPUT);
  pinMode(pinMesin, INPUT);
  pinMode(pinMold, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  stateClamp = digitalRead(pinClamp);
  lastInject = millis();

  startTime = time(nullptr);
  startIdle = millis();

  ipAddress = WiFi.localIP().toString();

  checkFirmwareUpdate();

  Serial.println("[SYSTEM] System ready");
}

// CAPTURE INJECT PROCESS
void monitorCycleTime() {
  stateClamp = digitalRead(pinClamp);

  if (stateClamp != laststateClamp) {
    if (stateClamp == HIGH) {  // Clamp = ON
      lastClamp = millis();
      staCla = 1;
      sendData = true;
    } else {  // Clamp = OFF
      staCla = 0;
      sendData = true;
    }
    laststateClamp = stateClamp;
    startIdle = millis();
  }

  if (stateClamp == HIGH) {
    stateInject = digitalRead(pinInject);
    if (stateInject != laststateInject) {
      if (stateInject == HIGH) {  // Inject = ON
        staInj = 1;
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

        // Checking if the Wemos is from Booting, to get cycletime from last inject before shut down
        if (shoot == 0) {
          File savedFile = SPIFFS.open("/last.txt", "r");
          if (!savedFile) {
            Serial.println("Failed to open file for writing");
          } else {
            String fileContent = savedFile.readString();
            unsigned long savedLast = fileContent.toInt();  // Convert to integer
            cycleTime = unixTime - savedLast;
          }
          savedFile.close();
        } else {
          // Save last inject's time
          File lastFile = SPIFFS.open("/last.txt", "w");
          if (!lastFile) {
            Serial.println("Failed to open file for writing");
          } else {
            lastFile.print(unixTime);
          }
          lastFile.close();
        }

        needResponse = true;
        shoot += 1;
        sendData = true;
        deviceStatus = STATUS_RUNNING;
        sendStatusData();
      } else {  // Inject = OFF
        staInj = 0;
        sendData = true;
      }
      startIdle = millis();
    }
    laststateInject = stateInject;
  }
}

unsigned long getMillis() {
  return millis();
}

void buttonAndon() {
  // Read the state of the button
  stateLeader = digitalRead(pinLeader);
  stateQc = digitalRead(pinQc);
  stateMesin = digitalRead(pinMesin);
  stateMold = digitalRead(pinMold);

  // Send data as Leader
  if (stateLeader != laststateLeader) {
    counterLeader++;
    if (counterLeader == 1) {
      leaderLastDebounce = getMillis();
    }
    if ((millis() - leaderLastDebounce) >= 3000) {
      if (stateLeader == HIGH) {
        // Serial.println("Leader 1");
        // buttonAction = "AL";
        buttonValue = 1;
        // sendAndon = 1;
      } else if (stateLeader == LOW) {
        // Serial.println("Leader 0");
        // buttonAction = "AL";
        buttonValue = 0;
      }
      andonType = ANDON_LEADER;
      sendAndon = 1;
      laststateLeader = stateLeader;
      counterLeader = 0;
    }
  }
  if (stateLeader == laststateLeader) {
    counterLeader = 0;
  }

  // Send data as QC
  if (stateQc != laststateQc) {
    counterQc++;
    if (counterQc == 1) {
      qcLastDebounce = getMillis();
    }
    if (millis() - qcLastDebounce >= 3000) {
      if (stateQc == HIGH) {
        // Serial.println("QC 1");
        // buttonAction = "AQ";
        buttonValue = 1;
        // sendAndon = 1;
      } else if (stateQc == LOW) {
        // Serial.println("QC 0");
        // buttonAction = "AQ";
        buttonValue = 0;
      }
      andonType = ANDON_QC;
      sendAndon = 1;
      laststateQc = stateQc;
      counterQc = 0;
    }
  }
  if (stateQc == laststateQc) {
    counterQc = 0;
  }

  // Send data as Mesin
  if (stateMesin != laststateMesin) {
    counterMesin++;
    if (counterMesin == 1) {
      mesinLastDebounce = getMillis();
    }
    if (millis() - mesinLastDebounce >= 3000) {
      if (stateMesin == HIGH) {
        // Serial.println("Mesin 1");
        // buttonAction = "AM";
        buttonValue = 1;
        // sendAndon = 1;
      } else if (stateMesin == LOW) {
        // Serial.println("Mesin 0");
        // buttonAction = "AM";
        buttonValue = 0;
      }
      andonType = ANDON_MAINTENANCE;
      sendAndon = 1;
      laststateMesin = stateMesin;
      counterMesin = 0;
    }
  }
  if (stateMesin == laststateMesin) {
    counterMesin = 0;
  }

  // Send data as Tool
  if (stateMold != laststateMold) {
    counterMold++;
    if (counterMold == 1) {
      moldLastDebounce = getMillis();
    }
    if (millis() - moldLastDebounce >= 3000) {
      if (stateMold == HIGH) {
        // Serial.println("Mold 1");
        // buttonAction = "AT";
        buttonValue = 1;
        // sendAndon = 1;
      } else if (stateMold == LOW) {
        // Serial.println("Mold 0");
        // buttonAction = "AT";
        buttonValue = 0;
      }
      andonType = ANDON_TOOL;
      sendAndon = 1;
      laststateMold = stateMold;
      counterMold = 0;
    }
  }
  if (stateMold == laststateMold) {
    counterMold = 0;
  }

  if (stateLeader == HIGH || stateQc == HIGH || stateMesin == HIGH || stateMold == HIGH) {
    unsigned long blinkMillis = millis();
    if (blinkMillis - previousBlink >= 500) {
      // Save the last time the LED was toggled
      previousBlink = blinkMillis;

      // If the LED is off, turn it on, and vice versa
      ledState = !ledState;
      digitalWrite(LED_BUILTIN, ledState);
    }
  } else {
    digitalWrite(LED_BUILTIN, LOW);
    ledState = LOW;  // Reset the LED state
  }

  if (sendAndon == 1) {
    unixTime = time(nullptr);
    needResponse = true;
    sendAndonData();
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
          deviceStatus = STATUS_IDLE;
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
  WiFiClientSecure updateClient;
  updateClient.setInsecure();
  // updateClient.setBufferSizes(512, 512);

  HTTPClient httpClient;
  httpClient.begin(updateClient, versionUrl);

  Serial.println("[SYSTEM] Checking for new Firmware..");

  int httpCode = httpClient.GET();
  if (httpCode == HTTP_CODE_OK) {
    String latestVersion = httpClient.getString();
    latestVersion.trim();
    Serial.printf("[SYSTEM] Current version: %s\n", versionNum);
    Serial.printf("[SYSTEM] Latest version: %s\n", latestVersion);
    httpClient.end();
    if (versionNum != latestVersion) {
      Serial.println("[SYSTEM] New firmware available. Updating...");
      t_httpUpdate_return ret = ESPhttpUpdate.update(updateClient, firmwareUrl);

      if (ret != HTTP_UPDATE_OK) {
        Serial.printf("[SYSTEM] Update failed (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      } else {
        Serial.println("[SYSTEM] Firmware update completed.");
      }
    } else {
      Serial.println("[SYSTEM] Firmware is up to date.");
      // To prevent http requesting the rest of the day
      is_update = true;
    }
  } else {
    Serial.printf("[SYSTEM] Failed to check for updates (%d)\n", httpCode);
  }
}

void publishData(const char* payload, RequestType type = REQ_NEW) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload);

  unsigned long time = doc["time"];
  if (!WiFi.isConnected() || wiFiConnected == false || !mqttClient.connected() || mqttConnected == false) {
    Serial.println("[SYSTEM] Publish gagal, koneksi tidak terhubung");
    if (needResponse == true){
      Serial.println("[SYSTEM] Menyimpan pesan");
      saveDataToFile(payload);
    }
    return;
  }

  if (mqttClient.publish(pubTopic, payload)) {
    Serial.print("[MQTT] Publish: ");
    Serial.println(payload);
    if (needResponse == true){
      addRequest(time, payload, type);
    }
  } else {
    Serial.println("[MQTT] Publish gagal");
    if (needResponse == true){
      Serial.println("[MQTT] Menyimpan pesan");
      saveDataToFile(payload);
    }
  }
}

//===============================================================================

void readDataFromFile() {

  char line[256];

  // ======================================================
  // =============== SHOOT FILE ===========================
  // ======================================================
  File shootFile = SPIFFS.open("/down.txt", "r");
  if (shootFile) {

    Serial.println("read shoot");

    DynamicJsonDocument shootDoc(12000);   // gunakan heap
    shootDoc["action"] = "shoot-file";
    shootDoc["id"] = deviceId;
    shootDoc["time"] = time(nullptr);
    JsonArray shootArray = shootDoc.createNestedArray("data");

    while (shootFile.available()) {

      int len = shootFile.readBytesUntil('\n', line, sizeof(line)-1);
      line[len] = '\0';
      if (len <= 0) continue;

      char *commaPos = strchr(line, ',');
      if (!commaPos) continue;

      *commaPos = '\0';

      int cycleTime = atoi(line);
      unsigned long time = strtoul(commaPos + 1, NULL, 10);

      JsonObject obj = shootArray.createNestedObject();
      obj["id"]        = deviceId;
      obj["cycletime"] = cycleTime;
      obj["ip"]        = ipAddress;
      obj["time"]      = time;
    }

    shootFile.close();

    size_t size = measureJson(shootDoc) + 20;
    char *buffer = (char*) malloc(size);
    
    if (buffer) {
      serializeJson(shootDoc, buffer, size);
      Serial.println("kirimshootfile");
      publishData(buffer, REQ_SHOOT_FILE);
      free(buffer);
    }
  } else {
    Serial.println("Tidak ada shootfile");
  }



  // ======================================================
  // =============== ANDON FILE ===========================
  // ======================================================
  // File andonFile = SPIFFS.open("/andon.txt", "r");
  // if (andonFile) {

  //   Serial.println("read andon");

  //   DynamicJsonDocument andonDoc(12000);
  //   andonDoc["action"] = "andon-file";
  //   andonDoc["id"] = deviceId;
  //   andonDoc["time"] = time(nullptr);
  //   JsonArray andonArray = andonDoc.createNestedArray("data");

  //   while (andonFile.available()) {

  //     int len = andonFile.readBytesUntil('\n', line, sizeof(line)-1);
  //     line[len] = '\0';
  //     if (len <= 0) continue;

  //     DynamicJsonDocument temp(1024);

  //     DeserializationError err = deserializeJson(temp, line);
  //     if (err) continue;

  //     andonArray.add(temp.as<JsonObject>());
  //   }

  //   andonFile.close();

  //   size_t size = measureJson(andonDoc) + 20;
  //   char *buffer = (char*) malloc(size);

  //   if (buffer) {
  //     serializeJson(andonDoc, buffer, size);
  //     Serial.println("kirimandonfile");
  //     publishData(buffer, REQ_ANDON_FILE);
  //     free(buffer);
  //   }
  // } else {
  //   Serial.println("Tidak ada andonfile");
  // }

  savingDataToFile = false;
}

//===============================================================================

void saveDataToFile(const char* payload) {
  StaticJsonDocument<256> doc;
  
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("Gagal parse JSON: ");
    Serial.println(error.f_str());
    return;
  }

  const char* action = doc["action"] | "";

  // ======================================================
  // =============== SHOOT FILE ===========================
  // ======================================================
  if (strcmp(action, "shoot") == 0) {

    int cyc = doc["cyc"].as<int>();

    // time HARUS unsigned long
    unsigned long time = doc["time"].as<unsigned long>();

    File shootFile = SPIFFS.open("/down.txt", "a");
    if (!shootFile) {
      Serial.println("Failed to open file for writing");
      return;
    }

    // simpan data
    shootFile.printf("%d, %lu\n", cyc, time);

    shootFile.close();
  }

  // ======================================================
  // =============== ANDON FILE ===========================
  // ======================================================
  // else {

  //   File andonFile = SPIFFS.open("/andon.txt", "a");
  //   if (!andonFile) {
  //     Serial.println("Failed to open file for writing");
  //     return;
  //   }

  //   // Hindari karakter CR/LF ganda, simpan bersih
  //   andonFile.print(payload);
  //   andonFile.print("\n");

  //   andonFile.close();
  // }

  // set flag
  savingDataToFile = true;
}


// JSON DATA
void sendShootData() {
  StaticJsonDocument<256> doc;

  doc["action"] = "shoot";
  doc["id"] = deviceId;
  doc["name"] = deviceName;
  doc["cla"] = staCla;
  doc["inj"] = staInj;
  doc["cyc"] = cycleTime;
  doc["shoot"] = shoot;
  doc["ip"] = ipAddress;
  doc["time"] = unixTime;
  doc["startTime"] = startTime;
  doc["version"] = versionNum;
  
  char jsonBuffer[256];
  size_t len = serializeJson(doc, jsonBuffer);
  publishData(jsonBuffer);
  needResponse = false;
  sendData = false;
}

void sendAndonData() {
  StaticJsonDocument<256> doc;

  doc["action"] = andonAction[andonType];
  doc["id"] = deviceId;
  doc["button"] = buttonValue ? 1 : 0;
  doc["ip"] = ipAddress;
  doc["time"] = unixTime;

  char jsonBuffer[256];
  size_t len = serializeJson(doc, jsonBuffer);

  publishData(jsonBuffer);
  needResponse = false;
  sendAndon = 0;
}

void sendStatusData() {
  StaticJsonDocument<256> doc;

  doc["action"] = "status";
  doc["id"] = deviceId;
  doc["name"] = deviceName;
  doc["value"] = statusText[deviceStatus];
  doc["time"] = time(nullptr);
  doc["version"] = versionNum;
  doc["ssid"] = WiFi.SSID();

  char jsonBuffer[256];
  size_t len = serializeJson(doc, jsonBuffer);

  mqttClient.publish(pubTopic, jsonBuffer, true);
}

void addRequest(unsigned long time, const char* payload, RequestType type) {
  for (int i = 0; i < MAX_REQUESTS; i++) {
    if (!requests[i].waiting) {
      requests[i].id = time;
      requests[i].payload = String(payload);
      requests[i].timeSent = millis();
      requests[i].waiting = true;
      requests[i].type = type;
      return;
    }
  }
  Serial.println("Request queue penuh!");
}

void clearRequest(unsigned long id) {
  for (int i = 0; i < MAX_REQUESTS; i++) {
    if (requests[i].waiting && requests[i].id == id) {
      requests[i].waiting = false;
      return;
    }
  }
}

void checkRequestTimeouts() {
  for (int i = 0; i < MAX_REQUESTS; i++) {
    if (requests[i].waiting && millis() - requests[i].timeSent > REQUEST_TIMEOUT) {
      Serial.printf("Request ID %d timeout!\n", requests[i].id);
      if (requests[i].type == REQ_NEW) {
        saveDataToFile(requests[i].payload.c_str());
      }
      clearRequest(requests[i].id);
    }
  }
}

void loop() {
  // Calling Cycletime capture proses
  monitorCycleTime();

  checkRequestTimeouts();

  // Checking WiFi connection
  if (!WiFi.isConnected()) {
    // Disconnect from Wifi then try to reconnect
    if (wiFiConnected == true) {
      Serial.println("[SYSTEM] WiFi disconnected, reconnecting...");
      wifiMillis = millis();
      // WiFi.disconnect();
      WiFi.begin();
      digitalWrite(LED_BUILTIN, HIGH);
      wiFiConnected = false;
      mqttConnected = false;
    }

    if ((millis() - wifiMillis) / 1000 >= 300) {
        Serial.println("[SYSTEM] WiFi disconnected too long");
        Serial.println("[SYSTEM] Restarting ESP...");

        delay(500);
        ESP.restart();
    }
  } else {
    // WiFi succesfully connected/reconnected
    if (wiFiConnected == false) {
      wifiDownSecond = (millis() - wifiMillis) / 1000;  // Wifi Downtime in second
      wifiDownMinute = wifiDownSecond / 60;             // Wifi Downtime in minute
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
      ipAddress = WiFi.localIP().toString();
      Serial.println("[SYSTEM] Connected to WiFi");
      wiFiConnected = true;
    }

    if (!mqttClient.connected()) {
      digitalWrite(LED_BUILTIN, HIGH);
      reconnectMQTT();
    }

    mqttClient.loop();
    server.handleClient();
    ArduinoOTA.handle();

    // Reading File if MQTT is connected and have saved file
    if (savingDataToFile && mqttConnected){
      Serial.println("bacafile");
      readDataFromFile();
    }

    unsigned long currentStatusSeconds = millis() / 1000;
    if (currentStatusSeconds - previousStatusSeconds >= 30) {
      previousStatusSeconds = currentStatusSeconds;
      sendStatusData();
    }

    unsigned long idleDuration = millis() - startIdle;
    unsigned long idleSeconds = idleDuration / 1000;
    if (idleSeconds >= timeToIdle) {
      deviceStatus = STATUS_IDLE;
    }

    // Calling Andon proses and make delay for stabilization button input
    unsigned long buttonMillis = millis();
    if (buttonMillis - previousButtonMillis >= 250) {
      previousButtonMillis = buttonMillis;
      buttonAndon();
    }

    // Create Interval to run Check Update Firmware
    unsigned long currentCheckSeconds = millis() / 1000;
    if (currentCheckSeconds - previousCheckSeconds >= 300) {
      previousCheckSeconds = currentCheckSeconds;
      timeToCheck();
    }

  }

  // Send data via websocket as JSON format
  if (sendData == true) {
    sendShootData();
  }
}

/*
Update:
Mengubah protokol komunikasi ke MQTT
Tambah proses Request-Response untuk kehandalan
*/