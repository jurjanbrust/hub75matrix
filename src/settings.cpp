#include "settings.h"
#include "globals.h"

Preferences preferences;

// Function to load brightness from preferences
void loadBrightnessFromPreferences() {
    preferences.begin("matrix_settings", false); // false = read/write mode
    brightness = preferences.getInt("brightness", DEFAULT_BRIGHTNESS);
    preferences.end();
    
    Serial.printf("Loaded brightness from preferences: %d\n", brightness);
}

// Function to save brightness to preferences
void saveBrightnessToPreferences() {
    preferences.begin("matrix_settings", false); // false = read/write mode
    preferences.putInt("brightness", brightness);
    preferences.end();
    
    Serial.printf("Saved brightness to preferences: %d\n", brightness);
}
