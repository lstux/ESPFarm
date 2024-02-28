#ifndef _ESPWiFi_h_
#define _ESPWiFi_h_

#define ESPWIFI_PREFS_NAMESPACE "espwifi"
#define ESPWIFI_MAX_SSIDS  3

#define ESPWIFI_D_HOSTNAME "ESPWiFi"
#define ESPWIFI_D_APSSID   "ESPWiFi"
#define ESPWIFI_D_APPASS   "espwifi1234"
#define ESPWIFI_D_CONFFILE "/espconf.txt"

#define ESPWIFI_WITH_WEBSERVER
#define ESPWIFI_WEBSERVER_PORT 80

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <SD.h>

#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <NTPClient.h>


enum espwifi_status_t {
  ESPWIFI_CONNECTED,
  ESPWIFI_SCANNING,
  ESPWIFI_CONNECTING,
  ESPWIFI_DISCONNECTED
};

typedef struct {
    char * ssid;
    char * passphrase;
    uint8_t priority;
    bool hasFailed;
} WifiAPlist_t;

class ESPWiFi {
  private:
    espwifi_status_t wifistatus;
    uint8_t CurAP;
    uint32_t CurAPconnect;
    WifiAPlist_t APlist[ESPWIFI_MAX_SSIDS];
    WiFiUDP wudp;
    char prefs_namespace[16];
    Preferences prefs;
    uint8_t sd_cs_pin;
    bool littlefs_mounted = false;
    bool sd_mounted = false;
    void connect_best(int16_t nbssid);
    File conf_open(const char *path=ESPWIFI_D_CONFFILE, const char *mode=FILE_READ);
    bool file_findKey(const char *key, char *value, File f);
    bool conf_findKey(const char *key, char *value, const char *path=ESPWIFI_D_CONFFILE);
  public:
    ESPWiFi(const char *prefs_namespace=ESPWIFI_PREFS_NAMESPACE, uint8_t sd_cs_pin=(uint8_t)20U);
    ~ESPWiFi();
    NTPClient *ntp;
    bool conf_load(const char *path=ESPWIFI_D_CONFFILE, bool remove=false);
    uint8_t start();
    wl_status_t status();
    espwifi_status_t update();
};

#endif