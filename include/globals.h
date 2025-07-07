#ifndef _GLOBAL_
#define _GLOBAL_

#ifdef FILE_READ
#undef FILE_READ
#endif
#ifdef FILE_WRITE
#undef FILE_WRITE
#endif

#include <FS.h>
#include <SdFat.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define GIF_DIR "/gifs"
#define FILESYSTEM sd  // Using SD card for all file operations
#define SHOW_PROGRESS false // Show progress on the matrix display

#define PANEL_RES_X 64     // Number of pixels wide of each INDIVIDUAL panel module.
#define PANEL_RES_Y 32     // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 2      // Total number of panels chained one to another horizontally only.

#define R1_PIN 32
#define G1_PIN 33
#define B1_PIN 19
#define R2_PIN 25
#define G2_PIN 26
#define B2_PIN 18
#define A_PIN 27
#define B_PIN 5
#define C_PIN 14
#define D_PIN 17
#define E_PIN -1 // required for 1/32 scan panels, like 64x64. Any available pin would do, i.e. IO32
#define LAT_PIN 16
#define OE_PIN 13
#define CLK_PIN 12

// Default brightness value
#define DEFAULT_BRIGHTNESS 64

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
extern bool gifPlaybackEnabled;
extern unsigned long total_files;
extern String current_gif;

#endif