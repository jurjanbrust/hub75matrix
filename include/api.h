#ifndef API_H
#define API_H

#include <WiFiManager.h>

extern WiFiManager wm;

// Function declarations
String getContentType(String filename);
void setupAPIEndpoints();

#endif // API_H 