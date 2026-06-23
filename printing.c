#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h> // write

#include "printing.h"

uint8_t *scaleMax (const uint8_t *src, int w, int h, int new_w, int *new_h)
{
    double scale = (double)new_w/w;
    
    int h2 = scale * h;
    uint8_t *dst = (uint8_t *) malloc(new_w * h2);

    for (int y = 0; y < h2; ++y)
    {
        for (int x = 0; x < new_w; ++x) {
            dst[y * new_w + x] = src[(int)((float)y/scale) * w + (int)((float)x/scale)];
        }
    }

    *new_h = h2;
    return dst;
}

void ditherAtkinson (uint8_t *img, int w, int h)
{
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {

            int i = y * w + x;
            uint8_t old = img[i];
            uint8_t new = (old < 128) ? 0 : 255;
            img[i] = new;

            int err = (int)old - (int)new;
            int diff = err / 8;  // Fehleranteil pro Nachbar

            // Atkinson-Maske:
            //       X   1   1
            //   1   1   1
            //       1

            // y, x+1
            if (x + 1 < w)
                img[i + 1] = (uint8_t)CLAMP(img[i + 1] + diff, 0, 255);

            // y, x+2
            if (x + 2 < w)
                img[i + 2] = (uint8_t)CLAMP(img[i + 2] + diff, 0, 255);

            // y+1, x-1
            if (y + 1 < h && x - 1 >= 0)
                img[i + w - 1] = (uint8_t)CLAMP(img[i + w - 1] + diff, 0, 255);

            // y+1, x
            if (y + 1 < h)
                img[i + w] = (uint8_t)CLAMP(img[i + w] + diff, 0, 255);

            // y+1, x+1
            if (y + 1 < h && x + 1 < w)
                img[i + w + 1] = (uint8_t)CLAMP(img[i + w + 1] + diff, 0, 255);

            // y+2, x
            if (y + 2 < h)
                img[i + 2 * w] = (uint8_t)CLAMP(img[i + 2 * w] + diff, 0, 255);
        }
    }
}

void printRaster (int fd, const uint8_t *bmp, uint16_t width, int height)
{
    int lines = height / 24;
    uint8_t nL = width & 0xFF;
    uint8_t nH = (width >> 8) & 0xFF;
    
    uint8_t hdr[5] = {
        0x1B, 0x2A, 0x21, nL, nH
    };
    
    for (int line = 0; line < lines; ++line)
    {
        write(fd, hdr, sizeof(hdr));

        for (int x = 0; x < width; x+=3)
        {
            for (int i = 0; i < 3; ++i)
            {
                for (int b = 0; b < 3; ++b)
                {
                    uint8_t byte = 0;
                    for (int y = 0; y < 8; ++y)
                    {
                        int posY = line * 24 + y + b*8;
                        if (posY >= height) continue;
                        if ((x+i) < width) {
                            uint8_t pixel = bmp[posY * width + x + i];
                            if (pixel < 128) {
                                byte |= (1 << (7 - y));
                            }
                        }
                    }
                    write(fd, &byte, 1);
                }
            }
        }
        write(fd, "\x0A", 1);
    }
}

void printImage (int fd, const uint8_t *img, int w, int h, int max_width)
{
    int new_h;
    uint8_t *scaled = NULL;
    
    printf("load with width=%d height=%d\n", w, h);
    scaled = scaleMax(img, w, h, max_width, &new_h);
    ditherAtkinson(scaled, max_width, new_h);
    // linefeed to 0
    write(fd, "\x1B\x33\x00", 3);
    printRaster(fd, scaled, max_width, new_h);
    free(scaled);
}
