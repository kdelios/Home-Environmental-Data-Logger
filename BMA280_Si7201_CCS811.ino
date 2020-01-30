/* Environmental data station build, using a ESP8266 NodeMCUV3, and the following sensors:
  
   1. DHT21(AM2301) for measuring indoor temperature (°C) &  humidity (%RH).
   
   2. GY-21-Si7021  (I2C) for measuring outdoor temperature (°C) & humidity (%RH).
   
   3. CJMCU-811 CS811 (I2C) sensor measuring eCO2 (the equivalent CO2 *400ppm to 8192ppm*)& TVOC (Total Volatile Organic Compound *0ppb to 1187ppb*).
      CCS811 receives temperature and humidity readings from BME280 for compensation algorithm.
      New CCS811 sensors requires at 48h-burn in. Once burned in a sensor requires 20 minutes of run in before readings are considered good.
      **Connect nWAKE sensor pin directly to GND, so the CCS811 will avoid enter into SLEEP mode [sensor it's always ACTIVE]**
      
   4. DS18B20 attached at a heating radiator metal tube for measuring the heating water temp, in order to register the heat cycles.
     
   All data are transmited to a ThinkSpeak channel every 1 minute.
   Keep I2C sensor wires as sort as possible. Use high quality wires with as low capacitance as possible.
   Build by Konstantinos Deliopoulos @ Jan 2020. */

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>      //https://github.com/tzapu/WiFiManager
#include <OneWire.h>
#include "DHT.h"
#include <DallasTemperature.h>
#include <Wire.h>            // Wire library for I2C protocol
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_Si7021.h"
#include "Adafruit_CCS811.h"

const char* host = "api.thingspeak.com";
const char* THINGSPEAK_API_KEY = "QRI74I18VUQRQUXF";

 // DS18B20 Settings
#define DS18B20 2     //DS18B20 is connected to D4-->GPIO2-->Pin2
byte parasite_power_mode = 1;
float temp;           //Stores DS18B20 temperature value
OneWire ourWire(DS18B20);
DallasTemperature sensor(&ourWire);
const boolean IS_METRIC = true;

const int UPDATE_INTERVAL_SECONDS = 600;       // Update post to ThingSpeak every 60 seconds = 1 minute (60000ms). Min with ThingSpeak is ~20 seconds

#define DHTTYPE DHT21                          // DHT 21 (AM2301)
#define DHTPIN 0                               // Digital pin connected to the DHT sensor yellow wire --> D3 --> GPIO0
#define LED D8                                 // Initialize digital pin LED as an output (D8 green SMT LED with 150 Ohm resistor-draws ~ 7.3mA from GPIO15)
#define I2C_CCS811_ADDRESS 0x5A                // CCS811 I2C address

Adafruit_Si7021 sensor_si = Adafruit_Si7021(); // Si7021 is connected (I2C) to D1-->SLC-->GPIO5-->Pin5 & D2-->SDA-->GPIO4-->Pin4

Adafruit_CCS811 ccs;                           // CCS811 is connected (I2C) to D1-->SLC-->GPIO5-->Pin5 & D2-->SDA-->GPIO4-->Pin4

DHT dht(DHTPIN, DHTTYPE);
 
void setup() {
  
  delay(3000);                           // Give user some time to connect USB serial
  Serial.begin(115200);

  WiFiManager wifiManager;
 //reset saved settings
 //wifiManager.resetSettings();
  wifiManager.autoConnect("AutoConnectAP");
  
  pinMode(LED, OUTPUT);                  // LED pin as output
  Wire.begin();                          // Enable I2C
  dht.begin();                           // Enable DHT21
  sensor_si.begin();                     // Enable Si7021
  ccs.begin();                           // Enable CCS811
  delay(10);

 Serial.println("CCS811 test");
 if(!ccs.begin()){
 Serial.println("Failed to start CCS811 sensor! Please check your wiring!");
 while(1);

 Serial.println("Si7021 test");
 if(!sensor_si.begin()){
 Serial.println("Failed to start Si7021 sensor! Please check your wiring!");
while(1);

 // Set CCS811 to Mode 2: Pulse heating mode IAQ measurement every 10 seconds
 ccs.setDriveMode(CCS811_DRIVE_MODE_10SEC);
                
 }   
  }
   }
    
