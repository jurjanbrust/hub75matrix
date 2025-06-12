#include "sdcard.h"
#include "globals.h" // For SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN, GIF_DIR, frame_status_t, SD_CARD_ERROR, NO_FILES, PLAYING_ART
#include <SPI.h>

// Uncomment this define to limit the number of GIFs loaded for debugging purposes.
// This helps determine if the crash is due to too many files.
// #define MAX_GIFS_TO_LOAD 50 // Example: load only first 50 GIFs

// Global SD-related variables definitions
SdFs sd;
bool sdError = false;
std::vector<char*> gifFilePaths; // <<-- CHANGED: Now a vector of C-style strings
unsigned long total_files = 0;
String current_gif = ""; // Keep this as String for now, if it's used elsewhere as such.

// Helper function for displaying status on the matrix display (NO CHANGE)
void displayStatus(MatrixPanel_I2S_DMA *dma_display, const char* message, uint16_t color) {
    if (dma_display == nullptr) return;

    dma_display->fillScreen(dma_display->color565(0, 0, 0));
    dma_display->setCursor(10, 10);
    dma_display->setTextColor(color);
    dma_display->setTextSize(1);
    dma_display->print(message);
}

// Utility to clear the paths and free memory (NEW FUNCTION)
void clearGifFilePaths() {
    for (char* path : gifFilePaths) {
        delete[] path; // Free each individual char array allocated with new[]
    }
    gifFilePaths.clear(); // Clear the vector itself
}

// Function to initialize the SD card (NO CHANGE)
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

// Function to list all directories on the root folder (YIELDING ADDED PREVIOUSLY, no further change needed here)
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

// Function to load GIF file paths from the dedicated GIF directory (MAJOR CHANGES HERE)
bool loadGifFilePaths(MatrixPanel_I2S_DMA *dma_display) {
    Serial.println("Fetching GIF filenames from SD card...");
    displayStatus(dma_display, "Scanning GIFs...", dma_display->color565(255, 255, 255));
    Serial.printf("Heap before GIF scan: %lu bytes\n", ESP.getFreeHeap());

    // CRITICAL: Clear any previously allocated paths to prevent memory leaks if re-scanning
    clearGifFilePaths();

    FsFile gifRoot = sd.open(GIF_DIR);
    if (!gifRoot) {
        Serial.println("Failed to open /gifs directory on SD card!");
        Serial.println("Make sure the /gifs directory exists on your SD card");
        displayStatus(dma_display, "No /gifs Dir", dma_display->color565(255, 0, 0));
        return false;
    }

    FsFile file;
    unsigned int gifsFoundInScan = 0;
    unsigned long lastYieldTime = millis();

    while (file.openNext(&gifRoot, O_RDONLY)) {
        yield(); // Keep yielding
        if (file.isFile()) {
            char fileName[256]; // Buffer for just the filename from SdFat
            file.getName(fileName, sizeof(fileName));

            // Check if it's a GIF file (case-insensitive) using the filename string
            // We'll still use a temporary String here for convenience of endsWith()
            String fileNameStr = String(fileName);
            fileNameStr.toLowerCase();
            if (fileNameStr.endsWith(".gif")) {
                #ifdef MAX_GIFS_TO_LOAD
                if (gifsFoundInScan >= MAX_GIFS_TO_LOAD) {
                    Serial.printf("Reached MAX_GIFS_TO_LOAD (%d). Skipping remaining.\n", MAX_GIFS_TO_LOAD);
                    file.close(); // Close current file before breaking
                    break;
                }
                #endif

                // Allocate memory for the full path (+1 for null terminator)
                char* pathBuffer = new (std::nothrow) char[MAX_GIF_PATH_LEN]; // Use new (nothrow) for safe allocation
                if (pathBuffer == nullptr) {
                    Serial.println("ERROR: Failed to allocate memory for GIF path!");
                    displayStatus(dma_display, "MEM ALLOC ERR!", dma_display->color565(255, 0, 0));
                    clearGifFilePaths(); // Clean up partial allocations
                    return false; // Indicate critical memory allocation failure
                }
                // Construct the full path using snprintf for safety (prevents buffer overflow)
                snprintf(pathBuffer, MAX_GIF_PATH_LEN, "%s/%s", GIF_DIR, fileName);
                gifFilePaths.push_back(pathBuffer); // Store the char* pointer

                Serial.printf("Found GIF: %s (Heap: %lu)\n", pathBuffer, ESP.getFreeHeap());
                gifsFoundInScan++;
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
        Serial.println("No GIF files found in /gifs directory on SD card!");
        displayStatus(dma_display, "No GIFs Found", dma_display->color565(255, 0, 0));
        return false;
    } else {
        Serial.printf("Total GIFs found: %lu\n", total_files);
        Serial.printf("Heap after GIF scan: %lu bytes\n", ESP.getFreeHeap());

        char msg[32];
        sprintf(msg, "Found %lu GIFs", total_files);
        displayStatus(dma_display, msg, dma_display->color565(0, 255, 0));
        delay(2000);
        return true;
    }
}