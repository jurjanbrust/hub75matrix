#include "FS.h"
#include <SdFat.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <AnimatedGIF.h>
#include "Globals.h"

int x_offset, y_offset;
AnimatedGIF gif;
FsFile f; // Changed from File to FsFile for SdFat compatibility

void InitMatrixGif()
{
  gif.begin(LITTLE_ENDIAN_PIXELS);
}

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
    f = FILESYSTEM.open(fname); // Use the global 'f' with SD filesystem
    if (f)
    {
        *pSize = f.size();
        return (void *)&f; // Return a pointer to the global 'f'
    }
    return NULL;
} /* GIFOpenFile() */

void GIFCloseFile(void *pHandle)
{
    FsFile *fp = static_cast<FsFile *>(pHandle); // Changed to FsFile*
    if (fp != NULL)
        fp->close(); // Close the file pointed to by pHandle
} /* GIFCloseFile() */

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
    int32_t iBytesRead;
    iBytesRead = iLen;
    FsFile *fp = static_cast<FsFile *>(pFile->fHandle); // Cast to FsFile*
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
    FsFile *fp = static_cast<FsFile *>(pFile->fHandle); // Cast to FsFile*
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
        while (gif.playFrame(true, NULL))
        {
            // No timeout, play fully
        }
        gif.close();
    } else {
        Serial.printf("Failed to open GIF: %s\n", name);
    }
} /* ShowGIF() */