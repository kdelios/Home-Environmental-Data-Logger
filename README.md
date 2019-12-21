Environmental data station build, using a ESP8266 NodeMCUV3, and the following sensors:
  
   1. GY-BME280.3.3 (I2C) for measuring indoor temperature (°C), humidity (%RH), & atmosperic pressure (hPa).
   
   2. GY-21-Si7021  (I2C) for measuring outdoor temperature (°C) & humidity (%RH).
   
   3. CJMCU-811 CS811 (I2C) sensor measuring eCO2 (the equivalent CO2 400ppm to 8192ppm) & TVOC (Total Volatile Organic Compound 0ppb           to 1187ppb).
      CCS811 receives temperature and humidity readings from BME280 for compensation algorithm.
      New CCS811 sensors require 48h-burn in. Once burned in a sensor requires 20 minutes of run-in before readings are considered good.
      Connect nWAKE sensor pin directly to GND, so the CCS811 will avoid enter into SLEEP mode [sensor it's always ACTIVE]
      
   4. DS18B20 attached at a heating radiator metal tube for measuring the heating water temp, in order to register the heat cycles.
      Connect a 4,7K pull-up resistor between DS18B20 Pin2 (DQ) and 3,3V
   
   All data are transmited to a ThinkSpeak channel every 1 minute.
   ESP8266 is forced into deep sleep for 45" during the 1 min cycle.
   GPIO 16 (D0) must be connected to reset (RST) pin so the ESP8266 is able to wake up from deep sleep.
   Keep I2C sensor wires as sort as possible. Use high quality wires with as low capacitance as possible.
   Build by Konstantinos Deliopoulos @ Dec 2019.
