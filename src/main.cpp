//Potřebné knihovny
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>


//Přednastavené údaje k Wifi
const char* ssid = "jmeno";
const char* password = "heslo";

//Přednastavené údaje k MQTT
const char* mqtt_server = "ip adresa MQTT";

const double VCC = 3.3;             
const double R2 = 10000;            
const double adc_resolution = 1023; 

//Potřebné parametry rovnice termistoru
const double A = 0.001129148;   
const double B = 0.000234125;
const double C = 0.0000000876741; 

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // Připojení k WiFi
  Serial.println();
  Serial.print("Připojování k ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi připojena");
  Serial.println("IP adresa: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Zpráva příjmuta [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  //Ledka
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  
  }

}

void reconnect() {
  // Opakování dokud se nepřipojí
  while (!client.connected()) {
    Serial.print("Zkouška připojení k MQTT...");
    // Vytvoření náhodného ID klienta
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Pokus o připojení
    if (client.connect(clientId.c_str())) {
      Serial.println("připojeno");
      client.subscribe("inTopic");
    } else {
      Serial.print("nepřipojeno, rc=");
      Serial.print(client.state());
      Serial.println(" zkus znova za 5 sekund");
      // Čekej 5 sekund na opakování
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  //Převedení napětí na teplotu
  double Vout, Rth, teplota, adc_value; 

  adc_value = analogRead(A0);
  Vout = (adc_value * VCC) / adc_resolution;
  Rth = (VCC * R2 / Vout) - R2;

  //Vzorec pro vypočítání
  teplota = (1 / (A + (B * log(Rth)) + (C * pow((log(Rth)),3))));   

  //Převedení z Kelvinu na Celsia
  teplota = teplota - 273.15;  
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //posílání zpráv na HA
  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, " %lf", teplota);
    Serial.println(teplota);
    client.publish("/testtopic/teplota", msg);
  }
  delay(500);
}
