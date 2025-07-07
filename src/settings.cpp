#include "FS.h"
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

// Function to load GIF playback state from preferences
void loadGifPlaybackFromPreferences() {
    preferences.begin("matrix_settings", false); // false = read/write mode
    gifPlaybackEnabled = preferences.getBool("gif_playback", true); // Default to enabled
    preferences.end();
    
    Serial.printf("Loaded GIF playback state from preferences: %s\n", gifPlaybackEnabled ? "enabled" : "disabled");
}

// Function to save GIF playback state to preferences
void saveGifPlaybackToPreferences() {
    preferences.begin("matrix_settings", false); // false = read/write mode
    preferences.putBool("gif_playback", gifPlaybackEnabled);
    preferences.end();
    
    Serial.printf("Saved GIF playback state to preferences: %s\n", gifPlaybackEnabled ? "enabled" : "disabled");
}
