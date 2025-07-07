#ifndef SDCARD_H
#define SDCARD_H

#ifdef FILE_READ
#undef FILE_READ
#endif
#ifdef FILE_WRITE
#undef FILE_WRITE
#endif
#include <FS.h>
#include <SdFat.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <vector>

// Constants
#define BATCH_SIZE 10
#define MAX_GIF_PATH_LEN 256

// SD Card pin definitions
#define SD_CS_PIN    4    // SD card chip select pin
#define SD_MISO_PIN  21    // SD card MISO pin
#define SD_MOSI_PIN  22    // SD card MOSI pin  
#define SD_SCK_PIN   15    // SD card SCK pin

// Global SD-related variables
extern SdFs sd;
extern bool sdError;
extern std::vector<char*> gifFilePaths;
extern unsigned long total_files;
extern String current_gif;

// Batch processing variables
extern unsigned long total_gifs_count;
extern unsigned long current_batch_start;
extern bool batch_processing_complete;

// Function declarations
bool initSD(MatrixPanel_I2S_DMA *dma_display);
void listRootDirectories(MatrixPanel_I2S_DMA *dma_display);
bool countTotalGifs(MatrixPanel_I2S_DMA *dma_display);
bool loadNextGifBatch(MatrixPanel_I2S_DMA *dma_display);
void clearGifFilePaths();
void displayStatus(MatrixPanel_I2S_DMA *dma_display, const char* message, uint16_t color);
void displayStatus(MatrixPanel_I2S_DMA *dma_display, const char* line1, const char* line2, uint16_t color);

#endif