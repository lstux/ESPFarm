#include "ESPWiFi.h"

/* Configuration file format is
key=value
key2=value
....
*/
#define CONFIG_KEY_MAXLEN 32
#define CONFIG_VALUE_MAXLEN 64


// Constructor
ESPWiFi::ESPWiFi(const char *prefs_namespace, uint8_t sd_cs_pin) {
  strcpy(this->prefs_namespace, prefs_namespace);
  this->sd_cs_pin = sd_cs_pin;
  this->ntp = new NTPClient(this->wudp);
}

ESPWiFi::~ESPWiFi() {
  delete this->ntp;
}

// Search key in File and store to value if found
bool ESPWiFi::file_findKey(const char *key, char *value, File f) {
  char line[CONFIG_KEY_MAXLEN+CONFIG_VALUE_MAXLEN+4];
  char linekey[CONFIG_KEY_MAXLEN];
  for (uint8_t i=0; i<CONFIG_VALUE_MAXLEN; i++) value[i] = '\0';
  f.seek(0);
  while (f.available()) {
    //Read one line
    int line_length = f.readBytesUntil('\n', line, CONFIG_KEY_MAXLEN+CONFIG_VALUE_MAXLEN+4);
    if (line[strlen(key)]!='=') continue;
    //Trim (\r|\n|\t| ) chars if needed
    while (((line[line_length]=='\r')||(line[line_length]=='\n'))&&(line_length>0)) line_length--;
    //Get first strlen(key) characters of line
    strncpy(linekey, line, strlen(key)); linekey[strlen(key)] = '\0';
    //Compare with key, if it matches, value is in line 
    if (!strcmp(key, linekey)) {
      // Copy line from strlen(key)+1 to end
      uint8_t offset = strlen(key)+1;
      for (uint8_t i=0; i<(line_length-offset); i++) value[i] = line[offset+i];
      return true;
    }
  }
  return false;
}

// apply file_findKey on File opened from path
bool ESPWiFi::conf_findKey(const char *key, char *value, const char *path) {
  File f;
  bool r;
  if (f = this->conf_open(path)) r = this->file_findKey(key, value, f);
  f.close();
  return r;
}

// Try to open configuration file, from SDcard 1st, LittleFS 2nd
File ESPWiFi::conf_open(const char *path, const char *mode) {
  File f;
  // Try on SDcard
  if (SD.begin(this->sd_cs_pin)) {
    if (f = SD.open(path, mode)) {
      Serial.print("  conf file "); Serial.print(path); Serial.println(" found on SDcard");
      this->sd_mounted = true;
      return f;
    }
    SD.end();
  }
  // Try on LittleFS
  if (LittleFS.begin()) {
    if (f = LittleFS.open(path, mode)) {
      this->littlefs_mounted = true;
      Serial.print("  conf file "); Serial.print(path); Serial.println(" found on LittleFS");
      return f;
    }
    LittleFS.end();
  }
  Serial.print("  conf file "); Serial.print(path); Serial.println(" not found on SDcard/LittleFS");
  return f;
}

