#include <Arduino.h>
#include "config.h"
#include "display.h"

#include <U8g2lib.h>
U8G2_SH1106_128X64_NONAME_1_HW_I2C *u8g2 = nullptr;

#include <RTClib.h>
extern RTC_DS3231 *ds3231;
#include <Sensors.h>
extern Sensors *sensors;



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



void display_main(enum display_mode mode) {
  DateTime now = ds3231->now();
  char *label  = (char *)calloc(16, sizeof(char)), *buffer = (char *)calloc(64, sizeof(char));
  SensorsRecord record;
  switch(mode) {;
    case(DISPLAY_MAX): record = sensors->maxValues();  sprintf(label, "Maximal values"); break;
    case(DISPLAY_MIN): record = sensors->minValues();  sprintf(label, "Minimal values"); break;
    case(DISPLAY_AVG): record = sensors->avgValues();  sprintf(label, "Average values"); break;
    default:           record = sensors->lastRecord(); sprintf(label, "Current values"); break;
  }
  u8g2->firstPage();
  do {
    u8g2->setFont(u8g2_font_ncenB10_tr);
    sprintf(buffer, "%04d/%02d/%02d %02d:%02d", (int)now.year(), (int)now.month(), (int)now.day(), (int)now.hour(), (int)now.minute());
    u8g2->drawStr(0, 12, buffer);

    u8g2->setFont(u8g2_font_ncenB08_tr);
    u8g2->drawStr(0, 30, label);
    sprintf(buffer, "T=%.1f°C P=%.1fhPa", record.temperatureC(), record.pressurehPa());
    u8g2->drawStr(0, 42, buffer);
    int light = map(record.adc(ADC_LIGHT_CHANNEL), LIGHT_MIN, LIGHT_MAX, 0, 100);
    int moisture = map(record.adc(ADC_MOISTURE_CHANNEL), MOISTURE_MIN, MOISTURE_MAX, 0, 100);
    sprintf(buffer, "H%d%% L%d%% M%d%%", record.humidityPct(), light, moisture);
    u8g2->drawStr(0, 54, buffer);
  } while (u8g2->nextPage());
  free(buffer);
  return;
}
