#include "FS.h"
#include "portal.h"
#include "globals.h"
#include "sdcard.h"
#include "settings.h"
#include "LittleFS.h"
#include <ArduinoJson.h>

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

// Helper function to add CORS headers to responses
void addCorsHeaders() {
    // These headers allow requests from any origin (*)
    // and permit common HTTP methods and Content-Type header.
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type, X-Requested-With"); // X-Requested-With is common for AJAX
}

// Function to serve files from SD card
bool handleFileRead(String path) {
    Serial.println("handleFileRead: " + path);
    
    if (path.endsWith("/")) {
        path += "index.html";
    }
    // Ensure path is absolute
    if (!path.startsWith("/")) {
        path = "/" + path;
    }
    String contentType = getContentType(path);
    
    // Prepend /data to the path since that's where our web files are stored
    String fullPath = "/data" + path;
    
    if (sd.exists(fullPath.c_str())) {
        FsFile file = sd.open(fullPath.c_str(), O_RDONLY);
        if (file) {
            String content = file.readString();
            file.close();
            
            // Replace template variables with actual values
            if (path.endsWith(".html")) {
                content.replace("%CURRENT_GIF%", current_gif);
                content.replace("%SSID%", WiFi.SSID());
                content.replace("%IP%", WiFi.localIP().toString());
                content.replace("%BRIGHTNESS%", String(brightness));
            }
            
            addCorsHeaders(); // Add CORS headers before sending file content
            server.send(200, contentType, content);
            return true;
        }
    }
    
    Serial.println("File Not Found: " + fullPath);
    return false;
}

void apModeCallback(WiFiManager *myWiFiManager) {
    displayStatus(dma_display, "Setup WiFi Connect", "with: PixelMatrixFX", dma_display->color565(255, 255, 0));
}

void setupWifi() {
    // Remove LittleFS initialization since we're not using it anymore
    
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
        displayStatus(dma_display, "WIFI connected", WiFi.localIP().toString().c_str(), dma_display->color565(0, 0, 255));
        delay(3000);
        setupWebAPI();
    }
}

