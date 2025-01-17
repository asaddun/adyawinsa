#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsClient.h>
#include <ArduinoOTA.h>

const char *ssid = "API-HOTSPOT";
const char *password = "nevergiveup";
const char *authToken = "66ae1553a20eb59bb1d4db65f4c4c2d1";
const char *privateKey = "66ae1553a20eb59bb1d4db65f4c4c2d1";

ESP8266WebServer server(80);  // Menggunakan port 80 untuk server HTTP
WebSocketsClient webSocket;

int lockerDoor = 33;
String ipAddress, storedIpAddress;
const char* ws_host = "192.168.3.245";
int ws_port = 1880;
const char* ws_url = "/locker";
bool isWebsocketConnected = false;
unsigned long previousCheckSeconds;

const int lampPins[] = {D0, D1, D2, D3, D5, D6, D7, D8};  // Array pin untuk mengendalikan lampu
const int numLampPins = sizeof(lampPins) / sizeof(lampPins[0]);  // Jumlah pin lampu

// WEBSOCKET CLIENT HANDLING
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("Disconnected!\n");
      break;
    case WStype_CONNECTED:
      {
        isWebsocketConnected = true;
        Serial.println("Connected to Websocket");

        // send message to server when Connected
        // Serial.println("SENT: Connected");
        // webSocket.sendTXT("Connected");
      }
      break;
    case WStype_TEXT:
      Serial.printf("RESPONSE: %s\n", payload);
      break;
    case WStype_BIN:
      Serial.printf("get binary length: %u\n", length);
      hexdump(payload, length);
      break;
    case WStype_PING:
      // pong will be send automatically
      Serial.printf("get ping\n");
      break;
    case WStype_PONG:
      // answer to a ping we send
      Serial.printf("get pong\n");
      break;
  }
}
// END OF WEBSOCKET CLIENT

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("");

  // Inisialisasi semua pin lampu sebagai output dan matikan lampu saat startup
  for (int i = 0; i < numLampPins; i++) {
    pinMode(lampPins[i], OUTPUT);
    // digitalWrite(lampPins[i], LOW);
  }

  delay(1000);

  for (int i = 0; i < numLampPins; i++) {
    // pinMode(lampPins[i], OUTPUT);
    digitalWrite(lampPins[i], LOW);
  }

  // Mencoba untuk terhubung ke WiFi dengan SSID dan kata sandi yang diberikan
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  ipAddress = WiFi.localIP().toString();
  storedIpAddress = ipAddress;

  for (int i = 0; i < numLampPins; i++) {
    String route = "/locker/" + String(i + 1);  // Route berdasarkan nomor pin (mulai dari 1)
    server.on(route.c_str(), HTTP_POST, [i]() {
      String receivedToken = server.arg("token");

      if (receivedToken == authToken) {
        digitalWrite(lampPins[i], HIGH);  // Nyalakan lampu
        delay(3000);  // Tahan selama 3 detik
        digitalWrite(lampPins[i], LOW);  // Matikan lampu
        server.send(200, "application/json", "{\"success\": true}");
      }else{
        server.send(401, "application/json", "{\"success\": false}");
      }

    });
  }
  server.begin();

  // WEBSOCKET CLIENT SETUP
  Serial.println("Trying connect to Websocket..");

  webSocket.begin(ws_host, ws_port, ws_url);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  while (!isWebsocketConnected) {
    webSocket.loop(); // Attempt connection
    delay(200);       // Small delay to avoid spamming
  }

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

  pinMode(LED_BUILTIN, OUTPUT); // Tersambung dengan D4
  sendIdentity(); // Kirim identitas di awal
}

void checkIP(){
  ipAddress = WiFi.localIP().toString();
  if (ipAddress != storedIpAddress){
    storedIpAddress = ipAddress;
    sendIdentity();
  } else {
    Serial.println("Check IP PASS");
  }
}

void sendIdentity(){
  String json = "{";
  json += "\"locker\":\"";
  json += lockerDoor;
  json += "\",\"ip\":\"";
  json += ipAddress;
  json += "\",\"key\":\"";
  json += privateKey;
  json += "\"";
  json += "}";
  Serial.println(json);
  webSocket.sendTXT(json);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();  // Handle permintaan klien
    webSocket.setReconnectInterval(5000);
    webSocket.loop();
    ArduinoOTA.handle();
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    WiFi.begin();
    delay(250);
    if (WiFi.status() != WL_CONNECTED) {
      delay(250);
      Serial.print(".");
    }
    digitalWrite(LED_BUILTIN, HIGH);
  }
  unsigned long currentCheckSeconds = millis() / 1000;
  if (currentCheckSeconds - previousCheckSeconds >= 30) {
    previousCheckSeconds = currentCheckSeconds;
    checkIP();
  }
  delay(250);
}

/*
D0: -: HIGH saat Boot
D1: OK
D2: OK
D3: -: HIGH saat Boot
D4: -: Tidak bisa upload code jika terhubung ke Relay
D5: OK
D6: OK
D7: OK
D8: OK
*/
