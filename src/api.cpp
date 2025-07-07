#include "api.h"
#include "globals.h"
#include "sdcard.h"
#include "settings.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

extern AsyncWebServer server;
extern SdFat sd;
extern String current_gif;
extern int brightness;
extern MatrixPanel_I2S_DMA *dma_display;

String getContentType(String filename) {
    Serial.println("Getting content type for: " + filename);
    
    if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".gif")) return "image/gif";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    else if (filename.endsWith(".jpeg")) return "image/jpeg";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".svg")) return "image/svg+xml";
    else if (filename.endsWith(".json")) return "application/json";
    
    Serial.println("Unknown file type, using text/plain");
    return "text/plain";
}

void setupAPIEndpoints() {
    // Handle OPTIONS preflight requests for all paths (important for CORS)
    server.on("/", HTTP_OPTIONS, [](AsyncWebServerRequest *request) { // Handle OPTIONS for root
        // Serial.println("OPTIONS / called");
        request->send(204); // Send 204 No Content for preflight
    });
    server.on("/api/status", HTTP_OPTIONS, [](AsyncWebServerRequest *request) { // Handle OPTIONS for status API
        // Serial.println("OPTIONS /api/status called");
        request->send(204);
    });
    server.on("/api/brightness/increase", HTTP_OPTIONS, [](AsyncWebServerRequest *request) { // Handle OPTIONS for brightness increase
        // Serial.println("OPTIONS /api/brightness/increase called");
        request->send(204);
    });
    server.on("/api/brightness/decrease", HTTP_OPTIONS, [](AsyncWebServerRequest *request) { // Handle OPTIONS for brightness decrease
        // Serial.println("OPTIONS /api/brightness/decrease called");
        request->send(204);
    });
    server.on("/api/brightness/set", HTTP_OPTIONS, [](AsyncWebServerRequest *request) { // Handle OPTIONS for brightness set
        // Serial.println("OPTIONS /api/brightness/set called");
        request->send(204);
    });
    server.on("/api/wifi/reset", HTTP_OPTIONS, [](AsyncWebServerRequest *request) { // Handle OPTIONS for WiFi reset
        // Serial.println("OPTIONS /api/wifi/reset called");
        request->send(204);
    });
    server.on("/api/restart", HTTP_OPTIONS, [](AsyncWebServerRequest *request) { // Handle OPTIONS for restart
        // Serial.println("OPTIONS /api/restart called");
        request->send(204);
    });

    // GIF upload endpoint
    server.on("/api/gif/upload", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
        // Serial.println("OPTIONS /api/gif/upload called");
        request->send(204);
    });

    server.on("/api/gif/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
        // Serial.println("POST /api/gif/upload called");
        if (request->hasParam("filename", true)) {
            String filename = request->getParam("filename", true)->value();
            if (!filename.endsWith(".gif")) {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Only .gif files are allowed\"}");
                return;
            }
            // Success response is now sent only if no error occurred during upload
            if (!request->hasParam("upload_error", true) || request->getParam("upload_error", true)->value() != "1") {
                request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Upload complete\"}");
            }
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing filename\"}");
        }
    },
    
    // Upload handler
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        static FsFile uploadFile;
        static String uploadFilename;
        static bool uploadError = false;
        
        if (index == 0) {
            uploadError = false;
            Serial.println("UPLOAD /api/gif/upload started");
            uploadFilename = filename;
            if (!uploadFilename.endsWith(".gif")) {
                Serial.println("Rejected non-GIF upload: " + uploadFilename);
                uploadError = true;
                return;
            }
            String path = String("/gifs/") + uploadFilename;
            if (sd.exists(path.c_str())) {
                sd.remove(path.c_str());
            }
            uploadFile = sd.open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
            if (!uploadFile) {
                Serial.println("SD card error or full: " + path);
                request->send(507, "application/json", "{\"status\":\"error\",\"message\":\"SD card error or full\"}");
                uploadError = true;
                return;
            }
            Serial.println("Upload start: " + path);
        }
        
        if (uploadFile && !uploadError && len > 0) {
            int written = uploadFile.write(data, len);
            if (written != len) {
                Serial.println("Write error during upload: " + uploadFilename);
                uploadFile.close();
                request->send(507, "application/json", "{\"status\":\"error\",\"message\":\"Write error during upload\"}");
                uploadError = true;
            }
        }
        
        if (final) {
            if (uploadFile && !uploadError) {
                uploadFile.close();
                Serial.println("Upload complete: " + uploadFilename);
            }
        }
    });

    // General file upload endpoint
    server.on("/api/file/upload", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
        Serial.println("OPTIONS /api/file/upload called");
        request->send(204);
    });
    server.on("/api/file/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
        Serial.println("POST /api/file/upload called");
        if (request->hasParam("filename", true) && request->hasParam("path", true)) {
            String filename = request->getParam("filename", true)->value();
            String targetPath = request->getParam("path", true)->value();
            
            // Success response is now sent only if no error occurred during upload
            if (!request->hasParam("upload_error", true) || request->getParam("upload_error", true)->value() != "1") {
                request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Upload complete\"}");
            }
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing filename or path\"}");
        }
    },
    // Upload handler for general files
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        static FsFile uploadFile;
        static String uploadFilename;
        static String uploadPath;
        static bool uploadError = false;
        
        if (index == 0) {
            uploadError = false;
            Serial.println("UPLOAD /api/file/upload started");
            uploadFilename = filename;
            uploadPath = request->getParam("path", true)->value();
            
            // Ensure the target directory exists
            String dirPath = uploadPath;
            int lastSlash = dirPath.lastIndexOf('/');
            if (lastSlash > 0) {
                String parentDir = dirPath.substring(0, lastSlash);
                if (!sd.exists(parentDir.c_str())) {
                    if (!sd.mkdir(parentDir.c_str())) {
                        Serial.println("Failed to create directory: " + parentDir);
                        request->send(507, "application/json", "{\"status\":\"error\",\"message\":\"Failed to create directory\"}");
                        uploadError = true;
                        return;
                    }
                }
            }
            
            // Remove existing file if it exists
            if (sd.exists(uploadPath.c_str())) {
                sd.remove(uploadPath.c_str());
            }
            
            uploadFile = sd.open(uploadPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
            if (!uploadFile) {
                Serial.println("SD card error or full: " + uploadPath);
                request->send(507, "application/json", "{\"status\":\"error\",\"message\":\"SD card error or full\"}");
                uploadError = true;
                return;
            }
            Serial.println("Upload start: " + uploadPath);
        }
        
        if (uploadFile && !uploadError && len > 0) {
            int written = uploadFile.write(data, len);
            if (written != len) {
                Serial.println("Write error during upload: " + uploadFilename);
                uploadFile.close();
                request->send(507, "application/json", "{\"status\":\"error\",\"message\":\"Write error during upload\"}");
                uploadError = true;
            }
        }
        
        if (final) {
            if (uploadFile && !uploadError) {
                uploadFile.close();
                Serial.println("Upload complete: " + uploadPath);
            }
        }
    });

    // Status endpoint
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";
        json += "\"status\":\"connected\",";
        json += "\"ssid\":\"" + WiFi.SSID() + "\",";
        json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
        json += "\"current_gif\":\"" + current_gif + "\",";
        json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
        json += "\"uptime\":" + String(millis()) + ",";
        json += "\"brightness\":" + String(brightness);
        json += "}";
        request->send(200, "application/json", json);
    });

    // Brightness control endpoints
    server.on("/api/brightness/increase", HTTP_GET, [](AsyncWebServerRequest *request) {
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
        request->send(200, "application/json", json);
        
        Serial.printf("Brightness increased from %d to %d and saved to preferences\n", oldBrightness, brightness);
    });

    server.on("/api/brightness/decrease", HTTP_GET, [](AsyncWebServerRequest *request) {
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
        request->send(200, "application/json", json);
        
        Serial.printf("Brightness decreased from %d to %d and saved to preferences\n", oldBrightness, brightness);
    });

    server.on("/api/brightness/set", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("value")) {
            String json = "{\"status\":\"error\",\"message\":\"Missing 'value' parameter\"}";
            request->send(400, "application/json", json);
            return;
        }
        
        int newBrightness = request->getParam("value")->value().toInt();
        if (newBrightness < 1 || newBrightness > 255) {
            String json = "{\"status\":\"error\",\"message\":\"Brightness value must be between 1 and 255\"}";
            request->send(400, "application/json", json);
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
        request->send(200, "application/json", json);
        
        Serial.printf("Brightness set from %d to %d and saved to preferences\n", oldBrightness, brightness);
    });

    // WiFi reset endpoint
    server.on("/api/wifi/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("WiFi reset requested via API");
        wm.resetSettings();
        String json = "{";
        json += "\"status\":\"success\",";
        json += "\"message\":\"WiFi credentials cleared. Device will restart in 3 seconds...\"";
        request->send(200, "application/json", json + "}");
            
        Serial.println("WiFi credentials cleared successfully. Restarting...");
        delay(3000);
        ESP.restart();
    });

    // Device restart endpoint
    server.on("/api/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("Device restart requested via API");
        String json = "{\"status\":\"success\",\"message\":\"Device restarting in 3 seconds...\"}";
        request->send(200, "application/json", json);
        
        delay(3000);
        ESP.restart();
    });

    // List files in a directory
    server.on("/api/files", HTTP_GET, [](AsyncWebServerRequest *request) {
        String path = "/";
        if (request->hasParam("path")) path = request->getParam("path")->value();
        if (!path.startsWith("/")) path = "/" + path;
        FsFile dir = sd.open(path.c_str(), O_RDONLY);
        if (!dir || !dir.isDir()) {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Directory not found\"}");
            return;
        }
        String json = "[";
        FsFile entry;
        bool first = true;
        while ((entry = dir.openNextFile())) {
            if (!first) json += ",";
            first = false;
            
            char name[256];
            entry.getName(name, sizeof(name));
            json += "{\"name\":\"" + String(name) + "\",";
            json += "\"type\":\"" + String(entry.isDir() ? "folder" : "file") + "\",";
            if (!entry.isDir()) json += "\"size\":" + String(entry.size()) + ",";
            if (json.endsWith(",")) json.remove(json.length() - 1); // Remove trailing comma
            json += "}";
            entry.close();
        }
        json += "]";
        dir.close();
        request->send(200, "application/json", json);
    });

    // Delete file or folder
    server.on("/api/files/delete", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("plain", true)) {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing body\"}");
            return;
        }
        String body = request->getParam("plain", true)->value();
        DynamicJsonDocument doc(256);
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        String path = doc["path"] | "";
        if (!path.startsWith("/")) path = "/" + path;
        if (!sd.exists(path.c_str())) {
            request->send(404, "application/json", "{\"status\":\"error\",\"message\":\"File/folder not found\"}");
            return;
        }
        bool ok = sd.remove(path.c_str());
        if (ok) request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Deleted\"}");
        else request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Delete failed\"}");
    });

    // Rename file or folder
    server.on("/api/files/rename", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("plain", true)) {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing body\"}");
            return;
        }
        String body = request->getParam("plain", true)->value();
        DynamicJsonDocument doc(256);
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        String path = doc["path"] | "";
        String newName = doc["newName"] | "";
        if (!path.startsWith("/")) path = "/" + path;
        int lastSlash = path.lastIndexOf('/');
        String newPath = path.substring(0, lastSlash + 1) + newName;
        if (!sd.exists(path.c_str())) {
            request->send(404, "application/json", "{\"status\":\"error\",\"message\":\"File/folder not found\"}");
            return;
        }
        bool ok = sd.rename(path.c_str(), newPath.c_str());
        if (ok) request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Renamed\"}");
        else request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Rename failed\"}");
    });

    // Move file or folder
    server.on("/api/files/move", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("plain", true)) {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing body\"}");
            return;
        }
        String body = request->getParam("plain", true)->value();
        DynamicJsonDocument doc(256);
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        String path = doc["path"] | "";
        String newPath = doc["newPath"] | "";
        if (!path.startsWith("/")) path = "/" + path;
        if (!newPath.startsWith("/")) newPath = "/" + newPath;
        if (!sd.exists(path.c_str())) {
            request->send(404, "application/json", "{\"status\":\"error\",\"message\":\"File/folder not found\"}");
            return;
        }
        bool ok = sd.rename(path.c_str(), newPath.c_str());
        if (ok) request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Moved\"}");
        else request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Move failed\"}");
    });

    // Create folder
    server.on("/api/files/create-folder", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("plain", true)) {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing body\"}");
            return;
        }
        String body = request->getParam("plain", true)->value();
        DynamicJsonDocument doc(256);
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        String name = doc["name"] | "";
        String path = "/" + name;
        if (sd.exists(path.c_str())) {
            request->send(409, "application/json", "{\"status\":\"error\",\"message\":\"Folder already exists\"}");
            return;
        }
        bool ok = sd.mkdir(path.c_str());
        if (ok) request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Folder created\"}");
        else request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Create folder failed\"}");
    });

    // Rename directory
    server.on("/api/dirs/rename", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("plain", true)) {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing body\"}");
            return;
        }
        String body = request->getParam("plain", true)->value();
        DynamicJsonDocument doc(256);
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        String path = doc["path"] | "";
        String newName = doc["newName"] | "";
        if (!path.startsWith("/")) path = "/" + path;
        int lastSlash = path.lastIndexOf('/');
        String newPath = path.substring(0, lastSlash + 1) + newName;
        if (!sd.exists(path.c_str())) {
            request->send(404, "application/json", "{\"status\":\"error\",\"message\":\"Directory not found\"}");
            return;
        }
        bool ok = sd.rename(path.c_str(), newPath.c_str());
        if (ok) request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Directory renamed\"}");
        else request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Rename failed\"}");
    });

    // Delete directory (recursive)
    server.on("/api/dirs/delete", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("plain", true)) {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing body\"}");
            return;
        }
        String body = request->getParam("plain", true)->value();
        DynamicJsonDocument doc(256);
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        String path = doc["path"] | "";
        if (!path.startsWith("/")) path = "/" + path;
        if (!sd.exists(path.c_str())) {
            request->send(404, "application/json", "{\"status\":\"error\",\"message\":\"Directory not found\"}");
            return;
        }
        // Recursively delete directory
        bool ok = sd.rmdir(path.c_str());
        if (ok) request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Directory deleted\"}");
        else request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Delete failed\"}");
    });
} 