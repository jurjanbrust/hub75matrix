#include "filesystem.h"
#include "LittleFS.h"

bool initLittleFS() {
    Serial.println("Initializing LittleFS...");
    
    if (!LittleFS.begin()) {
        Serial.println("LittleFS Mount Failed");
        return false;
    }
    
    Serial.println("LittleFS mounted successfully");
    
    // List all files for debugging
    listFiles("/");
    
    return true;
}

void listFiles(const char* dirname) {
    Serial.printf("Listing directory: %s\n", dirname);
    
    File root = LittleFS.open(dirname);
    if (!root) {
        Serial.println("Failed to open directory");
        return;
    }
    
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        return;
    }
    
    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.printf("  DIR:  %s\n", file.name());
            // Recursively list subdirectories
            listFiles(file.name());
        } else {
            Serial.printf("  FILE: %s  SIZE: %d bytes\n", file.name(), file.size());
        }
        file = root.openNextFile();
    }
}

bool fileExists(const char* path) {
    return LittleFS.exists(path);
}

String readFile(const char* path) {
    if (!LittleFS.exists(path)) {
        Serial.printf("File does not exist: %s\n", path);
        return "";
    }
    
    File file = LittleFS.open(path, "r");
    if (!file) {
        Serial.printf("Failed to open file: %s\n", path);
        return "";
    }
    
    String content = file.readString();
    file.close();
    
    return content;
}

bool writeFile(const char* path, const char* content) {
    File file = LittleFS.open(path, "w");
    if (!file) {
        Serial.printf("Failed to open file for writing: %s\n", path);
        return false;
    }
    
    size_t written = file.print(content);
    file.close();
    
    if (written > 0) {
        Serial.printf("File written successfully: %s (%d bytes)\n", path, written);
        return true;
    } else {
        Serial.printf("Failed to write file: %s\n", path);
        return false;
    }
}

size_t getFileSize(const char* path) {
    if (!LittleFS.exists(path)) {
        return 0;
    }
    
    File file = LittleFS.open(path, "r");
    if (!file) {
        return 0;
    }
    
    size_t size = file.size();
    file.close();
    
    return size;
}

void formatLittleFS() {
    Serial.println("Formatting LittleFS...");
    LittleFS.format();
    Serial.println("LittleFS formatted");
}