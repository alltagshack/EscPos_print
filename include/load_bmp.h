#ifndef __LOAD_BMP_H
#define __LOAD_BMP_H 1

#include <stdint.h>

#pragma pack(push,1)      /* a struct without padding */
typedef struct {
    uint16_t bfType;      /* 0 'BM' */
    uint32_t bfSize;      /* 2  filesize */
    uint16_t bfReserved1; /* 6 */
    uint16_t bfReserved2; /* 8 */
    uint32_t bfOffBits;   /* 10  Offset to pixel array */
} BITMAPFILEHEADER;

typedef struct {
    uint32_t biSize;          /* 14 Header size (40) */
    int32_t  biWidth;         /* 18 */
    int32_t  biHeight;        /* 22 */
    uint16_t biPlanes;        /* 26 value = 1 */
    uint16_t biBitCount;      /* 28 8bit */
    uint32_t biCompression;   /* 30 0 = no compression */
    uint32_t biSizeImage;     /* 34 pixel array size */
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;       /* 256 */
    uint32_t biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

uint8_t *loadBmp (const char *path, int *w, int *h);

#endif
