#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>

#include "config.h"
#include "i2cscanner.h"

#include <SD.h>
bool SD_initialized = false;

#include <U8g2lib.h>
U8G2_SH1106_128X64_NONAME_1_HW_I2C *u8g2 = nullptr;

#include <RTClib.h>
RTC_DS3231 *ds3231 = nullptr;

#include <SparkFunBME280.h>
BME280 *bme280;

#include <ADS1X15.h>
ADS1115 *ads1115 = nullptr;


void i2c_scan() {
  int e;
  Serial.println();
  Serial.println(F("*** Scanning I2C bus ***"));
  for (byte addr=1; addr<127; addr++) {
    Wire.beginTransmission(addr);
    e = Wire.endTransmission();
    switch(e) {
      case(0): Serial.print("  x"); Serial.print(addr, HEX); Serial.print(" "); Serial.println(i2caddresses[addr]); break;
      case(2): break;
      default: Serial.print(" !x"); Serial.print(addr, HEX); Serial.print("  Unmanaged error "); Serial.println(e); break;
    }
  }
}

void wifi_scan() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.println();
  Serial.println(F("*** Scanning WiFi networks ***"));
  Serial.print(F("MAC address "));
  for (uint8_t i=0; i<5; i++) { Serial.print(mac[i], HEX); Serial.print(":"); }
  Serial.print(mac[5], HEX);
  Serial.println();
  int numSsid = WiFi.scanNetworks();
  if (numSsid < 0) { Serial.println(F("Error : failed to scan WiFi networks")); return; }
  if (numSsid == 0) { Serial.println(F("No WiFi network found")); return; }
  Serial.print(numSsid); Serial.println(F(" WiFi network(s) found :"));
  for (int i=0; i<numSsid; i++) {
    Serial.print("  "); Serial.print(WiFi.SSID(i));
    Serial.print(F("\tSignal: ")); Serial.print(WiFi.RSSI(i)); Serial.print(F("dBm"));
    Serial.print(F("\tEncryption: "));
    switch(WiFi.encryptionType(i)) {
      case WIFI_AUTH_WEP:             Serial.println(F("WEP")); break;
      case WIFI_AUTH_WPA_PSK:         Serial.println(F("WPA")); break;
      case WIFI_AUTH_WPA2_PSK:        Serial.println(F("WPA2")); break;
      case WIFI_AUTH_WPA2_ENTERPRISE: Serial.println(F("WPA2 Enterprise")); break;
      case WIFI_AUTH_OPEN:            Serial.println(F("None")); break;
      default:            Serial.println(F("Unknown")); break;
    }
  }
}

void bme280_test() {
  if (bme280 == nullptr) return;
  Serial.println();
  Serial.println(F("*** BME280 readings ***"));
  Serial.print(F("  Temperature "));  Serial.println(bme280->readTempC(), 2);
  Serial.print(F("  Humidity ")); Serial.println(bme280->readFloatHumidity(), 0);
  Serial.print(F("  Pressure ")); Serial.println(bme280->readFloatPressure()/100, 0);
  return;
}


void u8g2_test() {
  if (u8g2 == nullptr) return;
  u8g2->firstPage();
  do {
    /*u8g2.drawHLine(1,1,10);
    u8g2.drawHLine(1+5,2,5);*/
    u8g2->setFont(u8g2_font_ncenB10_tr);
    u8g2->drawStr(2,24,"Hello World!");
  } while ( u8g2->nextPage() );
  return;
}

void sd_test() {
  if (!SD_initialized) return;
  Serial.println();
  Serial.println(F("*** SD card ***"));
  switch(SD.cardType()) {
    case CARD_NONE: Serial.println(F("  No card found"));    return; break;
    case CARD_SD:   Serial.println(F("  SD card found"));    break;
    case CARD_SDHC: Serial.println(F("  SDHC card found"));  break;
    case CARD_MMC:  Serial.println(F("  SDMMC card found")); break;
    default:        Serial.println(F("  Unknown SD card type")); break;
  }
  Serial.print(F("  card size : ")); Serial.print((uint64_t)(SD.cardSize()/(1024*1024))); Serial.println("MB");
  return;
}

