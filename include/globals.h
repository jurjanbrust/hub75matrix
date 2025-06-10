#ifndef _GLOBAL_
#define _GLOBAL_

#include <SdFat.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define GIF_DIR "/gifs"
#define FILESYSTEM LittleFS
#define FORMAT_LITTLEFS_IF_FAILED true

#define PANEL_RES_X 64     // Number of pixels wide of each INDIVIDUAL panel module.
#define PANEL_RES_Y 32     // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 2      // Total number of panels chained one to another horizontally only.

typedef enum {
    OFF = 0, // LED matrix is off
    PLAYING_ART, // Looping trough art
    STARTUP,
    INDEXING,
    SD_CARD_ERROR,
    NO_FILES,

} frame_status_t;

extern MatrixPanel_I2S_DMA *dma_display;
extern SdFs sd;
extern bool sdError;

extern frame_status_t target_state;
extern unsigned long lastStateChange;

extern bool interruptGif, gifsLoaded;
extern int brightness;
extern bool autoPlay;
extern bool gifPlaying;
extern bool allowNextGif;
extern bool queue_populate_requred;
extern unsigned long total_files;
extern String current_gif;

#endif