#include "portal.h"
#include "globals.h"

WebServer server(80);
WiFiManager wm;

void setupWifi() {
    // WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    // it is a good practice to make sure your code sets wifi mode how you want it.

    //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
    wm.setClass("invert");

    // reset settings - wipe stored credentials for testing
    // these are stored by the esp library
    // wm.resetSettings();

    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result

    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
    res = wm.autoConnect("AutoConnectAP","password"); // password protected ap
    if(!res) {
        Serial.println("Failed to connect");
        // ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...");
        
        // Start the web API after successful WiFi connection
        setupWebAPI();
    }
}

void setupWebAPI() {
    // Root endpoint - basic info
    server.on("/", HTTP_GET, []() {
        String html = "<html><head><title>GIF Matrix Display</title></head><body>";
        html += "<h1>GIF Matrix Display Control</h1>";
        html += "<p>Device Status: Connected to WiFi</p>";
        html += "<p>SSID: " + WiFi.SSID() + "</p>";
        html += "<p>IP Address: " + WiFi.localIP().toString() + "</p>";
        html += "<p>Current GIF: " + current_gif + "</p>";
        html += "<h2>API Endpoints:</h2>";
        html += "<ul>";
        html += "<li><a href='/api/status'>GET /api/status</a> - Get device status</li>";
        html += "<li><a href='/api/wifi/reset'>POST /api/wifi/reset</a> - Reset WiFi credentials</li>";
        html += "<li><a href='/api/restart'>POST /api/restart</a> - Restart device</li>";
        html += "</ul>";
        html += "<h2>Quick Actions:</h2>";
        html += "<button onclick=\"resetWiFi()\">Reset WiFi Credentials</button>&nbsp;";
        html += "<button onclick=\"restartDevice()\">Restart Device</button>";
        html += "<script>";
        html += "function resetWiFi() {";
        html += "  if(confirm('Are you sure you want to reset WiFi credentials? Device will restart in config mode.')) {";
        html += "    fetch('/api/wifi/reset', {method: 'POST'}).then(r => r.json()).then(d => alert(d.message));";
        html += "  }";
        html += "}";
        html += "function restartDevice() {";
        html += "  if(confirm('Are you sure you want to restart the device?')) {";
        html += "    fetch('/api/restart', {method: 'POST'}).then(r => r.json()).then(d => alert(d.message));";
        html += "  }";
        html += "}";
        html += "</script>";
        html += "</body></html>";
        server.send(200, "text/html", html);
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

    // WiFi reset endpoint
    server.on("/api/wifi/reset", HTTP_POST, []() {
        Serial.println("WiFi reset requested via API");
        
        wm.resetSettings(); // Reset WiFi credentials
        
        String json = "{";
        json += "\"status\":\"success\",";
        json += "\"message\":\"WiFi credentials cleared. Device will restart in 3 seconds...\"";
        server.send(200, "application/json", json + "}");
            
        Serial.println("WiFi credentials cleared successfully. Restarting...");
        delay(3000);
        ESP.restart();
    });

    // Device restart endpoint
    server.on("/api/restart", HTTP_POST, []() {
        Serial.println("Device restart requested via API");
        
        String json = "{\"status\":\"success\",\"message\":\"Device restarting in 3 seconds...\"}";
        server.send(200, "application/json", json);
        
        delay(3000);
        ESP.restart();
    });

    // Handle not found
    server.onNotFound([]() {
        String json = "{\"status\":\"error\",\"message\":\"Endpoint not found\"}";
        server.send(404, "application/json", json);
    });

    server.begin();
    Serial.println("Web API server started on port 80");
    Serial.println("Available endpoints:");
    Serial.println("  GET  / - Web interface");
    Serial.println("  GET  /api/status - Device status");
    Serial.println("  POST /api/wifi/reset - Reset WiFi credentials");
    Serial.println("  POST /api/restart - Restart device");
    Serial.print("Access at: http://");
    Serial.println(WiFi.localIP());
}

void handleWebAPIRequests() {
    server.handleClient();
}