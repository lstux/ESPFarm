#ifndef _ESPWiFi_h_
#define _ESPWiFi_h_

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#include <Preferences.h>
#include <LittleFS.h>
#include <SD.h>

#define ESPWIFI_D_HOSTNAME "ESPWiFi"
#define ESPWIFI_D_APSSID   "ESPWiFi"
#define ESPWIFI_D_APPASS   "espwifi1234"
#define ESPWIFI_D_CONFFILE "/espconf.txt"

#define ESPWIFI_PREFS_NAMESPACE "espwifi"


class ESPWiFi {
  private:
    WiFiMulti w;
    WiFiUDP wudp;
    NTPClient *ntp;
    char prefs_namespace[16];
    Preferences prefs;
    uint8_t sd_cs_pin;
    bool littlefs_mounted = false;
    bool sd_mounted = false;
    File conf_open(const char *path=ESPWIFI_D_CONFFILE);
    bool file_findKey(const char *key, char *value, File f);
    bool conf_findKey(const char *key, char *value, const char *path=ESPWIFI_D_CONFFILE);
  public:
    ESPWiFi(const char *prefs_namespace=ESPWIFI_PREFS_NAMESPACE, uint8_t sd_cs_pin=(uint8_t)20U);
    bool conf_load(const char *path=ESPWIFI_D_CONFFILE, bool remove=false);
    uint8_t start();
    void debug();
    /*void list();
    bool save(char *ssid, char*pass);
    bool remove(char *ssid);
    bool load();
    bool connect();
    uint8_t status();*/
};

#endif