#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>

//Définition du type de DHT
#define DHTTYPE DHT22

//Définition des broches de l'ESP32
#define DHTPIN 15           //DHT22
#define LDRPIN 34           //LDR (Light Dependent Resistor)
#define MQ135PIN 32        //MQ-135 (Capteur de qualité de l'air)
#define LEDPIN 2          //LED pour la qualité de l'air

//Définition de l'activation des capteurs
bool dhtEnabled = true;
bool bmpEnabled = true;
bool ldrEnabled = true;
bool mq135Enabled = true;


// Liste des réseaux connus & adresse ip du serveur associée
struct WifiCredential {
  const char* ssid;
  const char* password;
  const char* ip;
};

WifiCredential knownNetworks[] = {
  {"GalaxyA7164D1", "saaw6584", "192.168.215.32"},
  {"Proximus-Home-5088", "wjpc9nam4239x", "192.168.1.30"},
  {"VOO-HJ47MA1", "hZZRJt3XxJ7qKfxNXh", "192.168.0.186"}
};

const char* ip = nullptr;

//Initialisation des objets
DHT dht(DHTPIN, DHTTYPE);       //DHT22 (Tempéature, humidité et indice de chaleur)
Adafruit_BMP280 bmp;            //BMP280 ((Température), pression et (altitude))
LiquidCrystal_I2C lcd(0x27, 16, 2);            //Ecran LCD 2x16

WiFiClient espClient;                   //Wi-Fi
PubSubClient client(espClient);         //Broker MQTT

void setup() {

  Serial.begin(115200);
  Wire.begin();
  Serial.println("Starting...");

  //Initialisation de l'écran LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");

  // Initialisation des capteurs
  dht.begin();              //DHT22
  if (!bmp.begin(0x76)) {           //BMP280
    Serial.println("Error: BMP280 not detected.");
    while(1);
  }
  pinMode(LDRPIN, INPUT);
  pinMode(MQ135PIN, INPUT);

  //Initialisation des LEDs
  pinMode(LEDPIN, OUTPUT);

  //Initialisation du WiFi
  connectWiFi();

  //Initialisation du broker MQTT
  client.setServer(ip, 1883);
  client.setCallback(mqttCallback);
  connectMQTT();

  //Initialisation du serveur NTP (pour avoir l'heure)
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("Attempting NTP server connection...");
  struct tm timeinfo;
  while(!getLocalTime(&timeinfo)) {
    Serial.println("Error: NTP server connection failed.");
    delay(1000);
  }
  Serial.println("NTP server connected!");

  //Message d'initialisation complète
  lcd.clear();
  lcd.print("Ready!");

}

//Boucle de connexion au WiFi
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("Scanning WiFi networks...");
  int n = WiFi.scanNetworks();

  for (int i = 0; i < n; ++i) {
    String ssidScan = WiFi.SSID(i);
    for (WifiCredential cred : knownNetworks) {
      if (ssidScan == cred.ssid) {
        Serial.println("Attempting to connect to: " + ssidScan);
        WiFi.begin(cred.ssid, cred.password);
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
          delay(500);
          Serial.print(".");
        }
        if (WiFi.status() == WL_CONNECTED) {
          ip = cred.ip;
          Serial.println("WiFi connected to: " + ssidScan);
          Serial.println("IP address: " + WiFi.localIP().toString());
          return;
        } else {
          Serial.println("Error: Connection failed to " + ssidScan);
        }
      }
    }
  }
  Serial.println("Error : No known networks are available.");
}

//Boucle de connexion au broker MQTT
void connectMQTT() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT broker connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("MQTT broker connected!");
      client.subscribe("sensors/power/dht22");
      client.subscribe("sensors/power/bmp280");
      client.subscribe("sensors/power/ldr");
      client.subscribe("sensors/power/mq135");
    } else {
      Serial.println("Error: MQTT broker connection failed.");
      delay(2000);
    }
  }
}

