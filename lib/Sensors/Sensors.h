#ifndef _Sensors_h_
#define _Sensors_h_

#ifndef SENSORS_RAM_RECORDS
#define SENSORS_RAM_RECORDS 100
#endif

#ifndef ADS1115_GAIN
#define ADS1115_GAIN  1 // 0=6.144 1=4.096 2=2.048 4=1.024 8=0.512 16=0.256V
#endif
#ifndef ADS1115_MODE
#define ADS1115_MODE  1 // 0=continuous, 1=single reading
#endif
#ifndef ADS1115_RATE
#define ADS1115_RATE  0 // 0=8 1=16 2=32 3=64 4=128 5=250 6=475 7=860Hz
#endif

#include <Arduino.h>
#include <SparkFunBME280.h>
#include <ADS1X15.h>


//132 bytes per record
class SensorsRecord {
  private:
    uint32_t ts          = 0;
    int16_t  temperature = 32767; //(°C*10) Hopefully temperature should remain between -327° and 327°, spare 48bits compared to float
    uint16_t pressure    = 65535; //(Pa/10) Hopefully pressure wont exceed 655350Pa, spare 48bits compared to float (losing 10Pa precision)
    uint8_t  humidity    = 255;   //(%)     Relative humidity can be kept as unsigned integer (between 0 and 100), spare 56bits compared to float
    int16_t  adc_channel[4] = { -1, -1, -1, -1 }; //(none) Store light/soil moisture... from ADS1115
  public:
    SensorsRecord();
    SensorsRecord(uint32_t timestamp, float temperatureC, float pressurePa, float humidityPct, int16_t adc0=-1, int16_t adc1=-1, int16_t adc2=-1, int16_t adc3=-1);

    void setValues(uint32_t timestamp, float temperatureC, float pressurePa, float humidityPct, int16_t adc0=-1, int16_t adc1=-1, int16_t adc2=-1, int16_t adc3=-1);
    void setTemperature(int16_t temperature);
    void setTemperatureC(float temperatureC);
    void setPressure(uint16_t pressure);
    void setPressurePa(float pressurePa);
    void setPressurehPa(float pressurehPa);
    void setHumidity(uint8_t humidity);
    void setAdc(uint8_t channel, int16_t value);

    uint32_t    timestamp();
    int16_t     rawTemperature();
    float       temperatureC();
    uint16_t    rawPressure();
    float       pressurePa();
    float       pressurehPa();
    uint8_t     rawHumidity();
    int         humidityPct();
    int16_t     adc(uint8_t channel);
    void        print();
    char       *csvline();
    const char *csvhead();
};

class Sensors {
  private:
    SensorsRecord *records;
    uint16_t records_size;
    uint16_t records_index;
    BME280 *bme280 = nullptr;
    ADS1115 *ads1115 = nullptr;

  public:
    Sensors(uint16_t ram_records, uint8_t BMEaddr=0x76, uint8_t ADCaddr=0x48);
    bool begin();

    uint8_t update();
    SensorsRecord lastRecord();
    SensorsRecord getRecord(uint32_t timestamp);
    SensorsRecord getRecord(uint16_t record_index);
    SensorsRecord maxValues();
    SensorsRecord minValues();
    SensorsRecord avgValues();
};

#endif
