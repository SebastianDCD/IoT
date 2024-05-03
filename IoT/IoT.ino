#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "certificate.h"

const char* ssid = "Laptop Sebastian";
const char* password = "A123456789";


//const char* ssid = "IoT";
//const char* password = "76041082";


const char* mqtt_server = "d48a7e9bcba14f148f3cf3e0ce4f92e2.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "ESP32";
const char* mqtt_password = "A12344321a";

const char* LED1_TOPIC = "LED1";
const char* DHT_TOPIC = "DHT11";

const int ledPin = 2;
const int inputPin = 36;

#define DHTPIN 4       // Pin del sensor DHT11
#define DHTTYPE DHT11  // Tipo del sensor DHT

DHT dht(DHTPIN, DHTTYPE);

WiFiClientSecure espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Conectado a la red WiFi");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido en el canal: ");
  Serial.println(topic);

  if (strcmp(topic, LED1_TOPIC) == 0) {
    String msg = "";
    for (int i = 0; i < length; i++) {
      msg += (char)payload[i];
    }

    if (msg.equals("on")) {
      digitalWrite(ledPin, HIGH);
      Serial.println("LED encendido");
    } else {
      digitalWrite(ledPin, LOW);
      Serial.println("LED apagado");
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      Serial.println("Conectado");
      client.subscribe(LED1_TOPIC);
    } else {
      Serial.print("Falló, rc=");
      Serial.print(client.state());
      Serial.println(" Intentando nuevamente en 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
  dht.begin();
  setup_wifi();
  espClient.setCACert(root_ca);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  static unsigned long lastMillis = 0;
  const unsigned long interval = 5000;  // 5 segundos

  unsigned long currentMillis = millis();
  if (currentMillis - lastMillis >= interval) {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Fallo al leer el sensor DHT!");
      return;
    }

    Serial.print("Humedad: ");
    Serial.print(humidity);
    Serial.print(" %\t Temperatura: ");
    Serial.println(temperature);

    // Enviar datos al servidor MQTT
    char humidityStr[6];
    char temperatureStr[6];
    dtostrf(humidity, 4, 2, humidityStr);
    dtostrf(temperature, 4, 2, temperatureStr);

    client.publish((String(DHT_TOPIC) + "/humidity").c_str(), humidityStr, true);
    client.publish((String(DHT_TOPIC) + "/temperature").c_str(), temperatureStr, true);


    lastMillis = currentMillis;
  }
}