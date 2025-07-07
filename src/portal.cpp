#include "FS.h"
#include "portal.h"
#include "globals.h"
#include "sdcard.h"
#include "settings.h"
#include "api.h"
#include "LittleFS.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);
WiFiManager wm;

void apModeCallback(WiFiManager *myWiFiManager) {
    displayStatus(dma_display, "Setup WiFi Connect", "with: PixelMatrixFX", dma_display->color565(255, 255, 0));
}

void setupWifi() {
    // Initialize LittleFS for web files
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS initialization failed!");
        // Fall back to SD card for everything if LittleFS fails
    } else {
        Serial.println("LittleFS initialized successfully");
    }
    
    wm.setClass("invert");
    wm.setAPCallback(apModeCallback);

    // WiFi connection setup
    bool res;
    res = wm.autoConnect("PixelMatrixFX","password");
    if(!res) {
        Serial.println("Failed to connect");
        displayStatus(dma_display, "failed to connected...", WiFi.localIP().toString().c_str(), dma_display->color565(255, 0, 0));
        delay(3000);
    } 
    else {
        Serial.println("connected...");
        displayStatus(dma_display, "WIFI connected", WiFi.localIP().toString().c_str(), dma_display->color565(173, 216, 230));
        delay(3000);
        setupWebAPI();
    }
}

void setupWebAPI() {
    // Setup API endpoints
    setupAPIEndpoints();

    // Root endpoint - serve index.html from LittleFS
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("GET / called - serving index.html");
        String path = "/index.html";
        
        if (LittleFS.exists(path)) {
            File file = LittleFS.open(path, "r");
            if (file) {
                String content = file.readString();
                file.close();
                
                // Replace template variables with actual values
                content.replace("%CURRENT_GIF%", current_gif);
                content.replace("%SSID%", WiFi.SSID());
                content.replace("%IP%", WiFi.localIP().toString());
                content.replace("%BRIGHTNESS%", String(brightness));
                
                // Add CORS headers and send response
                AsyncWebServerResponse *response = request->beginResponse(200, "text/html", content);
                response->addHeader("Access-Control-Allow-Origin", "*");
                response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
                response->addHeader("Access-Control-Allow-Headers", "Content-Type, X-Requested-With");
                request->send(response);
            } else {
                Serial.println("index.html not found at: " + path);
                request->send(500, "text/plain", "Failed to open index.html");
            }
        } else {
            Serial.println("index.html not found at: " + path);
            request->send(404, "text/plain", "index.html not found");
        }
    });

    // Serve static files (CSS, JS, images) from LittleFS using built-in handler
    server.serveStatic("/", LittleFS, "/", "max-age=86400");

    server.begin();
    Serial.println("Web API server started on port 80");
    Serial.print("Access at: http://");
    Serial.println(WiFi.localIP());
}