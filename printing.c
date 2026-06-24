#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
/* write */
#include <unistd.h>

#include "printing.h"

uint8_t *scaleMax (const uint8_t *src, int w, int h, int new_w, int *new_h)
{
    int x, y;
    uint8_t *dst;
    double scale = (double)new_w/w;
    
    *new_h = scale * h;
    dst = (uint8_t *) malloc(new_w * (*new_h));

    for (y = 0; y < (*new_h); ++y)
    {
        for (x = 0; x < new_w; ++x) {
            dst[y * new_w + x] = src[(int)((float)y/scale) * w + (int)((float)x/scale)];
        }
    }

    return dst;
}

void ditherAtkinson (uint8_t *img, int w, int h)
{
    int x, y, i, err, diff;
    uint8_t old, fresh;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {

            i = y * w + x;
            old = img[i];
            fresh = (old < 128) ? 0 : 255;
            img[i] = fresh;

            err = (int)old - (int)fresh;
            diff = err / 8;  /* diffusion error per neighbor */

            /* Atkinson-Maske:
             *       X   1   1
             *   1   1   1
             *       1
             */

            /* y, x+1 */
            if (x + 1 < w)
                img[i + 1] = (uint8_t)CLAMP(img[i + 1] + diff, 0, 255);

            /* y, x+2 */
            if (x + 2 < w)
                img[i + 2] = (uint8_t)CLAMP(img[i + 2] + diff, 0, 255);

            /* y+1, x-1 */
            if (y + 1 < h && x - 1 >= 0)
                img[i + w - 1] = (uint8_t)CLAMP(img[i + w - 1] + diff, 0, 255);

            /* y+1, x */
            if (y + 1 < h)
                img[i + w] = (uint8_t)CLAMP(img[i + w] + diff, 0, 255);

            /* y+1, x+1 */
            if (y + 1 < h && x + 1 < w)
                img[i + w + 1] = (uint8_t)CLAMP(img[i + w + 1] + diff, 0, 255);

            /* y+2, x */
            if (y + 2 < h)
                img[i + 2 * w] = (uint8_t)CLAMP(img[i + 2 * w] + diff, 0, 255);
        }
    }
}

void printRaster (int fd, const uint8_t *bmp, uint16_t width, int height)
{
    int line, x, y, i, b, posY;
    int lines = height / 24;
    uint8_t pixel, byte, nL, nH;
    uint8_t hdr[5];

    nL = width & 0xFF;
    nH = (width >> 8) & 0xFF;

    hdr[0] = 0x1B;
    hdr[1] = 0x2A;
    hdr[2] = 0x21;
    hdr[3] = nL;
    hdr[4] = nH;
    
    for (line = 0; line < lines; ++line)
    {
        write(fd, hdr, sizeof(hdr));

        for (x = 0; x < width; x+=3)
        {
            for (i = 0; i < 3; ++i)
            {
                for (b = 0; b < 3; ++b)
                {
                    byte = 0;
                    for (y = 0; y < 8; ++y)
                    {
                        posY = line * 24 + y + b*8;
                        if (posY >= height) continue;
                        if ((x+i) < width) {
                            pixel = bmp[posY * width + x + i];
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
    /* linefeed to 0 */
    write(fd, "\x1B\x33\x00", 3);
    printRaster(fd, scaled, max_width, new_h);
    free(scaled);
}
