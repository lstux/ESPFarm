#include <Arduino.h>
#include <webserver.h>

AsyncWebServer *webserver;

#include <Sensors.h>
extern Sensors *sensors;

// Get mime type from file extension
String webserverMimeType(const char *path) {
  int offset, fname_length = strlen(path);
  for (offset=fname_length-1; offset>=0; offset--) { if (path[offset]=='.' || path[offset]=='/') break; }
  if (offset<=1) { Serial.printf("Can't get file extension for %s", path); return String("octet/stream"); }
  offset++; // dont't copy '.'
  char ext[fname_length-offset];
  for (int i=0; i<fname_length-offset; i++) ext[i] = path[offset+i];
  ext[fname_length-offset]='\0';
  if (!strcmp(ext, "css"))  return String("text/css");
  if (!strcmp(ext, "html")) return String("text/html");
  if (!strcmp(ext, "js"))   return String("text/javascript");
  if (!strcmp(ext, "json")) return String("application/json");
  if (!strcmp(ext, "png"))  return String("image/png");
  if (!strcmp(ext, "txt"))  return String("text/plain");
  if (!strcmp(ext, "csv"))  return String("text/csv");
  return String("octet/stream");
}

// 404 error page
void webserverNotFound(AsyncWebServerRequest *req) {
  return req->send(404, "text/plain", "Not found\n");
}

// 500 error page
void webserverInternalError(AsyncWebServerRequest *req) {
  return req->send(500, "text/plain", "Internal error\n");
}

String webserverFsListDirHTML(File d, uint8_t max_depth, String prefix) {
  File f; size_t size; float hsize; char unit=' ';
  String html = String("<ul>\n");
  while (f = d.openNextFile()) {
    if (!f) break;
    html += String("  <li>");
    if (f.isDirectory()) html += String("d "); else html += String("f ");
    html += String("<a href=\"")+prefix+String(f.name())+String("\">")+String(f.name())+String("</a>");
    if (f.isDirectory()) {
      html += String("</li>\n");
      if (max_depth>0) html += webserverFsListDirHTML(f, max_depth-1, prefix+String(f.name())+String("/"));
    }
    else {
      size = f.size();
      html += String(" (");
      if (size>1024) {
        if (size>(1024*1024)) { unit='M'; size = (size_t)((float)size/(1024*1024))*10; hsize = (float)size/10; }
        else                  { unit='k'; size = (size_t)((float)size/1024)*10; hsize = (float)size/10; }
        html += String(hsize) + String(unit);
      }
      else { html += String((int)f.size()); }
      html += String("B)</li>\n");
    }
  }
  html += String("</ul>\n");
  return html;
}

void webserverFsListDir(AsyncWebServerRequest *req, File d, uint8_t max_depth, String prefix) {
  if (!d.isDirectory()) return webserverInternalError(req);
  String html = webserverFsListDirHTML(d, max_depth, prefix);
  return req->send(200, "text/html", html);
}

// Serve files from SD/Littlefs if exists
void webserverFs(AsyncWebServerRequest *req, const char *path) {
  if (SD.exists(path)) { Serial.print("Webserver 200 SD:"); Serial.println(path); return req->send(SD, path, webserverMimeType(path)); }
  else if (LittleFS.exists(path)) { Serial.print("Webserver 200 LittleFS:"); Serial.println(path); return req->send(LittleFS, path, webserverMimeType(path)); }
  Serial.print("Webserver 404 FS:"); Serial.println(path);
  return webserverNotFound(req);
}

void webserverFs(AsyncWebServerRequest *req) {
  return webserverFs(req, req->url().c_str());
}

void webserverDefault(AsyncWebServerRequest *req) {
  return webserverFs(req);
}

void webserverIndex(AsyncWebServerRequest *req) {
  String html = String("<html>\n");
  html += String("  <head>\n    <title>ESPFarm</title>\n  </head>\n");
  html += String("  <body>\n    <h1>ESPFarm</h1>\n");
  File d;
  if (d = SD.open("/", FILE_READ)) {
    html += String("    <h2>SD card content</h2>\n");
    html += webserverFsListDirHTML(d, 1, "/");
    d.close();
  }
  if (d = LittleFS.open("/", FILE_READ)) {
    html += String("    <h2>LittleFS content</h2>\n");
    html += webserverFsListDirHTML(d);
    d.close();
  }
  html += String("  </body>\n");
  html += String("</html>\n");
  return req->send(200, "text/html", html);
  //return webserverFs(req, "/index.html");
}

bool webserver_setup(uint16_t port, uint8_t sd_cs_pin) {
  if (SD.begin(sd_cs_pin)) Serial.println("Webserver mounted SDFS");
  else Serial.println("Webserver failed to mount SDFS");
  if (LittleFS.begin()) Serial.println("Webserver mounted LittleFS");
  else Serial.println("Webserver failed to mount LittleFS");

  webserver = new AsyncWebServer(port);
  webserver->on("/", HTTP_ANY, webserverIndex);
  webserver->onNotFound(webserverDefault);
  webserver->begin();

  Serial.print("Webserver listening on requests port "); Serial.println(port);
  return 1;
}