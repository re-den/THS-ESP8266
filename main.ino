#include <MQTT.h>
#include <PubSubClient.h>
#include <PubSubClient_JSON.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include <ESP8266WiFi.h>


bool debug = false;  //Display log message if True

const char* ssid = "Password";
const char* password = "bdcPVN5786";
const char* device1 = "Switch1";
const char* device2 = "Switch2";
const char* device3 = "Switch3";
String topic = "/sensors/dht";
String debug_topic = "DHTdebug";
String sub_topic = "homebridge/to/set";
IPAddress mqtt_server(192, 168, 1, 31);
IPAddress mqtt_server2(95, 174, 107, 100);
//String mqtt_server = "zbx.eff-t.ru";
int mqtt_port = 1883;
char* hellotopic = "hello_topic";
char message_buff[2048];
unsigned long currentTime; unsigned long currentUtimeReport;
int err_conn = 0;

#define DHTPIN D7     // what pin we're connected to
#define RELAYPIN D8
#define LEDPIN D9
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define REPORT_INTERVAL 5000 // in millisec
#define UPTIME_REPORT_INTERVAL 60000 // in millisec
#define BUFFER_SIZE 100
#define PinPhoto A0 // Аналоговый вход

float oldH ;
float oldT ;
String clientName;

void callback(const MQTT::Publish& pub);
String macToStr(const uint8_t* mac);

DHT dht(DHTPIN, DHTTYPE);
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void setup() {
  Serial.begin(115200);
  Serial.println("Start system!");
  delay(20);

  Serial.println("Initialization Relay PIN");
  pinMode(RELAYPIN, OUTPUT);
  digitalWrite(RELAYPIN, HIGH);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  clientName += "esp8266-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  //clientName += "-";
  //clientName += String(micros() & 0xff, 16);

  Serial.print("Connecting to ");
  Serial.print(mqtt_server);
  Serial.print(" as ");
  Serial.println(clientName);
  //Подключаемся к MQTT серверу
  while (!client.connected()) {
    mqtt_connect();
  }
  dht.begin();
  oldH = -1;
  oldT = -1;
}

void loop() {

  if (client.connected()) {
    if (millis() - currentTime > REPORT_INTERVAL) // Если время контроллера millis, больше переменной на REPORT_INTERVAL, то запускаем условие if
    {
      currentTime = millis();        // Приравниваем переменную текущего времени к времени контроллера, чтобы через REPORT_INTERVAL опять сработал наш цикл.
      sendTemperature();
      //Serial.println("Отправка прошла в " + uptime());
    }
/*
    if (millis() - currentUtimeReport > UPTIME_REPORT_INTERVAL) // Если время контроллера millis, больше переменной на UPTIME_REPORT_INTERVAL, то запускаем условие if
    {
      currentUtimeReport = millis();        // Приравниваем переменную текущего времени к времени контроллера, чтобы через UPTIME_REPORT_INTERVAL опять сработал наш цикл.
      String payload = "{\"id\":";
      payload += clientName;
      payload += ",\"uptime\":";
      payload += uptime();
      payload += "\"}";
      if (!client.publish(topic, (char*) payload.c_str())) {
        Serial.println("Publish failed");
      }
    }*/
    client.loop();
  }
  else {
    while (!client.connected()) {
      mqtt_connect();
    }
  }
}


void sendTemperature() {

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  String payload = "{\"id\":\"";
  payload += clientName;
  payload += "\",\"uptime\":\"";
  payload += uptime();
  payload += "\",\"humi\":\"";
  payload += h;
  payload += "\",\"temp\":\"";
  payload += t;
  payload += "\",\"light\":\"";
  payload += analogRead(PinPhoto);
  payload += "\"}";

  if (t != oldT || h != oldH )
  {
    if (client.connected()) {
      Serial.print("Client connected OK! ");

      if (client.publish(topic, (char*) payload.c_str())) {
        Serial.print("Publish ");
        Serial.print(topic);
        Serial.print(" ");
        Serial.println(payload.c_str());
      }
      else {
        Serial.println("Publish failed");
      }
    }
    oldT = t;
    oldH = h;
  }
  else {
    Serial.println("Нет новых данных для отправки");
  }

  if (debug) {
    String debug_payload = "{\"Client\":\"";
    debug_payload += clientName;
    debug_payload += "\",\"Millis\": \"";
    debug_payload += uptime();
    debug_payload += "\"}";
    if (client.publish(debug_topic, (char*) debug_payload.c_str())) {
      Serial.println(debug_payload);
    }
    else
    {
      Serial.println("Publish debug - ERROR");
    }
  }
}

String uptime() {
  long currentSeconds = millis() / 1000;
  int hours = currentSeconds / 3600;
  int minutes = (currentSeconds / 60) % 60;
  int seconds = currentSeconds % 60;
  String report = ((hours < 10) ? "0" + String(hours) : String(hours));
  report += ":";
  report += ((minutes < 10) ? "0" + String(minutes) : String(minutes));
  report += ":";
  report += ((seconds < 10) ? "0" + String(seconds) : String(seconds));
  return report;
}

void callback(const MQTT::Publish & pub) {


  String payload = pub.payload_string();
  if (payload != "") {
    if (String(pub.topic()) == sub_topic) {

      DynamicJsonDocument doc(2048);

      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }

      const char* device_name = doc["name"]; // "Switch1"
      const char* service_name = doc["service_name"]; // "Switch 1"
      const char* service = doc["service"]; // "Outlet"
      const char* characteristic = doc["characteristic"]; // "On"
      bool value = doc["value"]; // true|false

      if (strcmp (device_name, device1) == 0) {
        if (value) {
          digitalWrite(RELAYPIN, LOW);
        }
        else {
          digitalWrite(RELAYPIN, HIGH);
        }
        Serial.print(device_name);
        Serial.print(" - is ");
        Serial.println(value);
      }
      if (strcmp (device_name, device2) == 0) {
        // код действия для Switch2


      }

      if (strcmp (device_name, device3) == 0) {
        debug = value;
        digitalWrite(LEDPIN, ((value == false) ? HIGH : LOW));
        Serial.println((value == true) ? "ОТЛАДКА ВКЛЮЧЕНА" : "ОТЛАДКА ОТКЛЮЧЕНА");
      }
    }
  }
  else
  {
    if (debug)
      Serial.println("Empty payload");
  }
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}