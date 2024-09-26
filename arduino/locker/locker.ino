#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char *ssid = "API-HOTSPOT";
const char *password = "nevergiveup";
const char *authToken = "66ae1553a20eb59bb1d4db65f4c4c2d1";
IPAddress localIp(192,168,10,12);
IPAddress gateaway(192,168,10,2);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server(80);  // Menggunakan port 80 untuk server HTTP

const int lampPins[] = {D0, D1, D2, D3, D5, D6, D7, D8};  // Array pin untuk mengendalikan lampu
const int numLampPins = sizeof(lampPins) / sizeof(lampPins[0]);  // Jumlah pin lampu

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("");

  // Inisialisasi semua pin lampu sebagai output dan matikan lampu saat startup
  for (int i = 0; i < numLampPins; i++) {
    pinMode(lampPins[i], OUTPUT);
    // digitalWrite(lampPins[i], LOW);
  }
  digitalWrite(D3, HIGH);
  // digitalWrite(D4, HIGH);

  delay(1000);

  for (int i = 0; i < numLampPins; i++) {
    // pinMode(lampPins[i], OUTPUT);
    digitalWrite(lampPins[i], LOW);
  }

  // Mencoba untuk terhubung ke WiFi dengan SSID dan kata sandi yang diberikan
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.config(localIp, gateaway, subnet);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

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

  pinMode(LED_BUILTIN, OUTPUT);

}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();  // Handle permintaan klien
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
    WiFi.begin(ssid, password);
  }
}
