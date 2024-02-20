#include <Arduino.h>
#include "sdcard.h"
#include "config.h"

bool SD_initialized = false;

void sd_listdir(File descriptor, bool recursive, uint8_t level) {
  File f;
  while (f = descriptor.openNextFile()) {
    if (!f) break;
    for (uint8_t i=0; i<level+1; i++) Serial.print("  ");
    if (f.isDirectory()) Serial.print("d "); else Serial.print("f ");
    Serial.print(f.name());
    if (f.isDirectory()) {
      Serial.println();
      if (recursive) sd_listdir(f, recursive, level+1);
    }
    else {
      Serial.print(" (");
      Serial.print(f.size(), DEC);
      Serial.print("B)");
      Serial.println();
    }
  }
  return;
}

void sd_listdir(const char *path, bool recursive, uint8_t level) {
  Serial.print("Listing SD content in ");
  Serial.println(path);
  File sdroot;
  if (!(sdroot = SD.open(path))) {
    Serial.print(F("SD can't open "));
    Serial.println(path);
    return;
  }
  sd_listdir(sdroot, recursive, level);
  uint64_t total = SD.totalBytes(), free;
  free = total - SD.usedBytes();
  Serial.print("  ");
  Serial.print((int)(free/(1024*1024)));
  Serial.print("/");
  Serial.print((int)(total/(1024*1024)));
  Serial.println(F("MB used"));
  return;
}

bool sd_setup() {
  if (!SD.begin(SPI_SD_CS_PIN)) {
    Serial.println(F("Error : SD initialization failed"));
    return false;
  }
  sd_listdir("/", false);

  if (!SD.exists(SD_SENSORS_CSVFILE)) {
    File csv;
    if (!(csv = SD.open(SD_SENSORS_CSVFILE, FILE_WRITE, true))) {
      Serial.print(F("Error : failed to open SD file "));
      Serial.println(SD_SENSORS_CSVFILE);
      return false;
    }
    //csv.println(sensors->lastRecord().csvhead());
    csv.close();
    Serial.print(F("SD card, initialized CSV log file "));
  }
  else {
    Serial.print(F("SD card, resuming logs to CSV "));
  }
  Serial.println(SD_SENSORS_CSVFILE);
  SD_initialized = true;
  return true;
}

bool sd_loadconf(const char *path="/espconfig.txt") {
  if (!SD.exists(path)) return false;
  File cfile;
  if (!(cfile = SD.open(path))) {
    Serial.print(F("Error : failed to open SD file "));
    Serial.println(path);
    return false;
  }

  cfile.close();
  return true;
}