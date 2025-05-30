#include "FS.h"
#include <LittleFS.h>
#include <AnimatedGIF.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <vector> // Include for std::vector

#define FILESYSTEM LittleFS
#define FORMAT_LITTLEFS_IF_FAILED true

#define PANEL_RES_X 64     // Number of pixels wide of each INDIVIDUAL panel module.
#define PANEL_RES_Y 32     // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 2      // Total number of panels chained one to another horizontally only.

//MatrixPanel_I2S_DMA dma_display;
MatrixPanel_I2S_DMA *dma_display = nullptr;

// We'll define these colors once dma_display is initialized in setup()
uint16_t myBLACK;
uint16_t myWHITE;
uint16_t myRED;
uint16_t myGREEN;
uint16_t myBLUE;

AnimatedGIF gif;
File f; // This 'f' will now be used by the GIF callbacks directly

int x_offset, y_offset;

// Vector to store file paths
std::vector<String> gifFilePaths;

// Draw a line of image directly on the LED Matrix
void GIFDraw(GIFDRAW *pDraw)
{
    uint8_t *s;
    uint16_t *d, *usPalette, usTemp[320];
    int x, y, iWidth;

    iWidth = pDraw->iWidth;
    if (iWidth > dma_display->width())
        iWidth = dma_display->width();

    usPalette = pDraw->pPalette;
    y = pDraw->iY + pDraw->y; // current line

    s = pDraw->pPixels;
    if (pDraw->ucDisposalMethod == 2) // restore to background color
    {
        for (x=0; x<iWidth; x++)
        {
            if (s[x] == pDraw->ucTransparent)
                s[x] = pDraw->ucBackground;
        }
        pDraw->ucHasTransparency = 0;
    }
    // Apply the new pixels to the main image
    if (pDraw->ucHasTransparency) // if transparency used
    {
        uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
        int x, iCount;
        pEnd = s + pDraw->iWidth;
        x = 0;
        iCount = 0; // count non-transparent pixels
        while(x < pDraw->iWidth)
        {
            c = ucTransparent-1;
            d = usTemp;
            while (c != ucTransparent && s < pEnd)
            {
                c = *s++;
                if (c == ucTransparent) // done, stop
                {
                    s--; // back up to treat it like transparent
                }
                else // opaque
                {
                    *d++ = usPalette[c];
                    iCount++;
                }
            } // while looking for opaque pixels
            if (iCount) // any opaque pixels?
            {
                for(int xOffset = 0; xOffset < iCount; xOffset++ ){
                    dma_display->drawPixel(x + xOffset, y, usTemp[xOffset]); // 565 Color Format
                }
                x += iCount;
                iCount = 0;
            }
            // no, look for a run of transparent pixels
            c = ucTransparent;
            while (c == ucTransparent && s < pEnd)
            {
                c = *s++;
                if (c == ucTransparent)
                    iCount++;
                else
                    s--;
            }
            if (iCount)
            {
                x += iCount; // skip these
                iCount = 0;
            }
        }
    }
    else // does not have transparency
    {
        s = pDraw->pPixels;
        // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
        for (x=0; x<pDraw->iWidth; x++)
        {
            dma_display->drawPixel(x, y, usPalette[*s++]); // color 565
        }
    }
} /* GIFDraw() */


void * GIFOpenFile(const char *fname, int32_t *pSize)
{
    Serial.print("Playing gif: ");
    Serial.println(fname);
    f = FILESYSTEM.open(fname); // Use the global 'f'
    if (f)
    {
        *pSize = f.size();
        return (void *)&f; // Return a pointer to the global 'f'
    }
    return NULL;
} /* GIFOpenFile() */

void GIFCloseFile(void *pHandle)
{
    File *fp = static_cast<File *>(pHandle);
    if (fp != NULL)
        fp->close(); // Close the file pointed to by pHandle
} /* GIFCloseFile() */

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
    int32_t iBytesRead;
    iBytesRead = iLen;
    File *fp = static_cast<File *>(pFile->fHandle); // Cast to File*
    // Note: If you read a file all the way to the last byte, seek() stops working
    if ((pFile->iSize - pFile->iPos) < iLen)
        iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
    if (iBytesRead <= 0)
        return 0;
    iBytesRead = (int32_t)fp->read(pBuf, iBytesRead);
    pFile->iPos = fp->position();
    return iBytesRead;
} /* GIFReadFile() */

int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition)
{
    int i = micros();
    File *fp = static_cast<File *>(pFile->fHandle); // Cast to File*
    fp->seek(iPosition);
    pFile->iPos = (int32_t)fp->position();
    i = micros() - i;
//    Serial.printf("Seek time = %d us\n", i);
    return pFile->iPos;
} /* GIFSeekFile() */

unsigned long start_tick = 0;

void ShowGIF(const char *name) // Changed to const char*
{
    start_tick = millis();

    if (gif.open(name, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw))
    {
        x_offset = (dma_display->width() - gif.getCanvasWidth())/2;
        if (x_offset < 0) x_offset = 0;
        y_offset = (dma_display->height() - gif.getCanvasHeight())/2;
        if (y_offset < 0) y_offset = 0;
        Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
        Serial.flush();
        while (gif.playFrame(true, NULL))
        {
            // No timeout, play fully
        }
        gif.close();
    } else {
        Serial.printf("Failed to open GIF: %s\n", name);
    }
} /* ShowGIF() */

String gifDir = "/gifs"; // play all GIFs in this directory on the SD card

/************************* Arduino Sketch Setup and Loop() *******************************/
void setup() {
    Serial.begin(115200);

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

    // Start going through GIFS
    gif.begin(LITTLE_ENDIAN_PIXELS);

    // --- NEW: Fetch all filenames into the vector ---
    Serial.println("Fetching GIF filenames...");
    File root = FILESYSTEM.open(gifDir);
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