#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include "config.h"

#include "display.h"

#include <SD.h>
bool SD_initialized = false;

#include <RTClib.h>
RTC_DS3231 *ds3231 = nullptr;

#include <Sensors.h>
Sensors *sensors;





void vTaskSensors(void *pvParameters) {
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  while(true) {
    sensors->update();
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SENSORS_UPDATE_INTERVAL));
    //vTaskDelay(pdMS_TO_TICKS(SENSORS_UPDATE_INTERVAL));
  }
}

void vTaskDisplay(void *pvParameters){
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  while(true) {
    display_main();
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(DISPLAY_UPDATE_INTERVAL));
  }
}

void vTaskSDlog(void *pvParameters) {
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  while(true) {
    if (SD_initialized && SD.exists(SD_SENSORS_CSVFILE)) {
      File csv = SD.open(SD_SENSORS_CSVFILE, "w");
      csv.println(sensors->lastRecord().csvline());
      csv.close();
    }
    Serial.println();
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SDLOG_UPDATE_INTERVAL));
  }
}

void sysinfos() {
  esp_chip_info_t out_info;
  esp_chip_info(&out_info);
  Serial.print("CPU freq   : "); Serial.print(ESP.getCpuFreqMHz()); Serial.println(" MHz");
  Serial.print("CPU cores  : "); Serial.println(out_info.cores);
  Serial.print("Flash size : "); Serial.println(ESP.getFlashChipSize() / 1000000); Serial.println(" MB");
  Serial.print("Free RAM   : "); Serial.println((long)ESP.getFreeHeap()); Serial.println(" bytes");
  //Serial.print("Min. free seen : "); Serial.println(String((long)esp_get_minimum_free_heap_size()) + " bytes");
  Serial.print("tskIDLE_PRIORITY     : "); Serial.println((long)tskIDLE_PRIORITY);
  Serial.print("configMAX_PRIORITIES : "); Serial.println((long)configMAX_PRIORITIES);
  Serial.print("configTICK_RATE_HZ   : "); Serial.print(configTICK_RATE_HZ); Serial.println(" Hz");
  Serial.println();
}



void setup() {
  Serial.begin(SERIAL_SPEED);
  delay(2000);
  Serial.println(F("ESPFarm booting"));
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  sysinfos();

  /*** SD card setup ***/
  if (SD.begin(SPI_SD_CS_PIN)) {
    SD_initialized = true;
    Serial.println(F("SD card initialized"));
    if (!SD.exists(SD_SENSORS_CSVFILE)) {
      File csv = SD.open(SD_SENSORS_CSVFILE, "w", true);
      csv.println(sensors->lastRecord().csvhead());
      csv.close();
    }
  }
  else { Serial.println(F("Error : SD initialization failed")); }
  sysinfos();

  /*** U8G2 LCD Screen setup ***/
  display_setup();
  sysinfos();

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
  sysinfos();

  sensors = new Sensors(SENSORS_RAM_RECORDS, I2C_ADDR_BME280, I2C_ADDR_ADS1115);
  if (!sensors->begin()) {
    Serial.println(F("Warning : Some sensors may be unavailable..."));
    delay(2000);
  }
  sysinfos();

  xTaskCreate(vTaskSensors, /*taskName=*/"vTaskSensors", /*stackDepth=*/10000, /*pvParameters=*/NULL, /*taskPriority=*/1, /*taskHandle=*/NULL);
  xTaskCreate(vTaskDisplay, /*taskName=*/"vTaskDisplay", /*stackDepth=*/10000, /*pvParameters=*/NULL, /*taskPriority=*/1, /*taskHandle=*/NULL);
  xTaskCreate(vTaskSDlog,   /*taskName=*/"vTaskSDlog",   /*stackDepth=*/10000, /*pvParameters=*/NULL, /*taskPriority=*/1, /*taskHandle=*/NULL);
}

void loop() {
  sysinfos();
  delay(1000);
}