// Try to load configuration from SDcard/LittleFS
// If found, store configuration to preferences
bool ESPWiFi::conf_load(const char *path, bool remove) {
  File f;
  if (!(f=this->conf_open(path))) return false;
  char key[CONFIG_KEY_MAXLEN], *value, *ssid, *pass;
  String v, s, p;
  uint8_t prio;

  //Check if already loaded
  value = (char*)calloc(CONFIG_VALUE_MAXLEN, sizeof(char));
  bool loaded = false;
  if(this->file_findKey("conf_loaded", value, f)) loaded = true;
  free(value);
  if (loaded) {
    Serial.println("  configuration file has already been loaded");
    Serial.println("  remove 'conf_loaded=' key in conf to force reload");
    f.close();
    if (this->sd_mounted) SD.end();
    else if (this->littlefs_mounted) LittleFS.end();
    return true;
  }

  this->prefs.begin(this->prefs_namespace, false);

  //Get hostname
  value = (char*)calloc(CONFIG_VALUE_MAXLEN, sizeof(char));
  if (this->file_findKey("hostname", value, f) && strcmp(value, "")) {
    v = String(value); this->prefs.putString("hostname", v);
    Serial.print("  + hostname : ");
  }
  else {
    v = this->prefs.getString("hostname", String(""));
    if (v=="") {
      v = String(ESPWIFI_D_HOSTNAME);
      this->prefs.putString("hostname", v);
      Serial.println("  - hostname (default) : ");
    }
    else { Serial.print("  * hostname (keep current) : "); }
  }
  Serial.println(v);
  free(value);

  //Get WiFi credentials (max 3)
  for (uint8_t i=1; i<=3; i++) {
    sprintf(key, "wifi_ssid%d", i);
    ssid = (char*)calloc(CONFIG_VALUE_MAXLEN, sizeof(char));
    //If SSID found, get password and priority and store them
    if (this->file_findKey(key, ssid, f) && strcmp(ssid, "")) {
      this->prefs.putString(key, String(ssid));
        
      sprintf(key, "wifi_pass%d", i);
      pass = (char*)calloc(CONFIG_VALUE_MAXLEN, sizeof(char));
      if (!this->file_findKey(key, pass, f)) strcpy(pass, "");
      this->prefs.putString(key, String(pass));
        
      sprintf(key, "wifi_prio%d", i);
      value = (char*)calloc(CONFIG_VALUE_MAXLEN, sizeof(char));
      if (this->file_findKey(key, value, f)) prio = (uint8_t)atoi(value); else prio = 0;
      this->prefs.putUChar(key, prio);

      Serial.print("  + WiFi SSID "); Serial.println(ssid);
      free(pass); free(value);
    }
    //Else remove preferences
    else {
      v = this->prefs.getString(key, String(key));
      Serial.print("  - WiFi SSID "); Serial.println(v);
      this->prefs.remove(key);
      sprintf(key, "wifi_pass%d", i);
      this->prefs.remove(key);
      sprintf(key, "wifi_prio%d", i);
      this->prefs.remove(key);
    }
    free(ssid);
  }

  //Get AP credentials
  ssid = (char*)calloc(CONFIG_VALUE_MAXLEN, sizeof(char));
  if(this->file_findKey("ap_ssid", ssid, f) && strcmp(ssid, "")) {
    this->prefs.putString("ap_ssid", String(ssid));
    pass = (char*)calloc(CONFIG_VALUE_MAXLEN, sizeof(char));
    if (!this->file_findKey("ap_pass", pass, f)) strcpy(pass, "");
    this->prefs.putString("ap_pass", String(pass));
    Serial.print("  + AP SSID "); Serial.print(ssid);
    if (pass=="") Serial.println(", without password");
    else Serial.println(", password protected");
    free(pass);
  }
  else {
    Serial.print("  - AP SSID (reset) "); Serial.print(ESPWIFI_D_APSSID);
    Serial.print(" password "); Serial.println(ESPWIFI_D_APPASS);
    this->prefs.putString("ap_ssid", String(ESPWIFI_D_APSSID));
    this->prefs.putString("ap_pass", String(ESPWIFI_D_APPASS));
  }
  free(ssid);

  //Get NTP parameters
  value = (char*)calloc(CONFIG_VALUE_MAXLEN, sizeof(char));
  if (this->file_findKey("ntp_server", value, f) && strcmp(value, "")) {
    this->prefs.putString("ntp_server", String(value));
    Serial.print("  + NTP server : "); Serial.println(value);
  }
  else {
    this->prefs.putString("ntp_server", String(""));
    Serial.println("  - NTP server (reset)");
  }
  free(value);
  value = (char*)calloc(CONFIG_VALUE_MAXLEN, sizeof(char));
  if (this->file_findKey("time_offset", value, f) && strcmp(value, "")) {
    int16_t offset = (int16_t)atoi(value); this->prefs.putShort("time_offset", offset);
    Serial.print("  + NTP time offset : "); Serial.println(offset);
  }
  else {
    this->prefs.putShort("time_offset", 0);
    Serial.println("  - NTP time offset (reset) : 0");
  }
  free(value);

  this->prefs.end();

  // Check if configuration file should be removed once loaded
  value = (char*)calloc(CONFIG_VALUE_MAXLEN, sizeof(char));
  if (this->file_findKey("remove_loaded", value, f) && strcmp(value, "true")) {
    Serial.println("  Will remove conf file as specified");
    remove = true;
  }
  // If not, add a key/value to indicate that file was already loaded
  else {
    f.close();
    if (f=this->conf_open(path, FILE_WRITE)) {
      f.println("");
      f.println("# Remove this comment and following line if modified");
      f.println("conf_loaded=true");
    } else {
      Serial.println("  Can't reopen conf file for write, it will be reloaded next time...");
    }
  }
  free(value);

  f.close();
  if (this->sd_mounted) {
    if(remove) SD.remove(path);
    SD.end();
  } 
  else if (this->littlefs_mounted) {
    if (remove) LittleFS.remove(path);
    LittleFS.end();
  }
  return true;
}


