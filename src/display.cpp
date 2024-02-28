#include <Arduino.h>
#include "config.h"
#include "display.h"

#include <U8g2lib.h>
//U8G2_SH1106_128X64_NONAME_1_HW_I2C *u8g2 = nullptr;
U8G2_SSD1306_128X64_NONAME_1_HW_I2C *u8g2 = nullptr;
#include <RTClib.h>
extern RTC_DS3231 *ds3231;
#include <Sensors.h>
extern Sensors *sensors;

#include "u8g2_icons.h"


bool display_setup() {
  pinMode(BUTTON_OK, INPUT_PULLUP);
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  u8g2 = new U8G2_SSD1306_128X64_NONAME_1_HW_I2C(U8G2_R0,  /* reset=*/ U8X8_PIN_NONE);
  u8g2->setI2CAddress(2*I2C_ADDR_U8G2);
  if (!u8g2->begin()) {
    Serial.println(F("U8g2 screen initialized"));
    return true;
  }
  Serial.println(F("Error : U8g2 screen initialization failed"));
  free(u8g2);
  u8g2 = nullptr;
  return false;
}

void display_bootup(const char *label, uint8_t pct, uint16_t delayms) {
  u8g2->clear();
  u8g2->firstPage();
  do {
    u8g2->setFont(u8g2_font_ncenB08_tr);
    u8g2->drawStr(14, 24, label);
    u8g2->drawRFrame(14, 40, 100, 12, 2);
    if (pct>0) u8g2->drawRBox(14, 40, pct, 10, 2);
  } while (u8g2->nextPage());
  if (delay>0) delay(delayms);
  return;
}


void display_record(char *label, SensorsRecord record) {
  DateTime now;
  if (ds3231!=nullptr) now = ds3231->now();
  else now = DateTime();
  char *buffer = (char *)calloc(64, sizeof(char));
  u8g2->firstPage();
  do {
    u8g2->setFont(u8g2_font_ncenB08_tr);
    sprintf(buffer, "%s %04d/%02d/%02d %02d:%02d:%02d", label, (int)now.year(), (int)now.month(), (int)now.day(), (int)now.hour(), (int)now.minute(), (int)now.second());
    u8g2->drawStr(0,   8, buffer);
  
    u8g2->drawXBM(8,  14, temperature_icon_width, temperature_icon_height, temperature_icon_bits);
    u8g2->drawXBM(68, 14, humidity_icon_width,    humidity_icon_height,    humidity_icon_bits);
    //u8g2->drawXBM(84, 14, pressure_icon_width,    pressure_icon_height,    pressure_icon_bits);
    sprintf(buffer, "%.1f°C",  record.temperatureC()+(float)SENSORS_TEMPERATURE_OFFSET); u8g2->drawStr(30, 30, buffer);
    sprintf(buffer, "%d%%",    record.humidityPct()+(int)SENSORS_HUMIDITY_OFFSET);  u8g2->drawStr(92, 30, buffer);
    //sprintf(buffer, "%.1fhPa", record.pressurehPa()+(float)SENSORS_PRESSURE_OFFSET);  u8g2->drawStr(88, 30, buffer);
  
    u8g2->drawXBM(4,  40, light_icon_width,       light_icon_height,       light_icon_bits);
    u8g2->drawXBM(66, 40, moisture_icon_width,    moisture_icon_height,    moisture_icon_bits);
    sprintf(buffer, "%d%%", map(record.adc(ADC_LIGHT_CHANNEL), LIGHT_MIN, LIGHT_MAX, 0, 100));          u8g2->drawStr(30, 56, buffer);
    sprintf(buffer, "%d%%", map(record.adc(ADC_MOISTURE_CHANNEL), MOISTURE_MIN, MOISTURE_MAX, 0, 100)); u8g2->drawStr(92, 56, buffer);
  } while (u8g2->nextPage());
  free(buffer);
  return;
}


void display_main(enum display_mode mode) {
  DateTime now = ds3231->now();
  char *label  = (char *)calloc(4, sizeof(char));
  SensorsRecord record;
  switch(mode) {;
    case(DISPLAY_MAX): record = sensors->maxValues();  sprintf(label, "Max"); break;
    case(DISPLAY_MIN): record = sensors->minValues();  sprintf(label, "Min"); break;
    case(DISPLAY_AVG): record = sensors->avgValues();  sprintf(label, "Avg"); break;
    //default:           record = sensors->lastRecord(); sprintf(label, "Cur"); break;
    default:           record = sensors->read(); sprintf(label, "Cur"); break;
  }
  display_record(label, record);
  return;
}
