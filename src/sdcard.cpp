#include <Arduino.h>
#include "sdcard.h"
#include "config.h"

#include <Sensors.h>
extern Sensors *sensors;


bool SD_initialized = false;

void sd_listdir(File descriptor, bool recursive, uint8_t level) {
  File f;
  size_t size;
  float hsize;
  char unit=' ';
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
      size = f.size();
      if (size>(1024*1024)) { unit='M'; size = (size_t)((float)(size/(1024*1024))*10); hsize = (float)size/10; }
      else if (size>1024)   { unit='k'; size = (size_t)((float)(size/1024)*10); hsize = (float)size/10; }
      else hsize = (float)size;
      Serial.print(" ("); Serial.print(hsize); if(unit!=' ') Serial.print(unit); Serial.println("B)");
    }
  }
  return;
}

void sd_listdir(const char *path, bool recursive, uint8_t level) {
  File sdroot;
  if (!(sdroot = SD.open(path))) {
    Serial.print(F("SDcard error : can't open "));
    Serial.print(path);
    Serial.println(F(" for listing"));
    return;
  }
  Serial.print("SDcard content in ");
  Serial.println(path);
  sd_listdir(sdroot, recursive, level);
  uint64_t total = SD.totalBytes(), free;
  free = total - SD.usedBytes();
  Serial.print((int)(((float)free/(float)total)*100));
  Serial.print("% free,  ");
  Serial.print((int)(free/(1024*1024)));
  Serial.print("/");
  Serial.print((int)(total/(1024*1024)));
  Serial.println(F("MB used"));
  return;
}

bool sd_setup() {
  if (!SD.begin(SPI_SD_CS_PIN)) {
    Serial.println(F("SDcard error : initialization failed"));
    return false;
  }
  sd_listdir("/", false);

  if (!SD.exists(SD_SENSORS_CSVFILE)) {
    File csv;
    if (!(csv = SD.open(SD_SENSORS_CSVFILE, FILE_WRITE, true))) {
      Serial.print(F("SDcard error : failed to open "));
      Serial.println(SD_SENSORS_CSVFILE);
      return false;
    }
    csv.println(sensors->lastRecord().csvhead());
    csv.close();
    Serial.print(F("SDcard, initialized CSV log file "));
  }
  else {
    Serial.print(F("SDcard, resuming logs to CSV "));
  }
  Serial.println(SD_SENSORS_CSVFILE);
  SD_initialized = true;
  return true;
}
