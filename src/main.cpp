#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>

#include "config.h"

#include "display.h"

#include "webserver.h"

#include "sdcard.h"
extern bool SD_initialized;

#include <ESPWiFi.h>
ESPWiFi *espwifi;

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
  File csv;
  while(true) {
    if (SD.exists(SD_SENSORS_CSVFILE) && sensors->lastRecord().timestamp()>0) {
      if (csv = SD.open(SD_SENSORS_CSVFILE, FILE_APPEND)) {
        csv.println(sensors->lastRecord().csvline());
        csv.close();
      }
    }
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SDLOG_UPDATE_INTERVAL));
  }
}

void vTaskNetwork(void *pvParameters) {
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  espwifi_status_t last = ESPWIFI_DISCONNECTED, cur;
  while(true) {
    cur = espwifi->update();
    vTaskDelay(1);
    if(cur!=last) {
      //WiFi status changed, print infos
      espwifi->status();
      last = cur;
      if (espwifi->ntp->isTimeSet()) {
        ds3231->adjust(DateTime(espwifi->ntp->getEpochTime()));
        DateTime now = ds3231->now();
        vTaskDelay(1);
        Serial.print("RTC time adjusted from NTP : ");
        Serial.print(now.year()); Serial.print("/"); Serial.print(now.month()); Serial.print("/"); Serial.print(now.day()); Serial.print(" ");
        Serial.print(now.hour()); Serial.print(":"); Serial.print(now.hour()); Serial.print(":"); Serial.println(now.second());        
      }
    }
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(NETWORK_UPDATE_INTERVAL));
  }
}





void sysinfos() {
  esp_chip_info_t out_info;
  esp_chip_info(&out_info);
  Serial.print("CPU freq   : "); Serial.print(ESP.getCpuFreqMHz()); Serial.println(" MHz");
  Serial.print("CPU cores  : "); Serial.println(out_info.cores);
  Serial.print("Flash size : "); Serial.print(ESP.getFlashChipSize() / 1000000); Serial.println(" MB");
  uint32_t freemem = ESP.getFreeHeap();
  Serial.print("Free RAM   : ");
  if (freemem > 2048) { float freekb = (float)freemem/1024; freekb = (float)((uint32_t)(freekb * 10)/10); Serial.print((long)freekb); Serial.println(" kBytes"); }
  else { Serial.print((long)freemem); Serial.println(" Bytes"); }
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

  /*** U8G2 LCD Screen setup ***/
  display_setup();
  display_bootup("ESPFarm booting...", 0, 1000);

  /*** WiFi configuration ***/
  espwifi = new ESPWiFi("espwifi", SPI_SD_CS_PIN);
  display_bootup("Loading ESP conf...", 10, 500);
  espwifi->conf_load();
  display_bootup("Starting ESPWiFi...", 20, 300);
  espwifi->start();
  xTaskCreate(vTaskNetwork, /*taskName=*/"vTaskNetwork", /*stackDepth=*/10000, /*pvParameters=*/NULL, /*taskPriority=*/1, /*taskHandle=*/NULL);

  /*** SD card setup ***/
  display_bootup("Setting up SD card...", 40, 500);
  sd_setup();

  /*** DS3231 RTC setup ***/
  display_bootup("Setting up RTC...", 60, 500);
  ds3231 = new RTC_DS3231();
  if (ds3231->begin()) {
    Serial.println(F("DS3231 RTC initialized"));
    if (ds3231->lostPower()) {
      Serial.println(F("Warning : RTC lost power, setting to firmware compilation time"));
      ds3231->adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }
  else { Serial.println(F("Error : DS3231 RTC initialization failed")); free(ds3231); ds3231 = nullptr; }

  /*** Sensors (BME280/ADS1115) setup*/
  display_bootup("Initializing sensors...", 70, 500);
  sensors = new Sensors(SENSORS_RAM_RECORDS, I2C_ADDR_BME280, I2C_ADDR_ADS1115);
  if (!sensors->begin()) {
    Serial.println(F("Warning : Some sensors may be unavailable..."));
    delay(2000);
  }

  /*** Webserver setup***/
  display_bootup("Web server setup...", 80, 500);
  webserver_setup(80, SPI_SD_CS_PIN);

  display_bootup("Starting tasks...", 95, 1000);
  xTaskCreate(vTaskSensors, /*taskName=*/"vTaskSensors", /*stackDepth=*/10000, /*pvParameters=*/NULL, /*taskPriority=*/1, /*taskHandle=*/NULL);
  xTaskCreate(vTaskSDlog,   /*taskName=*/"vTaskSDlog",   /*stackDepth=*/10000, /*pvParameters=*/NULL, /*taskPriority=*/1, /*taskHandle=*/NULL);

  display_bootup("ESPFarm ready!", 100, 1000);
  xTaskCreate(vTaskDisplay, /*taskName=*/"vTaskDisplay", /*stackDepth=*/10000, /*pvParameters=*/NULL, /*taskPriority=*/1, /*taskHandle=*/NULL);
}

void loop() {
  sysinfos();
  vTaskDelay(pdMS_TO_TICKS(10000));
}

