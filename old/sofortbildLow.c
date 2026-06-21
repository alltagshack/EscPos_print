/*  echo -e "\x1B\x2A\x00\x03\x00\xAA\xAA\xAA" > /dev/usb/lp0
 *
 * 90dpi, scale image
 * 
 *  Compile (needs libpng):
 *      gcc sofortbildLow.c -Wall -lpng -o sofortbildLow
 *
 *  Usage:
 *      ./sofortbildLow <image_file> <device>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <png.h>
#include <unistd.h>
#include <fcntl.h>

#define CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

static void abort_ (const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

static const uint8_t bayer[4][4] = {
    { 0,  8,  2, 10},
    {12,  4, 14,  6},
    { 3, 11,  1,  9},
    {15,  7, 13,  5}
};

uint8_t bayerThreshold (uint16_t x, uint16_t y)
{
    // (value + 0.5) * 255 / 16  →  round‑to‑nearest
    return (bayer[y & 3][x & 3] * 255 + 127) / 16;
}

uint8_t *scaleHeight (uint8_t *src, int w, int h, int *new_h)
{
    // factor is comming from tests
    double factor = 0.361f;
    int h2 = factor * h;
    uint8_t *dst = malloc(w * h2);

    for (int y = 0; y < h2; y++) {
        memcpy(dst + y * w, src + (int)((float)y/factor) * w, w);
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


static uint8_t *loadPNG (const char *path, int *w, int *h) {
    FILE *fp = fopen(path, "rb");
    if (!fp) abort_("cannot open image file");

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) abort_("png_create_read_struct failed");
    png_infop info = png_create_info_struct(png);
    if (!info) abort_("png_create_info_struct failed");
    if (setjmp(png_jmpbuf(png))) abort_("error during init_io");

    png_init_io(png, fp);
    png_read_info(png, info);

    int img_w = png_get_image_width(png, info);
    int img_h = png_get_image_height(png, info);
    png_byte ct = png_get_color_type(png, info);
    png_byte bd = png_get_bit_depth(png, info);

    if (bd == 16) png_set_strip_16(png);
    if (ct == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (ct == PNG_COLOR_TYPE_RGB ||
        ct == PNG_COLOR_TYPE_RGB_ALPHA) png_set_rgb_to_gray(png, 1, -1, -1);
    if (ct == PNG_COLOR_TYPE_GRAY && bd < 8) png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    png_read_update_info(png, info);

    png_bytep *rows = malloc(sizeof(png_bytep) * img_h);
    for (int y = 0; y < img_h; y++) {
        rows[y] = malloc(png_get_rowbytes(png, info));
    }
    png_read_image(png, rows);
    fclose(fp);

    uint8_t *gray = malloc(img_w * img_h);
    for (int y = 0; y < img_h; y++) {
        uint8_t *src = rows[y];
        for (int x = 0; x < img_w; x++) gray[y * img_w + x] = src[x];
        free(rows[y]);
    }
    free(rows);
    png_destroy_read_struct(&png, &info, NULL);

    *w = img_w;
    *h = img_h;
    return gray;
}

static void printRaster (int fd, const uint8_t *bmp, uint16_t width, int height)
{
    int lines = height / 8;
    uint8_t nL = width & 0xFF;
    uint8_t nH = (width >> 8) & 0xFF;
    
    uint8_t hdr[5] = {
        0x1B, 0x2A, 0x01, nL, nH
    };
    
    for (int line = 0; line < lines; line++)
    {
        write(fd, hdr, sizeof(hdr));

        for (int x = 0; x < width; x++)
        {
            uint8_t byte = 0;
            for (int y = 0; y < 8; y++)
            {
                int posY = line * 8 + y;
                if (posY >= height) continue;
                uint8_t pixel = bmp[posY * width + x];
                
                /* uint8_t th = bayerThreshold(x, y); */
                uint8_t th = 128; /* ditherAtkinson() was used in main */
                if (pixel < th) {
                    byte |= (1 << (7 - y));
                }
            }
            write(fd, &byte, 1);
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

    int src_w, src_h, new_h;
    uint8_t *src = loadPNG(img_path, &src_w, &src_h);
    printf("load: width=%d height=%d\n", src_w, src_h);

    uint8_t *scaled = scaleHeight(src, src_w, src_h, &new_h);
    
    /* if you use bayerThreshold, you do not need ditherAtkinson() */
    ditherAtkinson(scaled, src_w, new_h);
    
    int fd = open(dev_path, O_WRONLY);
    if (fd < 0) abort_("cannot open device for writing");
    
    // reset
    write(fd, "\x1B\x40", 2);
    // linefeed to 0
    write(fd, "\x1B\x33\x00", 3);
    printRaster(fd, scaled, src_w, new_h);

    close(fd);
    free(src);
    free(scaled);
    return EXIT_SUCCESS;
}
