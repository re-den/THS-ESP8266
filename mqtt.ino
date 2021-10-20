// подключение к MQTT серверу
void mqtt_connect() { // подключение к MQTT серверу
  client.set_server(mqtt_server, mqtt_port);
  if (client.connect(MQTT::Connect(clientName).set_auth("denisr", "bdcPVN5786"))) {
    subscribeclient();
  }
  else {
    err_conn++;
    Serial.println("MQTT connect failed");
    Serial.println("Will reset and try again...");
    Serial.println(err_conn);
    Serial.println(millis());
    client.set_server(mqtt_server2, mqtt_port);
    if (client.connect(MQTT::Connect(clientName).set_auth("denisr", "bdcPVN5786"))) {
      Serial.println("MQTT connect to ");
      Serial.println(mqtt_server2);      
    }
    else
      abort();
  }
}

//Подписка на топики отчет о подключении
void subscribeclient() {
  client.set_callback(callback);
  Serial.println("Connected to MQTT server");
  // подписываемся на топики
  if (client.subscribe(sub_topic)) {
    Serial.print("Subscribe to ");
    Serial.print(sub_topic);
    Serial.println(" - ok");
  }
  else {
    Serial.print("Subscribe to ");
    Serial.print(sub_topic);
    Serial.println(" - ERROR");
  }

  if (client.publish(hellotopic, "hello from ESP8266")) {
    Serial.println("Start publish ok");
  }
  else {
    Serial.println("Start publish failed");
  }
}
