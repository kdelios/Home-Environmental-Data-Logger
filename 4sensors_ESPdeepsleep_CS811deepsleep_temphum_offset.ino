/* Environmental data station build, using a ESP8266 NodeMCUV3, and the following sensors:
  
   1. GY-BME280.3.3 (I2C) for measuring indoor temperature (°C), humidity (%RH), & atmosperic pressure (hPa).
   
   2. GY-21-Si7021  (I2C) for measuring outdoor temperature (°C) & humidity (%RH).
   
   3. CJMCU-811 CS811 (I2C) sensor measuring eCO2 (the equivalent CO2 *400ppm to 8192ppm*)& TVOC (Total Volatile Organic Compound *0ppb to 1187ppb*).
      CCS811 receives temperature and humidity readings from BME280 for compensation algorithm.
      New CCS811 sensors require 48h-burn in. Once burned in a sensor requires 20 minutes of run-in before readings are considered good.
      **Connect nWAKE sensor pin directly to D6, so the CCS811 will wake up after SLEEP mode** (requires a 5th wire doing to CCS811).
      **Connect nWAKE sensor pin directly to GND, so the CCS811 will avoid enter into SLEEP mode [sensor it's always ACTIVE] (not used here).**
      
   4. DS18B20 attached at a heating radiator metal tube for measuring the heating water temp, in order to register the heat cycles.
      **Connect a 4,7K pull-up resistor between DS18B20 Pin2 (DQ) and 3,3V**
   
   All data are transmited to a ThinkSpeak channel every 1 minute.
   ESP8266 is forced into deep sleep for 45" during the 1 min cycle.
   **GPIO 16 (D0) must be connected to reset (RST) pin so the ESP8266 is able to wake up from deep sleep**
   Keep I2C sensor wires as sort as possible. Use high quality wires with as low capacitance as possible.
   Build by Konstantinos Deliopoulos @ Dec 2019. */

#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h> // Wire library for I2C protocol
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "Adafruit_Si7021.h"
#include "Adafruit_CCS811.h"

/***************************
 * Begin Settings
 **************************/
 
const char* ssid     = "Andruino";
const char* password = "KosDim2020!";
const char* host = "api.thingspeak.com";
const char* THINGSPEAK_API_KEY = "QRI74I18VUQRQUXF";

// DS18B20 Settings
#define DS18B20 2     // DS18B20 is connected to D4-->GPIO2-->Pin2. Connect a 4,7K pullup resistor between DS18B20 Pin2 (DQ) and 3,3V
float temp;           // Stores temperature DS18B20 value
OneWire ourWire(DS18B20);
DallasTemperature sensor(&ourWire);
const boolean IS_METRIC = true;

#define I2C_CCS811_ADDRESS 0x5A                // CCS811 I2C address
#define LED D0                                 // Initialize digital pin LED_BUILTIN as an output (D0 green LED with 150Ohm resistor-draws ~ 7.3mA from GPIO16)
#define CCS811_WAKE_PIN D6                     // Digital pin connected to the CCS sensor nWAKE pin --> D6 --> GPIO12

Adafruit_Si7021 sensor_si = Adafruit_Si7021(); // Si7021 is connected (I2C) to D1-->SLC-->GPIO5-->Pin5 & D2-->SDA-->GPIO4-->Pin4

Adafruit_BME280 bme;                           // BME280 is connected (I2C) to D1-->SLC-->GPIO5-->Pin5 & D2-->SDA-->GPIO4-->Pin4

Adafruit_CCS811 ccs;                           // CCS811 is connected (I2C) to D1-->SLC-->GPIO5-->Pin5 & D2-->SDA-->GPIO4-->Pin4

const int UPDATE_INTERVAL_SECONDS = 600;       // Update post to ThingSpeak every 60 seconds = 1 minute (60000ms). Min with ThingSpeak is ~20 seconds

/***************************
 * End Settings
 **************************/
 
