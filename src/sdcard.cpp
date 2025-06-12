#include "sdcard.h"
#include "globals.h" // For SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN, GIF_DIR, frame_status_t, SD_CARD_ERROR, NO_FILES, PLAYING_ART
#include <SPI.h>

// Global SD-related variables definitions
SdFs sd;
bool sdError = false;
std::vector<char*> gifFilePaths; // Vector of C-style strings for current batch
unsigned long total_files = 0;
String current_gif = "";

// Batch processing variables
unsigned long total_gifs_count = 0;
unsigned long current_batch_start = 0;
bool batch_processing_complete = false;

// Helper function for displaying status on the matrix display
void displayStatus(MatrixPanel_I2S_DMA *dma_display, const char* message, uint16_t color) {
    if (dma_display == nullptr) return;

    dma_display->fillScreen(dma_display->color565(0, 0, 0));
    dma_display->setCursor(10, 10);
    dma_display->setTextColor(color);
    dma_display->setTextSize(1);
    dma_display->print(message);
}

// Utility to clear the paths and free memory
void clearGifFilePaths() {
    for (char* path : gifFilePaths) {
        delete[] path; // Free each individual char array allocated with new[]
    }
    gifFilePaths.clear(); // Clear the vector itself
}

// Function to initialize the SD card
bool initSD(MatrixPanel_I2S_DMA *dma_display) {
    Serial.println("Initializing SD card...");
    displayStatus(dma_display, "Init SD...", dma_display->color565(255, 255, 255));

    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

    if (!sd.begin(SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(10)))) {
        Serial.println("SD card initialization failed! Trying alternative initialization...");
        if (!sd.begin(SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(4)))) {
            Serial.println("SD card initialization failed completely!");
            Serial.println("Check your wiring:");
            Serial.printf("CS: %d, SCK: %d, MISO: %d, MOSI: %d\n", SD_CS_PIN, SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN);
            sdError = true;
            displayStatus(dma_display, "SD Error!", dma_display->color565(255, 0, 0));
            return false;
        } else {
            Serial.println("SD card initialized with slower speed!");
        }
    } else {
        Serial.println("SD card initialized successfully!");
    }

    displayStatus(dma_display, "SD OK", dma_display->color565(0, 255, 0));
    delay(1000);
    return true;
}

// Function to list all directories on the root folder
void listRootDirectories(MatrixPanel_I2S_DMA *dma_display) {
    Serial.println("\nListing directories on SD card root:");
    displayStatus(dma_display, "List Dirs...", dma_display->color565(255, 255, 255));
    delay(1000);

    FsFile rootDir = sd.open("/");
    if (!rootDir) {
        Serial.println("Failed to open root directory!");
        displayStatus(dma_display, "No Root Dir!", dma_display->color565(255, 0, 0));
        delay(2000);
        return;
    } else {
        FsFile entry;
        int dirCount = 0;
        unsigned long lastYieldTime = millis();
        while (entry.openNext(&rootDir, O_RDONLY)) {
            if (entry.isDirectory()) {
                char entryName[256];
                entry.getName(entryName, sizeof(entryName));
                Serial.printf("  Found directory: %s\n", entryName);
                dirCount++;
            }
            entry.close();

            if (millis() - lastYieldTime > 50) {
                yield();
                lastYieldTime = millis();
            }
        }
        if (dirCount == 0) {
            Serial.println("  No directories found on root.");
            displayStatus(dma_display, "No Dirs Found", dma_display->color565(255, 255, 0));
        } else {
            Serial.printf("Found %d directories.\n", dirCount);
            char msg[32];
            sprintf(msg, "Found %d Dirs", dirCount);
            displayStatus(dma_display, msg, dma_display->color565(0, 255, 0));
        }
        rootDir.close();
    }
    Serial.println("Finished listing directories.");
    delay(2000);
}