//Réceptionne les messages publiés sur les topics sensor/power/
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();

  Serial.print("Message reçu sur ");
  Serial.print(topic);
  Serial.print(" : ");
  Serial.println(message);

  if (String(topic) == "sensors/power/dht22") {
    dhtEnabled = (message == "ON");
  } else if (String(topic) == "sensors/power/bmp280") {
    bmpEnabled = (message == "ON");
  } else if (String(topic) == "sensors/power/ldr") {
    ldrEnabled = (message == "ON");
  } else if (String(topic) == "sensors/power/mq135") {
    mq135Enabled = (message == "ON");
  }
}

//Fonction permettant de ne prendre que deux chiffres après la virgule
float roundToTwoDecimals(float value) {
  return round(value * 100.0) / 100.0;
}

//Fonction permettant d'afficher les données sur l'écran
void displayScreen(int page, float temperature, float humidity, float heatIndex, float pressure, int light, int airQuality) {
  lcd.clear();

  lcd.setCursor(0, 0);      // Première ligne
  lcd.setCursor(0, 1);      // Deuxième ligne

  switch (page) {
    case 0: 
      lcd.setCursor(0, 0);
      lcd.print("Temp: " + String(temperature) + (char)223 + "C");
      lcd.setCursor(0, 1);
      lcd.print("HeatIdx: " + String(roundToTwoDecimals(heatIndex)) + (char)223 + "C");
      break;
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("Hum: " + String(humidity) + "%");
      lcd.setCursor(0, 1);
      lcd.print("Press: " + String(pressure) + "hPa");
      break;
    case 2: 
      lcd.setCursor(0, 0);
      lcd.print("Light: " + String(light));
      lcd.setCursor(0, 1);
      lcd.print("AirQ: " + String(airQuality));
      break;
  }

}

void loop() {

  static int page = 0;

  connectWiFi();

  //Vérification de la connexion au broker MQTT
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  //Lecture des mesures
  float temperature = dhtEnabled ? dht.readTemperature() : -1;                 //Température du DHT22
  float humidity = dhtEnabled ? dht.readHumidity() : -1;                       //Humidité du DHT22
  float heatIndex = (dhtEnabled && !isnan(temperature) && !isnan(humidity)) ? dht.computeHeatIndex(temperature, humidity, false) : -1;               //Indice de chaleur du DHT22

  //float temperature = bmp.readTemperature();                //Température du BMP280 (non-utilisée ici)
  float pressure = bmpEnabled ? bmp.readPressure() / 100.0F : -1;  //Pression du BMP280 (en hPa)
  //float altitude = bmp.readAltitude(1013.25);    //Altitude du BMP280 (non-utilisée ici)

  int light = ldrEnabled ? analogRead(LDRPIN) : -1;             //LDR (valeur entre 0 et 4095)

  int airQuality = mq135Enabled ? analogRead(MQ135PIN) : -1;             // Valeur brute du capteur MQ-135

  //Allumer la LED si la qualité de l'air est > 10
  if (mq135Enabled && airQuality > 10) {
    digitalWrite(LEDPIN, HIGH);
  } else {
    digitalWrite(LEDPIN, LOW);
  }

  //Récupération du timestamp (en secondes)
  time_t now;
  time(&now);
  long timestamp = now;

  //Code principal
  if (!(isnan(temperature) || isnan(humidity) || isnan(pressure) || isnan(light) || isnan(airQuality))) {

    //Affichage des données sur l'écran
    displayScreen(page, temperature, humidity, heatIndex, pressure, light, airQuality);
    page = (page + 1) % 3;            // Passer à la page suivante

    //Création du fichier json
    StaticJsonDocument<256> jsonDoc;

    jsonDoc["timestamp"] = timestamp;
    jsonDoc["temperature"] = temperature;
    jsonDoc["humidity"] = humidity;
    jsonDoc["heatindex"] = roundToTwoDecimals(heatIndex);
    jsonDoc["pressure"] = pressure;
    jsonDoc["light"] = light;
    jsonDoc["airquality"] = airQuality;

    char jsonBuffer[256];
    serializeJson(jsonDoc, jsonBuffer);

    //Publication sur le broker MQTT
    client.publish("sensors/data", jsonBuffer);

    //Impression dans la console
    Serial.println(jsonBuffer);

  } else {
    lcd.clear();
    Serial.println("Error: Measurements cannot be read.");
  }

  delay(2000);         //Pause toutes les 2 secondes
}