void setup() {
  
  // Enable serial
  delay(3000);                           // Give user some time to connect USB serial
  Serial.begin(115200);
  pinMode(LED, OUTPUT);                  // LED pin as output
  pinMode(CCS811_WAKE_PIN, OUTPUT);      // Define D6 (connected to CCS811 nWAKE pin) as output (not used here)
  digitalWrite(CCS811_WAKE_PIN, LOW);    // Wake up CCS811
  Wire.begin();                          // Enable I2C
  bme.begin();                           // Enable BME280
  sensor_si.begin();                     // Enable Si7021
  ccs.begin();                           // Enable CCS811
//ccs.begin(uint8_t(CCS811_WAKE_PIN));   // Enable CCS811
  delay(10);

 Serial.println("CCS811 test");
 if(!ccs.begin()){
 Serial.println("Failed to start CCS811 sensor! Please check your wiring!");
 while(1);

 Serial.println("BME280 test");
 if(!bme.begin()){
 Serial.println("Failed to start BMA280 sensor! Please check your wiring!");
 while(1);

 Serial.println("Si7021 test");
 if(!sensor_si.begin()){
 Serial.println("Failed to start Si7021 sensor! Please check your wiring!");
 while(1);

  // Set sample rate of CCS811 to 1 sec
 ccs.setDriveMode(CCS811_DRIVE_MODE_1SEC);

    /* BME280 is set to Weather Station Scenario.
       BME280 is set to forced mode, 1x temperature / 1x humidity / 8x pressure oversampling, filter off. Suggested rate is 1/60Hz (1 min)*/
    
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X8, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF   );

 pinMode(LED_BUILTIN, OUTPUT);  // Initialize digital pin LED_BUILTIN as an output (D0 green LED with 150Ohm resistor-draws ~ 7.3mA from GPIO16)
                
  // Connecting to the WiFi network
  Serial.println();
  Serial.printf("setup: MAC %s\n",WiFi.macAddress().c_str());
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
  Serial.printf(" up (%s)\n",WiFi.localIP().toString().c_str());
}
 }
  }
   }
  
void loop() {     

  // Wake up CCS811
    digitalWrite(CCS811_WAKE_PIN, LOW); 

    bme.takeForcedMeasurement();
    
    Serial.print("connecting to ");
    Serial.println(host);

   // Read value from the BS18B20 sensor  
    sensor.requestTemperatures();
    temp = sensor.getTempCByIndex(0);

   // Store values from the BME280 & Si7021 sensors 
  float bmetemp = bme.readTemperature();
  int bmehum = bme.readHumidity();
  float bmepres = (bme.readPressure() / 100.0F);
  float sitemp = (sensor_si.readTemperature(), 2);
  int sihum = (sensor_si.readHumidity(), 2);

   // Pass BME280 redings to CSS811 for compensation algorithm.
   ccs.setEnvironmentalData(bmetemp, bmehum);

  // Store CCS811 values
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
  Serial.print(F(" °C "));
  Serial.print(F("  outdoor hum: "));
  Serial.print(sihum);
  Serial.print(F(" %RH "));
  Serial.print(F("  indoor temp: "));
  Serial.print(bmetemp);
  Serial.print(F(" °C "));
  Serial.print(F("  indoor hum: "));
  Serial.print(bmehum);
  Serial.print(F(" %RH "));
  Serial.print(F("  barometer: "));
  Serial.print(bmepres);
  Serial.print(F(" hPa "));
  Serial.print(F("  heating water temp: "));
  Serial.print(temp);
  Serial.print(F(" °C "));
  Serial.print(F("  eCO2: "));
  Serial.print(eco2);
  Serial.print(F(" ppm "));
  Serial.print(F("  TVOC: "));
  Serial.print(tvoc);
  Serial.print(F(" ppb "));

    // Create a URI for the ThingSpeak.com request
    String url = "/update?api_key=";
    url += THINGSPEAK_API_KEY;
    url += "&field1=";
    url += String(sensor_si.readTemperature(), 2); // outdoor temperature in Deg C (via Si7021)
    url += "&field2=";
    url += String(sensor_si.readHumidity(), 2);    // outdoor humidity in %RH (via Si7021)
    url += "&field3=";
    url += String(bme.readTemperature());          // indoor temperature in Deg C (via BME280)
    url += "&field4=";
    url += String(bme.readHumidity());             // indoor humitity in %RH (via BME280)
    url += "&field5=";
    url += String(bme.readPressure() / 100.0F);    // barometric pressure in hPa (via BME280)
    url += "&field6=";
    url += String(temp);                           // heater water temperature in Deg C (via BS18B20)
    url += "&field7=";
    url += String(ccs.geteCO2());                  // indoor eCO2 (the equivalent CO2 *400ppm to 8192ppm*) (via CCS811)
    url += "&field8=";
    url += String(ccs.getTVOC());                  // indoor TVOC (Total Volatile Organic Compound *0ppb to 1187ppb*) (via CCS811)

    Serial.print("Requesting URL: ");
    Serial.println(url);
    
    // Send the request to the ThinkSpeak server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" + 
                 "Connection: close\r\n\r\n");

     // Blink the LED at D0-->GPIO16 for 250ms          
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

   digitalWrite(CCS811_WAKE_PIN, HIGH);  // CCS811 goto sleep

    // NodeMCU goes into deep sleep mode for 45 seconds, the ESP8266 wakes up by itself if GPIO 16 (D0 in NodeMCU board) is connected to the RESET pin
  Serial.println("I'm awake, but I'm going into deep sleep mode for 45 seconds...");
  ESP.deepSleep(45e6); 
  
delay(10 * UPDATE_INTERVAL_SECONDS);

}
