bool debug = false;  //Отображение отладочной информации в серийный порт

#define DHTPIN D7             // Пин подключения датчика влажности и температуры
#define RELAYPIN D8           // Пин подключения реле
#define LEDPIN D9             // Пин подключения светодиода
#define DHTTYPE DHT11         // DHT 22  (AM2302) Тип датчика влажности
#define REPORT_INTERVAL 11000 // Интервал отправки данных брокеру
#define BUFFER_SIZE 100       // Размер буфера для получения сообщения 
#define PinPhoto A0           // Аналоговый вход

const char* ssid = "Password";        //Имя WIFI сети
const char* password = "bdcPVN5786";  //Пароль WIFI
const char* device1 = "Switch1";      //Имя управляемого устройства №1
const char* device2 = "Switch2";      //Имя управляемого устройства №2
const char* device3 = "Switch3";      //Имя управляемого устройства №3

String topic = "DHT";                   //Топик для отправки
String debug_topic = "DHTdebug";        //Топик отладочной информации
String sub_topic = "homebridge/to/set"; //Топик подписки
char* hellotopic = "hello_topic";       //Топик приветствия
char message_buff[2048];                //Размер буфера для принятого сообщения

IPAddress mqtt_server(192, 168, 1, 31);     //Первый сервер MQTT
IPAddress mqtt_server2(95, 174, 107, 100);  //Второй сервер MQTT
//String mqtt_server = "iot.eff-t.ru";
int mqtt_port = 1883;                       //Порт MQTT сервера

unsigned long currentTime;    //Переменная для преобразования времени работы модуля
int err_conn = 0;             //Счетчик ошибок подключения к MQTT серверу

float oldH ;        //Предыдущее значение влажности
float oldT ;        //Предыдущее значение температуры
String clientName;  //Имя клиента
