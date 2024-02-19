# Sensors library for ESPFarm

This library defines 2 classes SensorsRecord and Sensors to manage ESPfarm sensors (actually bme280 and ads1115)

## SensorsRecord

This class stores data read from sensors at a specified timestamp, and allows some transformations on it. To save some RAM, BME280 readings aren't sotred as float (64bits), but instead int16_t for temperature, uint16_t for pressure and uint8_t for relative humidity. SensorsRecord class should take care of transforming BME280 readings when setting/getting them.



## Sensors

This class stores a set of SensorsRecords and manages BME280/ADS1115 sensors.




## Usage example

```C
include <Arduino.h>
include <Sensors.h>
Sensors sensors(100);

void setup() {

};

void loop() {

};

```