void ds3231_test() {
  if (ds3231 == nullptr) return;
  Serial.println();
  Serial.println(F("*** DS3231 RTC ***"));
  DateTime now = ds3231->now();
  Serial.print(F("  Date : "));
  Serial.print(now.year(), DEC); Serial.print("/"); Serial.print(now.month(), DEC); Serial.print("/"); Serial.println(now.day());
  Serial.print(F("  Time : "));
  Serial.print(now.hour(), DEC); Serial.print(':'); Serial.print(now.minute(), DEC); Serial.print(':'); Serial.println(now.second(), DEC);
  return;
}

void ads1115_test() {
  if (ads1115 == nullptr) return;
  Serial.println();
  Serial.println(F("*** ADS1115 ADC ***"));
  int16_t v, m;
  for (uint8_t i=0; i<4; i++) {
    v = ads1115->readADC(i); m = map(v, 0, 32768, 0, 100);
    /* m = ((float)v/32768)*100; */
    Serial.print(F("  channel ")); Serial.print(i); Serial.print(F(" : "));
    Serial.print(v); Serial.print(F("raw (")); Serial.print(m); Serial.println(F("%)"));
  }
  v = ads1115->readADC(LIGHT_CHANNEL); m = map(v, LIGHT_MIN, LIGHT_MAX, 0, 100);
  /*m = (int16_t)((((float)v-LIGHT_MIN)/(LIGHT_MAX-LIGHT_MIN))*100);
  if (m>100) m=100;*/
  Serial.print(F("  light   ")); Serial.print(LIGHT_CHANNEL); Serial.print(F(" : "));
  Serial.print(v); Serial.print(F("raw (")); Serial.print(m); Serial.println(F("%)"));
  return;
}

void setup() {
  Serial.begin(SERIAL_SPEED);
  delay(2000);
  Serial.println(F("ESPFarm booting"));
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  /*** SD card setup ***/
  if (SD.begin(SPI_SD_CS_PIN)) {
    SD_initialized = true;
    Serial.println(F("SD card initialized"));    
  }
  else { Serial.println(F("Error : SD initialization failed")); }

  /*** U8G2 LCD Screen setup ***/
  u8g2 = new U8G2_SH1106_128X64_NONAME_1_HW_I2C(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
  u8g2->setI2CAddress(2*I2C_ADDR_U8G2);
  if (u8g2->begin()) { Serial.println(F("U8g2 screen initialized")); }
  else { Serial.println(F("Error : U8g2 screen initialization failed")); free(u8g2); u8g2 = nullptr; }

  /*** DS3231 RTC setup ***/
  ds3231 = new RTC_DS3231();
  if (ds3231->begin()) {
    Serial.println(F("DS3231 RTC initialized"));
    if (ds3231->lostPower()) {
      Serial.println(F("Warning : RTC lost power, setting to firmware compilation time"));
      ds3231->adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }
  else { Serial.println(F("Error : DS3231 RTC initialization failed")); free(ds3231); ds3231 = nullptr; }

  /*** BME280 sensor setup ***/
  bme280 = new BME280();
  bme280->setI2CAddress(I2C_ADDR_BME280);
  if (bme280->beginI2C()) { Serial.println(F("BME280 initialized")); }
  else { Serial.println(F("BME280 initialization failed")); free(bme280); bme280 = nullptr; }

  /** ADS1115 ADC setup ***/
  ads1115 = new ADS1115(I2C_ADDR_ADS1115);
  if (ads1115->begin()) { Serial.println(F("ADS1115 ADC initialized")); ads1115->setGain(ADS1115_GAIN); ads1115->setMode(ADS1115_MODE); ads1115->setDataRate(ADS1115_RATE); }
  else { Serial.println(F("Error : ADS1115 ADC initialization failed")); free(ads1115); ads1115 = nullptr; }

  delay(2000);
}

void loop() {
  static uint8_t i=0;
  sd_test();
  i2c_scan();
  ds3231_test();
  bme280_test();
  ads1115_test();
  u8g2_test();
  if ((i%10)==0) { wifi_scan(); i=0; }
  i++;
  delay(1000);
}

