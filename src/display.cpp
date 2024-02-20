#include <Arduino.h>
#include "config.h"
#include "display.h"

#include <U8g2lib.h>
U8G2_SH1106_128X64_NONAME_1_HW_I2C *u8g2 = nullptr;

#include <RTClib.h>
extern RTC_DS3231 *ds3231;
#include <Sensors.h>
extern Sensors *sensors;

#include "u8g2_icons.h"


bool display_setup() {
  u8g2 = new U8G2_SH1106_128X64_NONAME_1_HW_I2C(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
  u8g2->setI2CAddress(2*I2C_ADDR_U8G2);
  if (u8g2->begin()) {
    Serial.println(F("U8g2 screen initialized"));
    return true;
  }
  Serial.println(F("Error : U8g2 screen initialization failed"));
  free(u8g2);
  u8g2 = nullptr;
  return false;
}


void display_record(char *label, SensorsRecord record) {
  DateTime now;
  if (ds3231!=nullptr) now = ds3231->now();
  else now = DateTime();
  char *buffer = (char *)calloc(64, sizeof(char));
  u8g2->firstPage();
  do {
    u8g2->setFont(u8g2_font_ncenB08_tr);
    sprintf(buffer, "%04d/%02d/%02d %02d:%02d:%02d", (int)now.year(), (int)now.month(), (int)now.day(), (int)now.hour(), (int)now.minute(), (int)now.second());
    u8g2->drawStr(8,   8, buffer);
    //u8g2->drawStr(0,  16, label);
  
    u8g2->drawXBM(4,  16, temperature_icon_width, temperature_icon_height, temperature_icon_bits);
    u8g2->drawXBM(64, 16, humidity_icon_width,    humidity_icon_height,    humidity_icon_bits);
    //u8g2->drawXBM(84, 16, pressure_icon_width,    pressure_icon_height,    pressure_icon_bits);
    sprintf(buffer, "%.1f°C",  record.temperatureC()+(float)SENSORS_TEMPERATURE_OFFSET); u8g2->drawStr(26, 32, buffer);
    sprintf(buffer, "%d%%",    record.humidityPct()+(int)SENSORS_HUMIDITY_OFFSET);  u8g2->drawStr(88, 32, buffer);
    //sprintf(buffer, "%.1fhPa", record.pressurehPa()+(float)SENSORS_PRESSURE_OFFSET);  u8g2->drawStr(88, 32, buffer);
  
    u8g2->drawXBM(0,  40, light_icon_width,       light_icon_height,       light_icon_bits);
    u8g2->drawXBM(62, 40, moisture_icon_width,    moisture_icon_height,    moisture_icon_bits);
    sprintf(buffer, "%d%%", map(record.adc(ADC_LIGHT_CHANNEL), LIGHT_MIN, LIGHT_MAX, 0, 100));          u8g2->drawStr(26, 56, buffer);
    sprintf(buffer, "%d%%", map(record.adc(ADC_MOISTURE_CHANNEL), MOISTURE_MIN, MOISTURE_MAX, 0, 100)); u8g2->drawStr(88, 56, buffer);
  } while (u8g2->nextPage());
  free(buffer);
  return;
}


void display_main(enum display_mode mode) {
  DateTime now = ds3231->now();
  char *label  = (char *)calloc(16, sizeof(char));
  SensorsRecord record;
  switch(mode) {;
    case(DISPLAY_MAX): record = sensors->maxValues();  sprintf(label, "Maximal values"); break;
    case(DISPLAY_MIN): record = sensors->minValues();  sprintf(label, "Minimal values"); break;
    case(DISPLAY_AVG): record = sensors->avgValues();  sprintf(label, "Average values"); break;
    default:           record = sensors->lastRecord(); sprintf(label, "Current values"); break;
  }
  display_record(label, record);
  return;
}