// Read configuration from preferences and setup wifimulti/AP, mDNS, NTPClient
uint8_t ESPWiFi::start() {
  this->prefs.begin(this->prefs_namespace, true); //Read-only
  Serial.println("ESPWiFi starting");

  String ssid, pass;
  char key[CONFIG_KEY_MAXLEN];
  WiFi.mode(WIFI_STA);
  for (uint8_t i=1; i<=ESPWIFI_MAX_SSIDS; i++) {
    sprintf(key, "wifi_ssid%d", i); ssid = this->prefs.getString(key, String(""));
    if (ssid=="") {
      this->APlist[i-1].ssid = NULL;
      continue;
    }
    Serial.print("  ESPWiFi registering "); Serial.print(ssid);
    sprintf(key, "wifi_pass%d", i); pass = this->prefs.getString(key, String(""));
    if (pass=="") Serial.println(" without password"); else Serial.println(" password protected");
    this->APlist[i-1].ssid = strdup(ssid.c_str());
    this->APlist[i-1].passphrase = strdup(pass.c_str());
    this->APlist[i-1].priority = this->prefs.getUChar(key, (uint8_t)0);
    this->APlist[i-1].hasFailed = false;
  }
  this->CurAP = 0;

  String hostname = this->prefs.getString("hostname", ESPWIFI_D_HOSTNAME);
  WiFi.setHostname(hostname.c_str());
  if (MDNS.begin(hostname.c_str())) {
    Serial.print("  mDNS hostname set to "); Serial.print(hostname.c_str()); Serial.println("(.local)");
  }
  else { Serial.println("  mDNS failed to set hostname"); }

  this->ntp->begin();
  int16_t offset = this->prefs.getShort("time_offset", 0);
  this->ntp->setTimeOffset(offset);
  Serial.print("  NTP client configured");
  /*String ntpserver = this->prefs.getString("ntp_server", "pool.ntp.org");
  this->ntp->setPoolServerName(ntpserver.c_str());
  Serial.print(" on "); Serial.print(ntpserver.c_str());*/
  Serial.print(" (time offset="); Serial.print(offset); Serial.println(")");

  this->prefs.end();
  return true;
}


wl_status_t ESPWiFi::status() {
  wl_status_t status = WiFi.status();
  Serial.print("ESPWiFi ");
  switch(this->wifistatus) {
    case(ESPWIFI_CONNECTED):  Serial.println("connected"); break;
    case(ESPWIFI_CONNECTING): Serial.println("connecting"); return status; break;
    case(ESPWIFI_SCANNING):   Serial.println("scanning networks"); return status; break;
    default:                  Serial.println("disconnected"); break;
  }

  Serial.print("  ");
  switch(status) {
    case(WL_IDLE_STATUS):     Serial.println("still connecting"); break;
    case(WL_NO_SSID_AVAIL):   Serial.println("no SSID available"); break;
    case(WL_SCAN_COMPLETED):  Serial.println("scan completed"); break;
    case(WL_CONNECTED):       Serial.print("SSID "); Serial.print(WiFi.SSID()); Serial.print(" ("); Serial.print(WiFi.RSSI()); Serial.println("dbm)");
                              Serial.print("  IP         : "); Serial.println(WiFi.localIP());
                              Serial.print("  Gateway    : "); Serial.println(WiFi.gatewayIP());
                              Serial.print("  DNS server : "); Serial.println(WiFi.dnsIP());
                              break;
    case(WL_CONNECT_FAILED):  Serial.println("connection failed"); break;
    case(WL_CONNECTION_LOST): Serial.println("connection lost"); break;
    case(WL_DISCONNECTED):    Serial.println("disconnected"); break;
    default:                  Serial.println("unknown status"); break;
  }
  return status;
}

