#include "portal.h"
#include "globals.h"
#include "sdcard.h"

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
    res = wm.autoConnect("PixelMatrixFX","password"); // password protected ap
    if(!res) {
        Serial.println("Failed to connect");
        displayStatus(dma_display, "failed to connected...", WiFi.localIP().toString().c_str(), dma_display->color565(255, 0, 0));
        delay(3000);
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...");
        displayStatus(dma_display, "WIFI connected", WiFi.localIP().toString().c_str(), dma_display->color565(0, 255, 0));
        delay(3000);
        // Start the web API after successful WiFi connection
        setupWebAPI();
    }
}
void setupWebAPI() {
    // Root endpoint - basic info
    server.on("/", HTTP_GET, []() {
        String html = "<html><head><title>GIF Matrix Display</title></head><body>";
        html += "<h1>Pixel Matrix FX Control</h1>";
        html += "<p>Device Status: Connected to WiFi</p>";
        html += "<p>SSID: " + WiFi.SSID() + "</p>";
        html += "<p>IP Address: " + WiFi.localIP().toString() + "</p>";
        html += "<p>Current GIF: " + current_gif + "</p>";
        html += "<p>Brightness: <span id='brightness'>" + String(brightness) + "</span>/255</p>";
        html += "<h2>API Endpoints:</h2>";
        html += "<ul>";
        html += "<li><a href='/api/status'>GET /api/status</a> - Get device status</li>";
        html += "<li>GET /api/brightness/increase - Increase brightness by 25</li>";
        html += "<li>GET /api/brightness/decrease - Decrease brightness by 25</li>";
        html += "<li>GET /api/brightness/set?value=X - Set brightness to specific value (1-255)</li>";
        html += "<li><a href='/api/wifi/reset'>GET /api/wifi/reset</a> - Reset WiFi credentials</li>";
        html += "<li><a href='/api/restart'>GET /api/restart</a> - Restart device</li>";
        html += "</ul>";
        html += "<h2>Quick Actions:</h2>";
        html += "<div style='margin: 10px 0;'>";
        html += "<h3>Brightness Control:</h3>";
        html += "<label for='brightnessSlider'>Set Brightness: </label>";
        html += "<input type='range' id='brightnessSlider' min='1' max='255' value='" + String(brightness) + "' ";
        html += "oninput='updateBrightnessDisplay(this.value)' onchange='setBrightness(this.value)'>";
        html += "<span id='sliderValue'>" + String(brightness) + "</span>";
        html += "</div>";
        html += "<div style='margin: 10px 0;'>";
        html += "<h3>Device Control:</h3>";
        html += "<button onclick=\"resetWiFi()\">Reset WiFi Credentials</button>&nbsp;";
        html += "<button onclick=\"restartDevice()\">Restart Device</button>";
        html += "</div>";
        html += "<script>";
        html += "function setBrightness(value) {";
        html += "  fetch('/api/brightness/set?value=' + value, {method: 'GET'})";
        html += "    .then(r => r.json())";
        html += "    .then(d => {";
        html += "      if(d.status === 'success') {";
        html += "        document.getElementById('brightness').textContent = d.new_brightness;";
        html += "        console.log('Brightness set to: ' + d.new_brightness);";
        html += "      } else {";
        html += "        alert('Error: ' + d.message);";
        html += "      }";
        html += "    })";
        html += "    .catch(e => alert('Request failed: ' + e));";
        html += "}";
        html += "function updateBrightnessDisplay(value) {";
        html += "  document.getElementById('sliderValue').textContent = value;";
        html += "}";
        html += "function resetWiFi() {";
        html += "  if(confirm('Are you sure you want to reset WiFi credentials? Device will restart in config mode.')) {";
        html += "    fetch('/api/wifi/reset', {method: 'GET'}).then(r => r.json()).then(d => alert(d.message));";
        html += "  }";
        html += "}";
        html += "function restartDevice() {";
        html += "  if(confirm('Are you sure you want to restart the device?')) {";
        html += "    fetch('/api/restart', {method: 'GET'}).then(r => r.json()).then(d => alert(d.message));";
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

    // Brightness control endpoints
    server.on("/api/brightness/increase", HTTP_GET, []() {
        int oldBrightness = brightness;
        brightness = min(255, brightness + 25); // Increase by 25, cap at 255
        dma_display->setBrightness8(brightness);
        
        String json = "{";
        json += "\"status\":\"success\",";
        json += "\"message\":\"Brightness increased\",";
        json += "\"old_brightness\":" + String(oldBrightness) + ",";
        json += "\"new_brightness\":" + String(brightness);
        json += "}";
        server.send(200, "application/json", json);
        
        Serial.printf("Brightness increased from %d to %d\n", oldBrightness, brightness);
    });

    server.on("/api/brightness/decrease", HTTP_GET, []() {
        int oldBrightness = brightness;
        brightness = max(10, brightness - 25); // Decrease by 25, minimum at 10
        dma_display->setBrightness8(brightness);
        
        String json = "{";
        json += "\"status\":\"success\",";
        json += "\"message\":\"Brightness decreased\",";
        json += "\"old_brightness\":" + String(oldBrightness) + ",";
        json += "\"new_brightness\":" + String(brightness);
        json += "}";
        server.send(200, "application/json", json);
        
        Serial.printf("Brightness decreased from %d to %d\n", oldBrightness, brightness);
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
        
        String json = "{";
        json += "\"status\":\"success\",";
        json += "\"message\":\"Brightness set to " + String(brightness) + "\",";
        json += "\"old_brightness\":" + String(oldBrightness) + ",";
        json += "\"new_brightness\":" + String(brightness);
        json += "}";
        server.send(200, "application/json", json);
        
        Serial.printf("Brightness set from %d to %d\n", oldBrightness, brightness);
    });

    // WiFi reset endpoint
    server.on("/api/wifi/reset", HTTP_GET, []() {
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
    server.on("/api/restart", HTTP_GET, []() {
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