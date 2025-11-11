#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include "SPIFFS.h"

void setupnetwork();
void setupServerRoutes();
void handleFileRead(String path);
void handleServerClient();

#endif