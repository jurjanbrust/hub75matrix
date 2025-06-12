#ifndef SD_HANDLER_H
#define SD_HANDLER_H

#include <Arduino.h>
#include <SdFat.h>
#include <vector>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h> // For MatrixPanel_I2S_DMA pointer

// Define a generous maximum path length for your GIFs. Adjust if your paths are exceptionally long.
// E.g., "/gifs/" (6 chars) + 128 chars for filename + .gif (4 chars) + null (1 char) = 139. 256 is safe.
#define MAX_GIF_PATH_LEN 256
#define MAX_GIFS_TO_LOAD 100 // Limit the number of GIFs to load to prevent memory issues

// External declarations for global SD-related variables
extern SdFs sd;
extern bool sdError;
extern std::vector<char*> gifFilePaths; // <<-- CHANGED: Now a vector of C-style strings
extern unsigned long total_files;
extern String current_gif; // Keep this as String for now, if it's used elsewhere as such.

// Function prototypes for SD card operations
bool initSD(MatrixPanel_I2S_DMA *dma_display);
void listRootDirectories(MatrixPanel_I2S_DMA *dma_display);
bool loadGifFilePaths(MatrixPanel_I2S_DMA *dma_display);
void displayStatus(MatrixPanel_I2S_DMA *dma_display, const char* message, uint16_t color);

// New utility to free the allocated paths when done (CRITICAL for memory management!)
void clearGifFilePaths();

#endif // SD_HANDLER_H