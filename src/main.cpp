#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>

#include "config.h"
#include "i2cscanner.h"


#include <U8g2lib.h>
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

#include <SparkFunBME280.h>
BME280 bme280;


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
  for (uint8_t i=0; i<6; i++) { Serial.print(mac[i], HEX); Serial.print(":"); }
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
  Serial.println();
  Serial.println(F("*** BME280 readings ***"));
  Serial.print(F("  Temperature "));  Serial.println(bme280.readTempC(), 2);
  Serial.print(F("  Humidity ")); Serial.println(bme280.readFloatHumidity(), 0);
  Serial.print(F("  Pressure ")); Serial.println(bme280.readFloatPressure()/100, 0);
  return;
}


void u8g2_test() {
  u8g2.firstPage();
  do {
    /*u8g2.drawHLine(1,1,10);
    u8g2.drawHLine(1+5,2,5);*/
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(2,24,"Hello World!");
  } while ( u8g2.nextPage() );
  return;
}

void setup() {
  Serial.begin(SERIAL_SPEED);
  delay(500);
  Serial.println(F("ESPFarm booting"));
  Wire.begin(PIN_I2CSDA, PIN_I2CSCL);
  u8g2.begin();
  bme280.setI2CAddress(0x76);
  bme280.beginI2C();
  delay(2000);
}

void loop() {
  static uint8_t i=0;
  i2c_scan();
  bme280_test();
  u8g2_test();
  if ((i%10)==0) { wifi_scan(); i=0; }
  i++;
  delay(1000);
}

