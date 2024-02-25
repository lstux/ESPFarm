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
};

// Search key in File and store to value if found
bool ESPWiFi::file_findKey(const char *key, char *value, File f) {
  char line[CONFIG_KEY_MAXLEN+CONFIG_VALUE_MAXLEN+4];
  char linekey[CONFIG_KEY_MAXLEN];
  for (uint8_t i=0; i<CONFIG_VALUE_MAXLEN; i++) value[i] = '\0';
  f.seek(0);
  while (f.available()) {
    //Read one line
    int line_length = f.readBytesUntil('\n', line, CONFIG_KEY_MAXLEN+CONFIG_VALUE_MAXLEN+4);
    //Trim (\r|\n|\t| ) chars if needed
    while (((line[line_length]=='\r')||(line[line_length]=='\n')||(line[line_length]=='\t')||(line[line_length]==' '))&&(line_length>0)) line_length--;
    if (line[strlen(key)]!='=') continue;
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
File ESPWiFi::conf_open(const char *path) {
  File f;
  Serial.print("Conf file "); Serial.print(path);
  if (SD.begin(this->sd_cs_pin)) {
    if (f = SD.open(path, FILE_READ)) {
      Serial.println(" found on SDcard");
      this->sd_mounted = true;
      return f;
    }
    SD.end();
  }
  if (LittleFS.begin()) {
    if (f = LittleFS.open(path, FILE_READ)) {
      this->littlefs_mounted = true;
      Serial.println(" found on LittleFS");
      return f;
    }
    LittleFS.end();
  }
  Serial.println(" not found on SDcard/LittleFS");
  return f;
}

// Try to load configuration from SDcard/LittleFS
// If found, store configuration to preferences
bool ESPWiFi::conf_load(const char *path, bool remove) {
  File f;
  if (!(f=this->conf_open(path))) return false;
  this->prefs.begin(this->prefs_namespace, false);

  char key[CONFIG_KEY_MAXLEN], *value, *ssid, *pass;
  String v, s, p;
  uint8_t prio;

  //Get hostname
  value = (char*)calloc(CONFIG_VALUE_MAXLEN, sizeof(char));
  if (this->file_findKey("hostname", value, f) && strcmp(value, "")) {
    v = String(value); this->prefs.putString("hostname", v);
    Serial.print("  Stored hostname : ");
  }
  else {
    v = this->prefs.getString("hostname", String(""));
    if (v=="") {
      v = String(ESPWIFI_D_HOSTNAME);
      this->prefs.putString("hostname", v);
      Serial.println("  Setting default hostname : ");
    }
    else { Serial.print("  Keeping stored hostname : "); }
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

      Serial.print("  Registering WiFi "); Serial.print(ssid);
      if (!strcmp(pass, "")) Serial.print(", no password"); else { Serial.print(", with password "); }
      Serial.print(", priority "); Serial.println(prio);
      free(pass); free(value);
    }
    //Else remove preferences
    else {
      v = this->prefs.getString(key, String(key));
      Serial.print("  Removing WiFi network ");
      Serial.println(v);
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
    Serial.print("  Setting AP SSID to ");
    Serial.print(ssid);
    if (pass=="") Serial.println(", without password");
    else Serial.println(", password protected");
    free(pass);
  }
  else {
    Serial.print("  Resetting AP SSID to "); Serial.print(ESPWIFI_D_APSSID);
    Serial.print(" password "); Serial.println(ESPWIFI_D_APPASS);
    this->prefs.putString("ap_ssid", String(ESPWIFI_D_APSSID));
    this->prefs.putString("ap_pass", String(ESPWIFI_D_APPASS));
  }
  free(ssid);

  //Get NTP parameters
  value = (char*)calloc(CONFIG_VALUE_MAXLEN, sizeof(char));
  if (this->file_findKey("ntp_server", value, f) && strcmp(value, "")) {
    this->prefs.putString("ntp_server", String(value));
    Serial.print("  NTP server address stored : "); Serial.println(value);
  }
  else {
    this->prefs.putString("ntp_server", String(""));
    Serial.println("  Resetting NTP server address");
  }
  free(value);
  value = (char*)calloc(CONFIG_VALUE_MAXLEN, sizeof(char));
  if (this->file_findKey("time_offset", value, f) && strcmp(value, "")) {
    int16_t offset = (int16_t)atoi(value); this->prefs.putShort("time_offset", offset);
    Serial.print("  Stored NTP time offset : "); Serial.println(offset);
  }
  else {
    this->prefs.putShort("time_offset", 0);
    Serial.println("  NTP time offset set to default value 0");
  }
  free(value);


  value = (char*)calloc(CONFIG_VALUE_MAXLEN, sizeof(char));
  if (this->file_findKey("remove_loaded", value, f) && strcmp(value, "true")) {
    Serial.println("  Will remove conf file as specified");
    remove = true;
  }
  free(value);

  this->prefs.end();
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


// Read configuration from preferences and setup wifimulti/AP and mDNS
uint8_t ESPWiFi::start() {
  this->prefs.begin(this->prefs_namespace, true); //Read-only
  Serial.println("ESPWiFi starting");

  String ssid, pass;
  char key[CONFIG_KEY_MAXLEN];
  WiFi.mode(WIFI_STA);
  for (uint8_t i=1; i<=3; i++) {
    sprintf(key, "wifi_ssid%d", i); ssid = this->prefs.getString(key, String(""));
    if (ssid=="") continue;
    Serial.print("  WiFiMulti registering "); Serial.print(ssid);
    sprintf(key, "wifi_pass%d", i); pass = this->prefs.getString(key, String(""));
    if (pass=="") Serial.println(" without password"); else Serial.println(" password protected");
    this->w.addAP(ssid.c_str(), pass.c_str());
  }
  this->w.run();
  Serial.print("WiFi connecting...");
  uint8_t i=0;
  while (WiFi.status()!=WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    i++;
    if (i==10) { Serial.println("failed"); return false; }
  }
  Serial.println("ok");
  this->debug();

  String hostname = this->prefs.getString("hostname", ESPWIFI_D_HOSTNAME);
  WiFi.setHostname(hostname.c_str());
  if (MDNS.begin(hostname.c_str())) {
    Serial.print("  mDNS hostname set to "); Serial.print(hostname.c_str()); Serial.println("(.local)");
  }
  else { Serial.println("  mDNS failed to set hostname"); }

  this->ntp->begin();
  int16_t offset = this->prefs.getShort("time_offset", 0);
  this->ntp->setTimeOffset(offset);
  String ntpserver = this->prefs.getString("ntp_server", "pool.ntp.org").c_str();
  this->ntp->setPoolServerName(ntpserver.c_str());
  Serial.print("  NTP client configured on "); Serial.print(ntpserver.c_str());
  Serial.print(" (time offset="); Serial.print(offset); Serial.println(")");
  this->ntp->update();

  this->prefs.end();
  return true;
}

void ESPWiFi::debug() {
  if (WiFi.status()==WL_CONNECTED) {
    Serial.print("WiFi connected : ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("WiFi not connected");
  }
}