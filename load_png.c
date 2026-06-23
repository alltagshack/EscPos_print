#include <png.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "globals.h"
#include "load_png.h"

uint8_t *loadPng (const char *path, int *w, int *h) {
    FILE *fp = fopen(path, "rb");
    if (!fp) g_abort("cannot open image file");

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) g_abort("png_create_read_struct failed");
    png_infop info = png_create_info_struct(png);
    if (!info) g_abort("png_create_info_struct failed");
    if (setjmp(png_jmpbuf(png))) g_abort("error during init_io");

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
