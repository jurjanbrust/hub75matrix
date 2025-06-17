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

// SD Card pin definitions
#define SD_CS_PIN    22    // SD card chip select pin
#define SD_MISO_PIN  32    // SD card MISO pin
#define SD_MOSI_PIN  21    // SD card MOSI pin  
#define SD_SCK_PIN   33    // SD card SCK pin

// Default brightness value
#define DEFAULT_BRIGHTNESS 128

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