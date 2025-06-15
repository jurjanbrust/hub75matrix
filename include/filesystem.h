#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <Arduino.h>
#include "LittleFS.h"

// Initialize LittleFS
bool initLittleFS();

// List all files in directory (for debugging)
void listFiles(const char* dirname);

// Check if file exists
bool fileExists(const char* path);

// Read file content as String
String readFile(const char* path);

// Write content to file
bool writeFile(const char* path, const char* content);

// Get file size
size_t getFileSize(const char* path);

// Format LittleFS (WARNING: This will delete all files!)
void formatLittleFS();

#endif // FILESYSTEM_H