void setupWebAPI() {

    // Handle OPTIONS preflight requests for all paths (important for CORS)
    server.on("/", HTTP_OPTIONS, []() { // Handle OPTIONS for root
        Serial.println("OPTIONS / called");
        addCorsHeaders();
        server.send(204); // Send 204 No Content for preflight
    });
    server.on("/api/status", HTTP_OPTIONS, []() { // Handle OPTIONS for status API
        Serial.println("OPTIONS /api/status called");
        addCorsHeaders();
        server.send(204);
    });
    server.on("/api/brightness/increase", HTTP_OPTIONS, []() { // Handle OPTIONS for brightness increase
        Serial.println("OPTIONS /api/brightness/increase called");
        addCorsHeaders();
        server.send(204);
    });
    server.on("/api/brightness/decrease", HTTP_OPTIONS, []() { // Handle OPTIONS for brightness decrease
        Serial.println("OPTIONS /api/brightness/decrease called");
        addCorsHeaders();
        server.send(204);
    });
    server.on("/api/brightness/set", HTTP_OPTIONS, []() { // Handle OPTIONS for brightness set
        Serial.println("OPTIONS /api/brightness/set called");
        addCorsHeaders();
        server.send(204);
    });
    server.on("/api/wifi/reset", HTTP_OPTIONS, []() { // Handle OPTIONS for WiFi reset
        Serial.println("OPTIONS /api/wifi/reset called");
        addCorsHeaders();
        server.send(204);
    });
    server.on("/api/restart", HTTP_OPTIONS, []() { // Handle OPTIONS for restart
        Serial.println("OPTIONS /api/restart called");
        addCorsHeaders();
        server.send(204);
    });

    // GIF upload endpoint
    server.on("/api/gif/upload", HTTP_OPTIONS, []() {
        Serial.println("OPTIONS /api/gif/upload called");
        addCorsHeaders();
        server.send(204);
    });
    server.on("/api/gif/upload", HTTP_POST, []() {
        Serial.println("POST /api/gif/upload called");
        addCorsHeaders();
        if (server.hasArg("filename")) {
            String filename = server.arg("filename");
            if (!filename.endsWith(".gif")) {
                server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Only .gif files are allowed\"}");
                return;
            }
            // Success response is now sent only if no error occurred during upload
            if (!server.arg("upload_error").equals("1")) {
                server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Upload complete\"}");
            }
        } else {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing filename\"}");
        }
    },
    // Upload handler
    []() {
        HTTPUpload& upload = server.upload();
        static FsFile uploadFile;
        static String uploadFilename;
        static bool uploadError = false;
        if (upload.status == UPLOAD_FILE_START) {
            uploadError = false;
            Serial.println("UPLOAD /api/gif/upload started");
            uploadFilename = upload.filename;
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
                addCorsHeaders();
                server.send(507, "application/json", "{\"status\":\"error\",\"message\":\"SD card error or full\"}");
                uploadError = true;
                return;
            }
            Serial.println("Upload start: " + path);
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (uploadFile && !uploadError) {
                int written = uploadFile.write(upload.buf, upload.currentSize);
                if (written != upload.currentSize) {
                    Serial.println("Write error during upload: " + uploadFilename);
                    uploadFile.close();
                    addCorsHeaders();
                    server.send(507, "application/json", "{\"status\":\"error\",\"message\":\"Write error during upload\"}");
                    uploadError = true;
                }
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (uploadFile && !uploadError) {
                uploadFile.close();
                Serial.println("Upload complete: " + uploadFilename);
            }
        } else if (upload.status == UPLOAD_FILE_ABORTED) {
            if (uploadFile) {
                uploadFile.close();
                Serial.println("Upload aborted: " + uploadFilename);
            }
        }
    });

    // General file upload endpoint
    server.on("/api/file/upload", HTTP_OPTIONS, []() {
        Serial.println("OPTIONS /api/file/upload called");
        addCorsHeaders();
        server.send(204);
    });
    server.on("/api/file/upload", HTTP_POST, []() {
        Serial.println("POST /api/file/upload called");
        addCorsHeaders();
        if (server.hasArg("filename") && server.hasArg("path")) {
            String filename = server.arg("filename");
            String targetPath = server.arg("path");
            
            // Success response is now sent only if no error occurred during upload
            if (!server.arg("upload_error").equals("1")) {
                server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Upload complete\"}");
            }
        } else {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing filename or path\"}");
        }
    },
    // Upload handler for general files
    []() {
        HTTPUpload& upload = server.upload();
        static FsFile uploadFile;
        static String uploadFilename;
        static String uploadPath;
        static bool uploadError = false;
        
        if (upload.status == UPLOAD_FILE_START) {
            uploadError = false;
            Serial.println("UPLOAD /api/file/upload started");
            uploadFilename = upload.filename;
            uploadPath = server.arg("path");
            
            // Ensure the target directory exists
            String dirPath = uploadPath;
            int lastSlash = dirPath.lastIndexOf('/');
            if (lastSlash > 0) {
                String parentDir = dirPath.substring(0, lastSlash);
                if (!sd.exists(parentDir.c_str())) {
                    if (!sd.mkdir(parentDir.c_str())) {
                        Serial.println("Failed to create directory: " + parentDir);
                        addCorsHeaders();
                        server.send(507, "application/json", "{\"status\":\"error\",\"message\":\"Failed to create directory\"}");
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
                addCorsHeaders();
                server.send(507, "application/json", "{\"status\":\"error\",\"message\":\"SD card error or full\"}");
                uploadError = true;
                return;
            }
            Serial.println("Upload start: " + uploadPath);
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (uploadFile && !uploadError) {
                int written = uploadFile.write(upload.buf, upload.currentSize);
                if (written != upload.currentSize) {
                    Serial.println("Write error during upload: " + uploadFilename);
                    uploadFile.close();
                    addCorsHeaders();
                    server.send(507, "application/json", "{\"status\":\"error\",\"message\":\"Write error during upload\"}");
                    uploadError = true;
                }
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (uploadFile && !uploadError) {
                uploadFile.close();
                Serial.println("Upload complete: " + uploadPath);
            }
        } else if (upload.status == UPLOAD_FILE_ABORTED) {
            if (uploadFile) {
                uploadFile.close();
                Serial.println("Upload aborted: " + uploadPath);
            }
        }
    });

    // Serve static files from SD card
    server.onNotFound([]() {
        Serial.println("404 Not Found: " + server.uri());
        if (!handleFileRead(server.uri())) {
            String json = "{\"status\":\"error\",\"message\":\"File not found\"}";
            server.send(404, "application/json", json);
        }
    });
    
    // Root endpoint - serve index.html from SD card
    server.on("/", HTTP_GET, []() {
        Serial.println("GET / called");
        if (!handleFileRead("/index.html")) {
            server.send(500, "text/plain", "Failed to load index.html");
        }
    });

    // Status endpoint
    server.on("/api/status", HTTP_GET, []() {
        Serial.println("GET /api/status called");
        addCorsHeaders();
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
        Serial.println("GET /api/brightness/increase called");
        int oldBrightness = brightness;
        brightness = min(255, brightness + 25);
        dma_display->setBrightness8(brightness);
        saveBrightnessToPreferences();
        addCorsHeaders();
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
        Serial.println("GET /api/brightness/decrease called");
        int oldBrightness = brightness;
        brightness = max(10, brightness - 25);
        dma_display->setBrightness8(brightness);
        saveBrightnessToPreferences();
        addCorsHeaders();
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
        Serial.println("GET /api/brightness/set called");
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
        addCorsHeaders();
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
        Serial.println("GET /api/wifi/reset called");
        Serial.println("WiFi reset requested via API");
        
        wm.resetSettings();
        addCorsHeaders();
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
        Serial.println("GET /api/restart called");
        Serial.println("Device restart requested via API");
        addCorsHeaders();
        String json = "{\"status\":\"success\",\"message\":\"Device restarting in 3 seconds...\"}";
        server.send(200, "application/json", json);
        
        delay(3000);
        ESP.restart();
    });

    // List files in a directory
    server.on("/api/files", HTTP_GET, []() {
        addCorsHeaders();
        String path = "/";
        if (server.hasArg("path")) path = server.arg("path");
        if (!path.startsWith("/")) path = "/" + path;
        FsFile dir = sd.open(path.c_str(), O_RDONLY);
        if (!dir || !dir.isDir()) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Directory not found\"}");
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
        server.send(200, "application/json", json);
    });

    // Delete file or folder
    server.on("/api/files/delete", HTTP_POST, []() {
        addCorsHeaders();
        if (!server.hasArg("plain")) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing body\"}");
            return;
        }
        String body = server.arg("plain");
        DynamicJsonDocument doc(256);
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        String path = doc["path"] | "";
        if (!path.startsWith("/")) path = "/" + path;
        if (!sd.exists(path.c_str())) {
            server.send(404, "application/json", "{\"status\":\"error\",\"message\":\"File/folder not found\"}");
            return;
        }
        bool ok = sd.remove(path.c_str());
        if (ok) server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Deleted\"}");
        else server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Delete failed\"}");
    });

    // Rename file or folder
    server.on("/api/files/rename", HTTP_POST, []() {
        addCorsHeaders();
        if (!server.hasArg("plain")) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing body\"}");
            return;
        }
        String body = server.arg("plain");
        DynamicJsonDocument doc(256);
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        String path = doc["path"] | "";
        String newName = doc["newName"] | "";
        if (!path.startsWith("/")) path = "/" + path;
        int lastSlash = path.lastIndexOf('/');
        String newPath = path.substring(0, lastSlash + 1) + newName;
        if (!sd.exists(path.c_str())) {
            server.send(404, "application/json", "{\"status\":\"error\",\"message\":\"File/folder not found\"}");
            return;
        }
        bool ok = sd.rename(path.c_str(), newPath.c_str());
        if (ok) server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Renamed\"}");
        else server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Rename failed\"}");
    });

    // Move file or folder
    server.on("/api/files/move", HTTP_POST, []() {
        addCorsHeaders();
        if (!server.hasArg("plain")) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing body\"}");
            return;
        }
        String body = server.arg("plain");
        DynamicJsonDocument doc(256);
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        String path = doc["path"] | "";
        String newPath = doc["newPath"] | "";
        if (!path.startsWith("/")) path = "/" + path;
        if (!newPath.startsWith("/")) newPath = "/" + newPath;
        if (!sd.exists(path.c_str())) {
            server.send(404, "application/json", "{\"status\":\"error\",\"message\":\"File/folder not found\"}");
            return;
        }
        bool ok = sd.rename(path.c_str(), newPath.c_str());
        if (ok) server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Moved\"}");
        else server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Move failed\"}");
    });

    // Create folder
    server.on("/api/files/create-folder", HTTP_POST, []() {
        addCorsHeaders();
        if (!server.hasArg("plain")) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing body\"}");
            return;
        }
        String body = server.arg("plain");
        DynamicJsonDocument doc(256);
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        String name = doc["name"] | "";
        String path = "/" + name;
        if (sd.exists(path.c_str())) {
            server.send(409, "application/json", "{\"status\":\"error\",\"message\":\"Folder already exists\"}");
            return;
        }
        bool ok = sd.mkdir(path.c_str());
        if (ok) server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Folder created\"}");
        else server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Create folder failed\"}");
    });

    // Rename directory
    server.on("/api/dirs/rename", HTTP_POST, []() {
        addCorsHeaders();
        if (!server.hasArg("plain")) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing body\"}");
            return;
        }
        String body = server.arg("plain");
        DynamicJsonDocument doc(256);
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        String path = doc["path"] | "";
        String newName = doc["newName"] | "";
        if (!path.startsWith("/")) path = "/" + path;
        int lastSlash = path.lastIndexOf('/');
        String newPath = path.substring(0, lastSlash + 1) + newName;
        if (!sd.exists(path.c_str())) {
            server.send(404, "application/json", "{\"status\":\"error\",\"message\":\"Directory not found\"}");
            return;
        }
        bool ok = sd.rename(path.c_str(), newPath.c_str());
        if (ok) server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Directory renamed\"}");
        else server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Rename failed\"}");
    });

    // Delete directory (recursive)
    server.on("/api/dirs/delete", HTTP_POST, []() {
        addCorsHeaders();
        if (!server.hasArg("plain")) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing body\"}");
            return;
        }
        String body = server.arg("plain");
        DynamicJsonDocument doc(256);
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        String path = doc["path"] | "";
        if (!path.startsWith("/")) path = "/" + path;
        if (!sd.exists(path.c_str())) {
            server.send(404, "application/json", "{\"status\":\"error\",\"message\":\"Directory not found\"}");
            return;
        }
        // Recursively delete directory
        bool ok = sd.rmdir(path.c_str());
        if (ok) server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Directory deleted\"}");
        else server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Delete failed\"}");
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
    Serial.println("  POST /api/gif/upload - Upload GIF");
    Serial.println("  POST /api/file/upload - Upload any file with path");
    Serial.println("  GET /api/files - List files in directory");
    Serial.println("  POST /api/files/delete - Delete file or folder");
    Serial.println("  POST /api/files/rename - Rename file or folder");
    Serial.println("  POST /api/files/move - Move file or folder");
    Serial.println("  POST /api/files/create-folder - Create new folder");
    Serial.println("  POST /api/dirs/rename - Rename directory");
    Serial.println("  POST /api/dirs/delete - Delete directory");
    Serial.print("Access at: http://");
    Serial.println(WiFi.localIP());
}

void handleWebAPIRequests() {
    server.handleClient();
}