// Function to count total number of GIF files without loading them into memory
bool countTotalGifs(MatrixPanel_I2S_DMA *dma_display) {
    Serial.println("Counting total GIF files...");
    displayStatus(dma_display, "Counting GIFs...", dma_display->color565(255, 255, 255));
    
    FsFile gifRoot = sd.open(GIF_DIR);
    if (!gifRoot) {
        Serial.println("Failed to open /gifs directory on SD card!");
        displayStatus(dma_display, "No /gifs Dir", dma_display->color565(255, 0, 0));
        return false;
    }

    FsFile file;
    total_gifs_count = 0;
    unsigned long lastYieldTime = millis();

    while (file.openNext(&gifRoot, O_RDONLY)) {
        yield();
        if (file.isFile()) {
            char fileName[256];
            file.getName(fileName, sizeof(fileName));
            
            String fileNameStr = String(fileName);
            fileNameStr.toLowerCase();
            if (fileNameStr.endsWith(".gif")) {
                total_gifs_count++;
            }
        }
        file.close();

        if (millis() - lastYieldTime > 50) {
            yield();
            lastYieldTime = millis();
        }
    }
    gifRoot.close();

    if (total_gifs_count == 0) {
        Serial.println("No GIF files found in /gifs directory!");
        displayStatus(dma_display, "No GIFs Found", dma_display->color565(255, 0, 0));
        return false;
    }

    Serial.printf("Total GIFs found: %lu\n", total_gifs_count);
    char msg[32];
    sprintf(msg, "Total: %lu GIFs", total_gifs_count);
    displayStatus(dma_display, msg, dma_display->color565(0, 255, 0));
    delay(2000);
    return true;
}

// Function to load the next batch of GIF file paths
bool loadNextGifBatch(MatrixPanel_I2S_DMA *dma_display) {
    Serial.printf("Loading batch starting from GIF #%lu...\n", current_batch_start + 1);
    char status_msg[32];
    sprintf(status_msg, "Batch %lu/%lu", (current_batch_start / BATCH_SIZE) + 1, (total_gifs_count + BATCH_SIZE - 1) / BATCH_SIZE);
    
    if(SHOW_PROGRESS) {
        displayStatus(dma_display, status_msg, dma_display->color565(255, 255, 255));
    }
    
    Serial.printf("Heap before batch load: %lu bytes\n", ESP.getFreeHeap());

    // Clear previous batch
    clearGifFilePaths();

    FsFile gifRoot = sd.open(GIF_DIR);
    if (!gifRoot) {
        Serial.println("Failed to open /gifs directory!");
        displayStatus(dma_display, "Dir Error!", dma_display->color565(255, 0, 0));
        return false;
    }

    FsFile file;
    unsigned long current_gif_index = 0;
    unsigned long loaded_in_batch = 0;
    unsigned long lastYieldTime = millis();

    while (file.openNext(&gifRoot, O_RDONLY) && loaded_in_batch < BATCH_SIZE) {
        yield();
        if (file.isFile()) {
            char fileName[256];
            file.getName(fileName, sizeof(fileName));
            
            String fileNameStr = String(fileName);
            fileNameStr.toLowerCase();
            if (fileNameStr.endsWith(".gif")) {
                // Check if this GIF is in our current batch range
                if (current_gif_index >= current_batch_start && current_gif_index < current_batch_start + BATCH_SIZE) {
                    // Allocate memory for the full path
                    char* pathBuffer = new (std::nothrow) char[MAX_GIF_PATH_LEN];
                    if (pathBuffer == nullptr) {
                        Serial.println("ERROR: Failed to allocate memory for GIF path!");
                        displayStatus(dma_display, "MEMORY ERROR!", dma_display->color565(255, 0, 0));
                        clearGifFilePaths();
                        gifRoot.close();
                        file.close();
                        return false;
                    }
                    
                    snprintf(pathBuffer, MAX_GIF_PATH_LEN, "%s/%s", GIF_DIR, fileName);
                    gifFilePaths.push_back(pathBuffer);
                    
                    Serial.printf("Loaded GIF #%lu: %s\n", current_gif_index + 1, pathBuffer);
                    loaded_in_batch++;
                }
                current_gif_index++;
            }
        }
        file.close();

        if (millis() - lastYieldTime > 50) {
            yield();
            lastYieldTime = millis();
        }
    }
    gifRoot.close();

    total_files = gifFilePaths.size();
    
    if (gifFilePaths.empty()) {
        Serial.println("No GIFs loaded in this batch!");
        displayStatus(dma_display, "Batch Empty!", dma_display->color565(255, 0, 0));
        return false;
    }

    Serial.printf("Loaded %lu GIFs in current batch\n", total_files);
    Serial.printf("Heap after batch load: %lu bytes\n", ESP.getFreeHeap());
    
    if(SHOW_PROGRESS) {
        sprintf(status_msg, "Loaded %lu GIFs", total_files);
        displayStatus(dma_display, status_msg, dma_display->color565(0, 255, 0));
        delay(1000);
    }
    return true;
}

// Legacy function for backward compatibility - now just calls the counting function
bool loadGifFilePaths(MatrixPanel_I2S_DMA *dma_display) {
    return countTotalGifs(dma_display);
}