void ESPWiFi::connect_best(int16_t nbssid) {
  if (nbssid==0) {
    Serial.println("ESPWiFi no network available");
    this->wifistatus = ESPWIFI_DISCONNECTED;
    return;
  }
  Serial.print("ESPWiFi "); Serial.print(nbssid); Serial.println(" networks available :");
  for (uint8_t i=0; i<nbssid; i++) { Serial.print("  * "); Serial.print(WiFi.SSID(i)); Serial.print(" "); Serial.print(WiFi.RSSI(i)); Serial.println("dbm"); }
  Serial.println("ESPWiFi configured networks :");
  for (uint8_t i=0; i<ESPWIFI_MAX_SSIDS; i++) { /*if (!strcmp(this->APlist[i].ssid, "")) continue;*/ Serial.print("  * "); Serial.println(this->APlist[i].ssid); }

  uint8_t best = 255;
  int32_t best_rssi = INT_MIN;
  for (uint8_t i=0; i<ESPWIFI_MAX_SSIDS; i++) {
    if (this->APlist[i].ssid == NULL) continue;
    if (this->APlist[i].hasFailed == true) continue;
    for (int16_t j=0; j<nbssid; j++) {
      if (!strcmp(WiFi.SSID(j).c_str(), this->APlist[i].ssid)) {
        if (WiFi.RSSI(j)>best_rssi) {
          best_rssi = WiFi.RSSI(j);
          best = i;
        }
      }
    }
  }
  if (best == 255) {
    Serial.println("ESPWiFi no configured network available");
    for (uint8_t i=0; i<ESPWIFI_MAX_SSIDS; i++) this->APlist[i].hasFailed = false;
    this->wifistatus = ESPWIFI_DISCONNECTED;
    return;
  }
  Serial.print("ESPWiFi best network "); Serial.println(this->APlist[best].ssid);
  WiFi.begin(this->APlist[best].ssid, this->APlist[best].passphrase);
  this->CurAPconnect = millis();
  this->wifistatus = ESPWIFI_CONNECTING;
  return;
}

espwifi_status_t ESPWiFi::update() {
  int16_t scan_result;
  wl_status_t st;
  switch (this->wifistatus) {
    case(ESPWIFI_CONNECTED):  if (WiFi.status()!=WL_CONNECTED) this->wifistatus = ESPWIFI_DISCONNECTED;
                              else {
                                this->ntp->update();
                              }
                              break;
    case(ESPWIFI_SCANNING):   scan_result = WiFi.scanComplete();
                              if (scan_result>0) this->connect_best(scan_result);
                              else if (scan_result == WIFI_SCAN_FAILED) WiFi.scanNetworks(true, true);
                              break;
    case(ESPWIFI_CONNECTING): st = WiFi.status();
                              if (st == WL_CONNECTED) { this->wifistatus = ESPWIFI_CONNECTED; }
                              else if (st == WL_CONNECT_FAILED) {
                                this->APlist[this->CurAP].hasFailed = true;
                                this->wifistatus = ESPWIFI_DISCONNECTED;
                                this->connect_best(WiFi.scanComplete());
                              }
                              else {
                                if ((millis()-this->CurAPconnect)>5000) {
                                  Serial.println("  ESPWiFi connection timed out");
                                  this->APlist[this->CurAP].hasFailed = true;
                                  this->wifistatus = ESPWIFI_DISCONNECTED;
                                  WiFi.disconnect();
                                  this->connect_best(WiFi.scanComplete());
                                }
                               }
                              break;
    default:                  WiFi.scanNetworks(true, true);
                              this->wifistatus = ESPWIFI_SCANNING;
                              break;
  }
  return this->wifistatus;
}
