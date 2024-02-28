#ifndef _webserver_h_
#define _webserver_h_

#include <ESPAsyncWebServer.h>
#include <SD.h>
#include <LittleFS.h>

String webserverMimeType(const char *path);

void webserverNotFound(AsyncWebServerRequest *req);
void webserverInternalError(AsyncWebServerRequest *req);

String webserverFsListDirHTML(File d, uint8_t max_depth=0, String prefix="/");
void webserverFsListDir(AsyncWebServerRequest *req, File d, uint8_t max_depth=0, String prefix="/");
void webserverFs(AsyncWebServerRequest *req, const char *path);
void webserverFs(AsyncWebServerRequest *req);
void webserverDefault(AsyncWebServerRequest *req);
void webserverIndex(AsyncWebServerRequest *req);

bool webserver_setup(uint16_t port=80, uint8_t sd_cs_pin=20U);

#endif
