#ifndef _sdcard_h_
#define _sdcard_h_
#include <Arduino.h>
#include <SD.h>

bool sd_setup();
bool sd_loadconf(const char *path="/espconfig.txt");
void sd_listdir(const char *path, bool recursive=true, uint8_t level=0);
void sd_listdir(File descriptor, bool recursive=true, uint8_t level=0);


#endif