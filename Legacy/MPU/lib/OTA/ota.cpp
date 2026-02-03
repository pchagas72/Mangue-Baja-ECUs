#include "ota.h"

// --- Network Configuration ---
const char* host = "esp32";
const char* ssid = "MPU Mangue_Baja";
const char* password = "***REMOVED***";

// Static IP configuration for the Access Point
IPAddress local_IP(192, 168, 34, 1);
IPAddress gateway(192, 168, 34, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(1880);

/**
 * @brief Configures and starts the Wi-Fi in Access Point mode.
 * Also sets up the mDNS service.
 */
void setupnetwork(){
  // Wi-Fi Config
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);

  if(!MDNS.begin(host)) // Use MDNS to solve DNS
  {
    // http://esp32.local
    //Serial.println("Error configuring mDNS. Rebooting in 1s...");
  }
  //Serial.println("mDNS configured");
}

void handleFileRead(String path) {

  if (SPIFFS.exists(path)) { 
    File file = SPIFFS.open(path, "r"); 
    server.streamFile(file, "text/html"); 
    file.close();
  } else {
    server.send(404, "text/plain", "404: Not Found");
  }
}

void setupServerRoutes(){
    // --- Web Server Routes ---

    server.on("/", HTTP_GET, []() {
        handleFileRead("/login.html");
    });

    server.on("/serverIndex", HTTP_GET, []() {
        handleFileRead("/index.html");
    });

    server.on("/update", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
    }, []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            //Serial.printf("Update: %s\n", upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("Update Success: %u bytes\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
        }
    });

    server.begin();
}

void handleServerClient(){
    server.handleClient();
    delay(1);
}
