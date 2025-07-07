#ifndef SETTINGS_H
#define SETTINGS_H

#include <Preferences.h>

extern Preferences preferences;

void loadBrightnessFromPreferences();
void saveBrightnessToPreferences();
void loadGifPlaybackFromPreferences();
void saveGifPlaybackToPreferences();

#endif
