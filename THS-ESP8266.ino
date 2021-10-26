#include <MQTT.h>
#include <PubSubClient.h>
#include <PubSubClient_JSON.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include <ESP8266WiFi.h>
#include <config.h>

//------------------------------------------------------------------------

bool debug = false;  //Отображение отладочной информации в серийный порт

#define DHTPIN D7             // Пин подключения датчика влажности и температуры
#define RELAYPIN D8           // Пин подключения реле
#define LEDPIN D9             // Пин подключения светодиода
#define DHTTYPE DHT11         // DHT 22  (AM2302) Тип датчика влажности
#define REPORT_INTERVAL 3000 // Интервал отправки данных брокеру
#define BUFFER_SIZE 200       // Размер буфера для получения сообщения 
#define PinPhoto A0           // Аналоговый вход

const char* ssid = "Password";        //Имя WIFI сети
const char* password = "bdcPVN5786";  //Пароль WIFI
const char* device1 = "Switch1";      //Имя управляемого устройства №1
const char* device2 = "Switch2";      //Имя управляемого устройства №2
const char* device3 = "Switch3";      //Имя управляемого устройства №3

String topic = "/sensors/dht";                   //Топик для отправки
String debug_topic = "/debug";        //Топик отладочной информации
String sub_topic = "/homebridge/to/set"; //Топик подписки
char* hellotopic = "/hello_topic";       //Топик приветствия
char message_buff[2048];                //Размер буфера для принятого сообщения

IPAddress mqtt_server(192, 168, 1, 31);     //Первый сервер MQTT
IPAddress mqtt_server2(95, 174, 107, 100);  //Второй сервер MQTT
//String mqtt_server = "iot.eff-t.ru";
int mqtt_port = 1883;                       //Порт MQTT сервера

unsigned long currentTime;    //Переменная для преобразования времени работы модуля
unsigned long currentUtimeReport;
int err_conn = 0;             //Счетчик ошибок подключения к MQTT серверу
int count=0;
int avg_count=1;

float oldH;        //Предыдущее значение влажности
float oldT;        //Предыдущее значение температуры
float temp_avg;    //Среднее значение температуры
float humy_avg;    //Среднее значение влажности
String clientName;  //Имя клиента
//========================================================================

//void callback(const MQTT::Publish& pub);
//String macToStr(const uint8_t* mac);

DHT dht(DHTPIN, DHTTYPE);
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void setup() {
  Serial.begin(115200);
  
  Serial.println();
  Serial.println();
  Serial.println("Start system!");
  delay(20);

  Serial.println("Initialization Relay PIN");
  pinMode(RELAYPIN, OUTPUT);
  digitalWrite(RELAYPIN, HIGH);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);

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

void sendTemperature() {

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (avg_count!=11){
    temp_avg+=t;
    humy_avg+=h;
    if (debug){
    Serial.print(avg_count);
    Serial.print(" измерений: ");
    Serial.print(t);
    Serial.print(" - ");
    Serial.print(temp_avg);
    Serial.print(" C; ");
    Serial.print(h);
    Serial.print(" - ");
    Serial.print(humy_avg);
    Serial.println(" %");
    }
    avg_count++;
  }  
  else{
    avg_count-=1;
    temp_avg/=avg_count;
    humy_avg/=avg_count;
    Serial.print("Средние дначени за 10 измерений: ");
    Serial.print(temp_avg);
    Serial.print(" C; ");
    Serial.print(humy_avg);
    Serial.println(" %");

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    Serial.println(t);
    Serial.println(h);
    while(count<7){
      digitalWrite(LEDPIN, LOW);
      delay(40);
      digitalWrite(LEDPIN, HIGH);
      delay(50);
      count++;
    }
    count=0;
    return;
  }
  String payload = "{\"id\":\"";
  payload += clientName;
  payload += "\",\"uptime\":\"";
  payload += uptime();
  payload += "\",\"humi\":\"";
  payload += humy_avg;
  payload += "\",\"temp\":\"";
  payload += temp_avg;
  payload += "\",\"light\":\"";
  payload += analogRead(PinPhoto);
  payload += "\"}";

  if (temp_avg != oldT || humy_avg != oldH )
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
    oldT = temp_avg;
    oldH = humy_avg;
  }
  else {
    Serial.println("нет новых данных для отправки");
  }

    temp_avg=0;
    humy_avg=0;
    avg_count=1;
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
