#include <DHT.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#define DEBUG 						  true
#define DHTPIN 						  2     // what digital pin we're connected to
#define SENSOR_LED                    14
#define LDR_INPUT                     A0
#define CO_CONTENT_INPUT              A1
#define SOIL_MOISTURE                 A2
#define HEATER_5V                     60000
#define HEATER_1_4_V                  90000
#define READING_DELAY_MILLIS          5000
#define DHTTYPE 					  DHT11

const char* g_host      = "pubsub.pubnub.com";
const char* g_pubKey    = "demo";
const char* g_subKey    = "demo";
const char* g_channel   = "hello_world";
SoftwareSerial Serial4(10,11);
DHT dht(DHTPIN, DHTTYPE);

unsigned long g_startMillis;
unsigned long g_switchTimeMillis;
boolean       g_heaterInHighPhase;

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(LDR_INPUT,INPUT);
  pinMode(CO_CONTENT_INPUT,INPUT);
  pinMode(SOIL_MOISTURE,INPUT);
  g_startMillis = millis();
  switchHeaterHigh();
  espCommand("AT",500,DEBUG);
  espCommand("AT+RST",2000,DEBUG);
}

void loop() {
  // Wait a few seconds between measurements.
  delay(2000);
  int ldrRead = analogRead(LDR_INPUT);
  int soilMoist = analogRead(SOIL_MOISTURE);
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    //return;
  }
  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);
  if(g_heaterInHighPhase){
    // 5v phase of cycle. see if need to switch low yet
    if(millis() > g_switchTimeMillis) {
      switchHeaterLow();
    }
  }
  else {
    // 1.4v phase of cycle. see if need to switch high yet
    if(millis() > g_switchTimeMillis) {
      switchHeaterHigh();
    }
  }
  unsigned int l_gasLevel = analogRead(CO_CONTENT_INPUT);
  unsigned int time = (millis() - g_startMillis) / 1000;
  Serial.print("SoilMoisture: ");
  Serial.print(soilMoist);
  Serial.print("\t");
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print(f);
  Serial.print(" *F\t");
  Serial.print("LDR: ");
  Serial.print(ldrRead);
  Serial.print("\tCO GAS Level: ");
  Serial.print(l_gasLevel);
  Serial.println();
  char l_buffer[6] = "";
  String url = "GET ";
  url += "/publish/";
  url += g_pubKey;
  url += "/";
  url += g_subKey;
  url += "/0/";
  url += g_channel;
  url += "/0/";
  url += "{\"s\":";
  url += String(soilMoist);
  //url += ",\"t\":";
  //url += String(t);
  url += ",\"G\":";
  url += String(l_gasLevel);
  url += "}";
  Serial.println(url);
  espCommand("AT+CIPMUX=0",500,DEBUG);
  espCommand("AT+CIPSTART=\"TCP\",g_host,80",2000,DEBUG);
  if(Serial4.find("Error")) return;
  else{
    String l_cmd = "AT+CIPSEND=";
    l_cmd += url.length();
    espCommand(l_cmd,500,DEBUG);
    espCommand(url,3000,DEBUG);
    espCommand("AT+CIPCLOSE",3000,DEBUG);
  }
  delay(READING_DELAY_MILLIS);
  espCommand("AT+RST",2000,DEBUG);
}

void switchHeaterHigh(void){
  // 5v phase
  digitalWrite(SENSOR_LED, LOW);
  g_heaterInHighPhase = true;
  g_switchTimeMillis = millis() + HEATER_5V;
}

void switchHeaterLow(void){
  // 1.4v phase
  digitalWrite(SENSOR_LED, HIGH);
  g_heaterInHighPhase = false;
  g_switchTimeMillis = millis() + HEATER_1_4_V;
}

String espCommand(String p_command, const int p_timeout, boolean p_debug)
{
  String l_response = "";
  Serial4.println(p_command); 
  long int l_time = millis();
  while( (l_time+p_timeout) > millis())
  {
    while(Serial4.available())
    {
      char l_buffer = Serial4.read();
      l_response+=l_buffer;
    }
  }
  if(p_debug)
  {
    Serial.print(l_response);
  }
  return l_response;
}
