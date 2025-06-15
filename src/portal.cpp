#include "portal.h"
#include "globals.h"
#include "sdcard.h"
#include "settings.h"
#include "LittleFS.h"

WebServer server(80);
WiFiManager wm;

// Function to get content type based on file extension
String getContentType(String filename) {
    if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".json")) return "application/json";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    else if (filename.endsWith(".gif")) return "image/gif";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    return "text/plain";
}

// Function to serve files from LittleFS
bool handleFileRead(String path) {
    Serial.println("handleFileRead: " + path);
    
    if (path.endsWith("/")) {
        path += "index.html";
    }
    
    String contentType = getContentType(path);
    
    if (LittleFS.exists(path)) {
        File file = LittleFS.open(path, "r");
        if (file) {
            String content = file.readString();
            file.close();
            
            // Replace template variables with actual values
            if (path.endsWith(".html")) {
                content.replace("%SSID%", WiFi.SSID());
                content.replace("%IP%", WiFi.localIP().toString());
                content.replace("%CURRENT_GIF%", current_gif);
                content.replace("%BRIGHTNESS%", String(brightness));
            }
            
            server.send(200, contentType, content);
            return true;
        }
    }
    
    Serial.println("File Not Found: " + path);
    return false;
}

void setupWifi() {
    // Initialize LittleFS
    if (!LittleFS.begin()) {
        Serial.println("LittleFS Mount Failed");
        return;
    }
    Serial.println("LittleFS mounted successfully");
    
    // List files in LittleFS for debugging
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    Serial.println("Files in LittleFS:");
    while (file) {
        Serial.print("  FILE: ");
        Serial.print(file.name());
        Serial.print("  SIZE: ");
        Serial.println(file.size());
        file = root.openNextFile();
    }
    
    wm.setClass("invert");

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
        displayStatus(dma_display, "WIFI connected", WiFi.localIP().toString().c_str(), dma_display->color565(0, 0, 255));
        delay(3000);
        setupWebAPI();
    }
}

void setupWebAPI() {
    // Serve static files from LittleFS
    server.onNotFound([]() {
        if (!handleFileRead(server.uri())) {
            String json = "{\"status\":\"error\",\"message\":\"File not found\"}";
            server.send(404, "application/json", json);
        }
    });
    
    // Root endpoint - serve index.html from LittleFS
    server.on("/", HTTP_GET, []() {
        if (!handleFileRead("/index.html")) {
            server.send(500, "text/plain", "Failed to load index.html");
        }
    });

    // Status endpoint
    server.on("/api/status", HTTP_GET, []() {
        String json = "{";
        json += "\"status\":\"connected\",";
        json += "\"ssid\":\"" + WiFi.SSID() + "\",";
        json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
        json += "\"current_gif\":\"" + current_gif + "\",";
        json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
        json += "\"uptime\":" + String(millis()) + ",";
        json += "\"brightness\":" + String(brightness);
        json += "}";
        server.send(200, "application/json", json);
    });

    // Brightness control endpoints
    server.on("/api/brightness/increase", HTTP_GET, []() {
        int oldBrightness = brightness;
        brightness = min(255, brightness + 25);
        dma_display->setBrightness8(brightness);
        saveBrightnessToPreferences();
        
        String json = "{";
        json += "\"status\":\"success\",";
        json += "\"message\":\"Brightness increased and saved\",";
        json += "\"old_brightness\":" + String(oldBrightness) + ",";
        json += "\"new_brightness\":" + String(brightness);
        json += "}";
        server.send(200, "application/json", json);
        
        Serial.printf("Brightness increased from %d to %d and saved to preferences\n", oldBrightness, brightness);
    });

    server.on("/api/brightness/decrease", HTTP_GET, []() {
        int oldBrightness = brightness;
        brightness = max(10, brightness - 25);
        dma_display->setBrightness8(brightness);
        saveBrightnessToPreferences();
        
        String json = "{";
        json += "\"status\":\"success\",";
        json += "\"message\":\"Brightness decreased and saved\",";
        json += "\"old_brightness\":" + String(oldBrightness) + ",";
        json += "\"new_brightness\":" + String(brightness);
        json += "}";
        server.send(200, "application/json", json);
        
        Serial.printf("Brightness decreased from %d to %d and saved to preferences\n", oldBrightness, brightness);
    });

    server.on("/api/brightness/set", HTTP_GET, []() {
        if (!server.hasArg("value")) {
            String json = "{\"status\":\"error\",\"message\":\"Missing 'value' parameter\"}";
            server.send(400, "application/json", json);
            return;
        }
        
        int newBrightness = server.arg("value").toInt();
        if (newBrightness < 1 || newBrightness > 255) {
            String json = "{\"status\":\"error\",\"message\":\"Brightness value must be between 1 and 255\"}";
            server.send(400, "application/json", json);
            return;
        }
        
        int oldBrightness = brightness;
        brightness = newBrightness;
        dma_display->setBrightness8(brightness);
        saveBrightnessToPreferences();
        
        String json = "{";
        json += "\"status\":\"success\",";
        json += "\"message\":\"Brightness set to " + String(brightness) + " and saved\",";
        json += "\"old_brightness\":" + String(oldBrightness) + ",";
        json += "\"new_brightness\":" + String(brightness);
        json += "}";
        server.send(200, "application/json", json);
        
        Serial.printf("Brightness set from %d to %d and saved to preferences\n", oldBrightness, brightness);
    });

    // WiFi reset endpoint
    server.on("/api/wifi/reset", HTTP_GET, []() {
        Serial.println("WiFi reset requested via API");
        
        wm.resetSettings();
        
        String json = "{";
        json += "\"status\":\"success\",";
        json += "\"message\":\"WiFi credentials cleared. Device will restart in 3 seconds...\"";
        server.send(200, "application/json", json + "}");
            
        Serial.println("WiFi credentials cleared successfully. Restarting...");
        delay(3000);
        ESP.restart();
    });

    // Device restart endpoint
    server.on("/api/restart", HTTP_GET, []() {
        Serial.println("Device restart requested via API");
        
        String json = "{\"status\":\"success\",\"message\":\"Device restarting in 3 seconds...\"}";
        server.send(200, "application/json", json);
        
        delay(3000);
        ESP.restart();
    });

    server.begin();
    Serial.println("Web API server started on port 80");
    Serial.println("Available endpoints:");
    Serial.println("  GET  / - Web interface");
    Serial.println("  GET  /api/status - Device status");
    Serial.println("  GET /api/brightness/increase - Increase brightness");
    Serial.println("  GET /api/brightness/decrease - Decrease brightness");
    Serial.println("  GET /api/brightness/set?value=X - Set brightness");
    Serial.println("  GET /api/wifi/reset - Reset WiFi credentials");
    Serial.println("  GET /api/restart - Restart device");
    Serial.print("Access at: http://");
    Serial.println(WiFi.localIP());
}

void handleWebAPIRequests() {
    server.handleClient();
}