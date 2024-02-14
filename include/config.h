#define SERIAL_SPEED 115200

#define SPI_SD_CS_PIN 5

#define I2C_SDA_PIN      6
#define I2C_SCL_PIN      7
#define I2C_ADDR_U8G2    0x3c
#define I2C_ADDR_BME280  0x76
#define I2C_ADDR_ADS1115 0x48
#define I2C_ADDR_DS3231  0x68 // For reference only, can't be changed in RTClib

#define ADS1115_GAIN  1 // 0=6.144 1=4.096 2=2.048 4=1.024 8=0.512 16=0.256V
#define ADS1115_MODE  1 // 0=continuous, 1=single reading
#define ADS1115_RATE  0 // 0=8 1=16 2=32 3=64 4=128 5=250 6=475 7=860Hz

#define LIGHT_CHANNEL 0
#define LIGHT_MIN     10500
#define LIGHT_MAX     100
