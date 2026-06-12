#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// ---------- WiFi ----------
const char* ssid     = "Wokwi-GUEST";
const char* password = "";

// ---------- Backend ----------
const String serverName = "https://ecosmart-backend-mufu.onrender.com/api/mediciones";

// ---------- Sensores ----------
struct Sensor {
  uint8_t trig;
  uint8_t echo;
  int     tachoId;
};

Sensor sensores[] = {
  {4,  2,  1},
  {13, 12, 2},
  {14, 27, 3},
  {32, 33, 4},
  {25, 26, 5}
};
const int numSensores = 5;

const float FACTOR  = 59.88;
const int MUESTRAS  = 7;

// ---------- Lectura individual ----------
float medirDistancia(uint8_t trigPin, uint8_t echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duracion = pulseIn(echoPin, HIGH, 30000);
  if (duracion == 0) return -1.0;
  return duracion / FACTOR;
}

// ---------- Mediana ----------
float medirFiable(uint8_t trigPin, uint8_t echoPin) {
  float valores[MUESTRAS];
  int validos = 0;

  for (int i = 0; i < MUESTRAS; i++) {
    float d = medirDistancia(trigPin, echoPin);
    if (d >= 0) valores[validos++] = d;
    delay(20);
  }

  if (validos == 0) return -1.0;

  for (int i = 0; i < validos - 1; i++)
    for (int j = 0; j < validos - i - 1; j++)
      if (valores[j] > valores[j + 1]) {
        float tmp = valores[j];
        valores[j] = valores[j + 1];
        valores[j + 1] = tmp;
      }

  return valores[validos / 2];
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);

  for (int i = 0; i < numSensores; i++) {
    pinMode(sensores[i].trig, OUTPUT);
    pinMode(sensores[i].echo, INPUT);
    digitalWrite(sensores[i].trig, LOW);
  }

  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
}

// ---------- Loop ----------
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado, reconectando...");
    WiFi.begin(ssid, password);
    delay(3000);
    return;
  }

  JsonDocument doc;
  JsonArray lecturas = doc["lecturas"].to<JsonArray>();

  Serial.println("--- Tomando lecturas ---");
  for (int i = 0; i < numSensores; i++) {
    float dist = medirFiable(sensores[i].trig, sensores[i].echo);

    JsonObject tacho = lecturas.add<JsonObject>();
    tacho["tacho_id"]  = sensores[i].tachoId;
    tacho["distancia"] = (dist >= 0) ? (int)ceil(dist) : -1;

    if (dist >= 0)
      Serial.printf("Tacho %d: %.1f cm\n", sensores[i].tachoId, dist);
    else
      Serial.printf("Tacho %d: sin eco\n", sensores[i].tachoId);
  }

  String body;
  serializeJson(doc, body);
  Serial.println("JSON: " + body);

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, serverName);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(15000);

  int code = http.POST(body);
  if (code > 0) {
    Serial.printf("HTTP %d\n", code);
    Serial.println(http.getString());
  } else {
    Serial.printf("Error HTTP: %s\n", http.errorToString(code).c_str());
  }

  http.end();
  Serial.println("--- Esperando 20 s ---");
  delay(20000);
}