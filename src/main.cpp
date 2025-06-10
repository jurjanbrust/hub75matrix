#include "FS.h"
#include "globals.h"
#include "gif.h"
#include <LittleFS.h>
#include <AnimatedGIF.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <vector> // Include for std::vector

// #define R1_PIN 25
// #define G1_PIN 26
// #define B1_PIN 27
// #define R2_PIN 14
// #define G2_PIN 12
// #define B2_PIN 13
// #define A_PIN 23
// #define B_PIN 19
// #define C_PIN 5
// #define D_PIN 17
// #define E_PIN -1 // required for 1/32 scan panels, like 64x64px. Any available pin would do, i.e. IO32
// #define LAT_PIN 4
// #define OE_PIN 15
// #define CLK_PIN 16

//MatrixPanel_I2S_DMA dma_display;
MatrixPanel_I2S_DMA *dma_display = nullptr;

// Vector to store file paths
std::vector<String> gifFilePaths;

// We'll define these colors once dma_display is initialized in setup()
uint16_t myBLACK;
uint16_t myWHITE;
uint16_t myRED;
uint16_t myGREEN;
uint16_t myBLUE;

/************************* Arduino Sketch Setup and Loop() *******************************/
void setup() {
    Serial.begin(115200);

    InitMatrixGif();

    if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
        Serial.println("LittleFS Mount Failed");
        return;
    }

    // Initialize display colors after dma_display is created
    myBLACK = dma_display->color565(0, 0, 0);
    myWHITE = dma_display->color565(255, 255, 255);
    myRED = dma_display->color565(255, 0, 0);
    myGREEN = dma_display->color565(0, 255, 0);
    myBLUE = dma_display->color565(0, 0, 255);


    HUB75_I2S_CFG mxconfig(
        PANEL_RES_X,    // module width
        PANEL_RES_Y,    // module height
        PANEL_CHAIN     // Chain of panels - Horizontal width only.
    );

    // mxconfig.gpio.e = 18;
    // mxconfig.clkphase = false;
    // mxconfig.driver = HUB75_I2S_CFG::FM6126A;

    // Display Setup
    dma_display = new MatrixPanel_I2S_DMA(mxconfig);
    dma_display->begin();
    dma_display->setBrightness8(128); //0-255
    dma_display->clearScreen();
    dma_display->fillScreen(myWHITE);

    // --- NEW: Fetch all filenames into the vector ---
    Serial.println("Fetching GIF filenames...");
    File root = FILESYSTEM.open(GIF_DIR);
    if (!root) {
        Serial.println("Failed to open /gifs directory!");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String filePath = file.path();
            if (filePath.endsWith(".gif") || filePath.endsWith(".GIF")) { // Optional: Filter for .gif extension
                gifFilePaths.push_back(filePath);
                Serial.print("Found GIF: ");
                Serial.println(filePath);
            }
        }
        file.close(); // Close the current file handle
        file = root.openNextFile(); // Get the next one
    }
    root.close(); // Close the directory handle

    if (gifFilePaths.empty()) {
        Serial.println("No GIF files found in /gifs directory!");
    } else {
        Serial.printf("Total GIFs found: %d\n", gifFilePaths.size());
    }
}

void loop()
{
    if (gifFilePaths.empty()) {
        Serial.println("No GIFs to play. Looping...");
        delay(5000); // Wait a bit if no GIFs are found
        return;
    }

    while (true) // Loop through the GIFs indefinitely
    {
        for (const String& path : gifFilePaths) {
            ShowGIF(path.c_str()); // Play GIF using the stored path
            delay(100); // Small delay between GIFs
        }
        delay(1000); // Pause before restarting the loop through all GIFs
    }
}