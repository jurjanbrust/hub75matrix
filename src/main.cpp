#include "FS.h"
#include "globals.h" // Expected to define frame_status_t, STARTUP, SD_CARD_ERROR, NO_FILES, PLAYING_ART, PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN
#include "gif.h"     // Expected to define InitMatrixGif(), ShowGIF()
#include "sdcard.h"  // Include our updated SD handler header
#include "portal.h"  // Include WiFi portal setup header
#include "settings.h" // Include Preferences for storing settings
#include "filesystem.h" // Include LittleFS filesystem handler
#include <AnimatedGIF.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <SPI.h>
#include <vector>

// Global variables from globals.h (if they are not extern in globals.h already)
frame_status_t target_state = STARTUP;
unsigned long lastStateChange = 0;
bool interruptGif = false;
bool gifsLoaded = false;
int brightness = DEFAULT_BRIGHTNESS;
bool autoPlay = true;
bool gifPlaying = false;
bool allowNextGif = true;
bool queue_populate_requred = false;

// Matrix display pointer
MatrixPanel_I2S_DMA *dma_display = nullptr;

// We'll define these colors once dma_display is initialized in setup()
uint16_t myBLACK;
uint16_t myWHITE;
uint16_t myRED;
uint16_t myGREEN;
uint16_t myBLUE;

/************************* Arduino Sketch Setup and Loop() *******************************/
void setup() {
    Serial.begin(115200);
    delay(1000); // Give serial time to initialize

    Serial.println("Initializing HUB75 Matrix Display...");

    // Load brightness from preferences before initializing display
    loadBrightnessFromPreferences();

    // Initialize HUB75 display first
    HUB75_I2S_CFG mxconfig(
        PANEL_RES_X,    // module width
        PANEL_RES_Y,    // module height
        PANEL_CHAIN     // Chain of panels - Horizontal width only.
    );

    // Display Setup
    dma_display = new MatrixPanel_I2S_DMA(mxconfig);
    dma_display->begin();
    dma_display->setBrightness8(brightness); // Use loaded brightness value
    dma_display->clearScreen();

    // Initialize display colors after dma_display is created
    myBLACK = dma_display->color565(0, 0, 0);
    myWHITE = dma_display->color565(255, 255, 255);
    myRED = dma_display->color565(255, 0, 0);
    myGREEN = dma_display->color565(0, 255, 0);
    myBLUE = dma_display->color565(0, 0, 255);

    dma_display->fillScreen(myBLACK);
    dma_display->setCursor(0, 0);
    dma_display->setTextColor(myWHITE);
    dma_display->setTextSize(1);
    dma_display->print("Booting...");

    Serial.printf("Display initialized with brightness: %d\n", brightness);

    // --- Initialize LittleFS for web files ---
    if (!initLittleFS()) {
        Serial.println("LittleFS initialization failed!");
        dma_display->fillScreen(myBLACK);
        dma_display->setCursor(0, 0);
        dma_display->setTextColor(myRED);
        dma_display->print("LittleFS Error");
        // Continue anyway, as LittleFS is only for web interface
    } else {
        Serial.println("LittleFS initialized successfully");
    }

    // --- Initialize SD card using the existing function ---
    if (!initSD(dma_display)) {
        // SD card failed to initialize, halt or enter error state
        target_state = SD_CARD_ERROR;
        while(1) {
            delay(1000); // Stay in error state
        }
    }

    InitMatrixGif(); // This function is expected to be defined in "gif.h"

    setupWifi(); // This now also sets up the web API with LittleFS

    // --- Count total GIFs first ---
    if (!countTotalGifs(dma_display)) {
        // No GIF files found or directory failed to open
        target_state = NO_FILES;
        while(1) {
            delay(5000); // Stay in error state
        }
    }

    // Initialize batch processing variables
    current_batch_start = 0;
    batch_processing_complete = false;
    gifsLoaded = true;
    target_state = PLAYING_ART;
    
    Serial.printf("Setup complete. Found %lu total GIFs. Ready to start batch processing.\n", total_gifs_count);
}

void loop() {
    // Handle web API requests
    handleWebAPIRequests();
    
    if (total_gifs_count == 0) {
        Serial.println("No GIFs found. Looping...");
        delay(5000);
        return;
    }

    // Main batch processing loop
    while (true) {
        // Handle web API requests during GIF processing
        handleWebAPIRequests();
        
        // Check if we need to start over from the beginning
        if (current_batch_start >= total_gifs_count) {
            Serial.println("Completed all GIFs! Starting over from the beginning...");
            current_batch_start = 0;
            batch_processing_complete = false;
            delay(2000); // Pause before restarting the entire cycle
        }

        // Load the next batch of GIFs
        Serial.printf("Loading batch starting from GIF #%lu (Batch size: %d)\n", current_batch_start + 1, BATCH_SIZE);
        if (!loadNextGifBatch(dma_display)) {
            Serial.println("Failed to load GIF batch! Retrying...");
            delay(2000);
            continue; // Retry loading the same batch
        }

        // Play all GIFs in the current batch
        Serial.printf("Playing %lu GIFs in current batch...\n", gifFilePaths.size());
        for (size_t i = 0; i < gifFilePaths.size(); i++) {
            const char* path = gifFilePaths[i];
            current_gif = String(path);
            
            Serial.printf("Playing GIF #%lu: %s\n", current_batch_start + i + 1, path);
            
            // Display progress on matrix
            if(SHOW_PROGRESS) {
                char progress_msg[32];
                sprintf(progress_msg, "GIF %lu/%lu", current_batch_start + i + 1, total_gifs_count);
                displayStatus(dma_display, progress_msg, dma_display->color565(0, 255, 255));
                delay(500); // Brief pause to show progress
            }
            
            ShowGIF(path); // Play GIF using the stored path
            delay(100); // Small delay between GIFs
            
            yield(); // Allow other tasks to run
            handleWebAPIRequests(); // Handle API requests between GIFs
        }

        // Move to the next batch
        current_batch_start += BATCH_SIZE;
        
        Serial.printf("Batch complete. Next batch will start from GIF #%lu\n", current_batch_start + 1);
        
        // Clear current batch from memory before loading the next one
        clearGifFilePaths();
        
        // Show memory status
        Serial.printf("Free heap after batch: %lu bytes\n", ESP.getFreeHeap());
    }
}