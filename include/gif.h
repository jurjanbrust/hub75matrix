#ifndef _GIF_
#define _GIF_

#include <AnimatedGIF.h>

void InitMatrixGif();
void ShowGIF(const char *name);
void GIFDraw(GIFDRAW *pDraw);
int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen);
void * GIFOpenFile(const char *fname, int32_t *pSize);
void GIFCloseFile(void *pHandle);

#endif