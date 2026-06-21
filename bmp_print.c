/*  echo -e "\x1B\x2A\x00\x03\x00\xAA\xAA\xAA" > /dev/usb/lp0
 *
 * 180dpi, scale image
 * 
 *  Compile:
 *      gcc bmp_print.c -Wall -o bmp_print
 *
 *  Usage:
 *      ./bmp_print <image_file> <device>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int src_w = 0;
int src_h = 0;

int allowed_w = 360;

#define CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

#pragma pack(push,1)                     // Struktur ohne Padding
typedef struct {
    uint16_t bfType;      // 0 'BM'
    uint32_t bfSize;      // 2  filesize
    uint16_t bfReserved1; // 6
    uint16_t bfReserved2; // 8
    uint32_t bfOffBits;   // 10  Offset to pixel array
} BITMAPFILEHEADER;

typedef struct {
    uint32_t biSize;          // 14 Header size (40)
    int32_t  biWidth;         // 18
    int32_t  biHeight;        // 22
    uint16_t biPlanes;        // 26 value = 1
    uint16_t biBitCount;      // 28 8bit
    uint32_t biCompression;   // 30 0 = no compression
    uint32_t biSizeImage;     // 34 pixel array size
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;       // 256
    uint32_t biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

void abort_ (const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

uint8_t *scaleMax (uint8_t *src, int w, int h, int *new_h)
{
    double scale = (double)allowed_w/w;
    
    int h2 = scale * h;
    uint8_t *dst = malloc(allowed_w * h2);

    for (int y = 0; y < h2; ++y)
    {
        for (int x = 0; x < allowed_w; ++x) {
            dst[y * allowed_w + x] = src[(int)((float)y/scale) * w + (int)((float)x/scale)];
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

uint8_t *loadBmp (const char *path, int *w, int *h)
{
    uint8_t *row;
    uint8_t *gray = NULL;
    int rowSize;
    int padding;
    BITMAPFILEHEADER fh = {0};
    BITMAPINFOHEADER ih = {0};
    
    FILE *fp = fopen(path, "rb");
    if (!fp) abort_("cannot open image file");

    fread(&fh, sizeof(BITMAPFILEHEADER), 1, fp);
    fread(&ih, sizeof(BITMAPINFOHEADER), 1, fp);
    fseek(fp, fh.bfOffBits, SEEK_SET);
    
    *w = ih.biWidth;
    *h = ih.biHeight;
    rowSize = ((*w + 3) / 4) * 4;
    padding = rowSize - *w;
    
    gray = malloc(sizeof(uint8_t) * (*w) * *h);
    
    for(int y = 0; y < *h; ++y)
    {
        row = gray + (*h - y - 1) * (*w);
        fread(row, 1, *w, fp);
        fseek(fp, padding, SEEK_CUR); 
    }
    fclose(fp);

    return gray;
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

int main (int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <image_file> <device>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *img_path = argv[1];
    const char *dev_path = argv[2];

    int new_h;
    uint8_t *src = loadBmp(img_path, &src_w, &src_h);
    printf("load with width=%d height=%d\n", src_w, src_h);

    uint8_t *scaled = scaleMax(src, src_w, src_h, &new_h);
    
    ditherAtkinson(scaled, allowed_w, new_h);
    
    int fd = open(dev_path, O_WRONLY);
    if (fd < 0) abort_("cannot open device for writing");
    
    // reset
    write(fd, "\x1B\x40", 2);
    // linefeed to 0
    write(fd, "\x1B\x33\x00", 3);
    printRaster(fd, scaled, allowed_w, new_h);

    close(fd);
    free(src);
    free(scaled);
    return EXIT_SUCCESS;
}