void loop() {     

  // Wait a few seconds between measurements
    delay(2000);
  
    Serial.print("connecting to ");
    Serial.println(host);

  // Read value from the BS18B20 sensor  
    sensor.requestTemperatures();
    temp = sensor.getTempCByIndex(0);

   /* Reading temperature or humidity from DHT21 takes about 250 milliseconds!
    Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)*/
  int h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
  Serial.println(F("Failed to read from DHT sensor!"));
  return;
  {

   // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
   // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  float sitemp = (sensor_si.readTemperature());
  int sihum = (sensor_si.readHumidity());

   // Pass DHT21 temp & hum readings to CSS811 for compensation algorithm
  ccs.setEnvironmentalData(t, h);

  if(ccs.available()){
  float temp = ccs.calculateTemperature();
  if(!ccs.readData()){
}
 }
   // Read CCS811 values
  float eco2 = ccs.geteCO2();
  float tvoc = ccs.getTVOC();
  
   // Use WiFiClient class to create TCP connections
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(host, httpPort)) {
      Serial.println("connection failed!");
      return;
    }
    
  Serial.print(F("  outdoor temp: "));
  Serial.print(sitemp);
  Serial.println(F(" °C "));
  Serial.print(F("  outdoor hum: "));
  Serial.print(sihum);
  Serial.println(F(" %RH "));
  Serial.print(F("  indoor temp: "));
  Serial.print(t);
  Serial.println(F(" °C "));
  Serial.print(F("  indoor hum: "));
  Serial.print(h);
  Serial.println(F(" %RH "));
  Serial.print(F("  heating water temp: "));
  Serial.print(temp);
  Serial.println(F(" °C "));
  Serial.print(F("  eCO2: "));
  Serial.print(eco2);
  Serial.println(F(" ppm "));
  Serial.print(F("  TVOC: "));
  Serial.print(tvoc);
  Serial.println(F(" ppb "));
  Serial.print("  RSSI: ");
  Serial.print(WiFi.RSSI());
  Serial.println("dbm");

    // Create a URI for the ThingSpeak.com request
    String url = "/update?api_key=";
    url += THINGSPEAK_API_KEY;
    url += "&field1=";
    url += String(sensor_si.readTemperature());  // outdoor temperature in Deg C (via Si7021)
    url += "&field2=";
    url += String(sensor_si.readHumidity());     // outdoor humidity in %RH (via Si7021)
    url += "&field3=";
    url += String(dht.readTemperature());        // indoor temperature in Deg C (via DHT21)
    url += "&field4=";
    url += String(dht.readHumidity());           // indoor humitity in %RH (via DHT21)
    url += "&field5=";
    url += String(WiFi.RSSI());                  // esp8266 rssi in dbm
    url += "&field6=";
    url += String(temp);                         // heater water temperature in Deg C (via BS18B20)
    url += "&field7=";
    url += String(ccs.geteCO2());                // indoor eCO2 (the equivalent CO2 *400ppm to 8192ppm*) (via CCS811)
    url += "&field8=";
    url += String(ccs.getTVOC());                // indoor TVOC (Total Volatile Organic Compound *0ppb to 1187ppb*) (via CCS811)

    Serial.print("Requesting URL: ");
    Serial.println(url);
    
    // Send the request to the ThinkSpeak server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" + 
                 "Connection: close\r\n\r\n");

    // Blink the LED at D8-->GPIO15 for 250ms          
     digitalWrite(LED, HIGH);  // turn the LED on
     delay(250);               // wait for 250ms
     digitalWrite(LED, LOW);   // turn the LED off
     
    delay(10);
    while(!client.available()){
      delay(100);
      Serial.print(".");
    }
    
    // Read all the lines of the reply from server and print them to Serial
    while(client.available()){
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }
    
    Serial.println();
    Serial.println("closing connection...");
  
delay(100 * UPDATE_INTERVAL_SECONDS);

}
 }
  }
