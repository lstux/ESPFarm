// Uncomment to activate debugging mode and serial messages
#define DEBUG
#define SERIAL_SPEED 115200

#define SENSORS_RAM_RECORDS     100
#ifdef DEBUG
#define DISPLAY_UPDATE_INTERVAL 1000  //1s
#define SENSORS_UPDATE_INTERVAL 1000  //1s
#define SDLOG_UPDATE_INTERVAL   60000 //1m
#else
#define DISPLAY_UPDATE_INTERVAL 1000   //1s
#define SENSORS_UPDATE_INTERVAL 10000  //1m
#define SDLOG_UPDATE_INTERVAL   600000 //10m
#endif


#define SPI_SD_CS_PIN 5
#define SD_SENSORS_CSVFILE "/ESPFarm.csv"

#define I2C_SDA_PIN      6
#define I2C_SCL_PIN      7
#define I2C_ADDR_U8G2    0x3c
#define I2C_ADDR_BME280  0x76
#define I2C_ADDR_ADS1115 0x48
#define I2C_ADDR_DS3231  0x68 // For reference only, can't be changed in RTClib

#define SENSORS_TEMPERATURE_OFFSET -2
#define SENSORS_PRESSURE_OFFSET     0
#define SENSORS_HUMIDITY_OFFSET     0
#define SENSORS_LIGHT_OFFSET        0
#define SENSORS_MOISTURE_OFFSET     0

#define ADC_LIGHT_CHANNEL 0
/*#define LIGHT_MIN     10500
#define LIGHT_MAX     100*/
#define LIGHT_MIN     32768
#define LIGHT_MAX     0

#define ADC_MOISTURE_CHANNEL 1
#define MOISTURE_MIN     0
#define MOISTURE_MAX     32768
