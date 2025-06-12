#include "FS.h"
#include "globals.h" // Expected to define frame_status_t, STARTUP, SD_CARD_ERROR, NO_FILES, PLAYING_ART, PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN
#include "gif.h"     // Expected to define InitMatrixGif(), ShowGIF()
#include "sdcard.h"      // Include our new SD handler header
#include <AnimatedGIF.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <SPI.h>
#include <vector>

// Global variables from globals.h (if they are not extern in globals.h already)
frame_status_t target_state = STARTUP;
unsigned long lastStateChange = 0;
bool interruptGif = false;
bool gifsLoaded = false;
int brightness = 128;
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

    // Initialize HUB75 display first
    HUB75_I2S_CFG mxconfig(
        PANEL_RES_X,    // module width
        PANEL_RES_Y,    // module height
        PANEL_CHAIN     // Chain of panels - Horizontal width only.
    );

    // Display Setup
    dma_display = new MatrixPanel_I2S_DMA(mxconfig);
    dma_display->begin();
    dma_display->setBrightness8(brightness); //0-255
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

    // --- Initialize SD card using the new function ---
    if (!initSD(dma_display)) {
        // SD card failed to initialize, halt or enter error state
        target_state = SD_CARD_ERROR;
        while(1) {
            delay(1000); // Stay in error state
        }
    }

    // --- List directories on root using the new function ---
    listRootDirectories(dma_display);

    InitMatrixGif(); // This function is expected to be defined in "gif.h"

    // --- Fetch all filenames into the vector using the new function ---
    if (!loadGifFilePaths(dma_display)) {
        // No GIF files found or directory failed to open
        target_state = NO_FILES;
        while(1) {
            delay(5000); // Stay in error state
        }
    } else {
        gifsLoaded = true;
        target_state = PLAYING_ART;
    }
}

void loop() {
    if (gifFilePaths.empty() || !gifsLoaded) {
        Serial.println("No GIFs to play. Looping...");
        delay(5000);
        return;
    }

    while (true) // Loop through the GIFs indefinitely
    {
        for (const String& path : gifFilePaths) {
            current_gif = path;
            Serial.printf("Playing: %s\n", path.c_str());
            ShowGIF(path.c_str()); // Play GIF using the stored path. ShowGIF() is expected to be defined in "gif.h"
            delay(100); // Small delay between GIFs
        }
        delay(1000); // Pause before restarting the loop through all GIFs
    }
}