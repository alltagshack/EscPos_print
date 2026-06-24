#include <png.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "globals.h"
#include "load_png.h"

uint8_t *loadPng (const char *path, int *w, int *h)
{
    int x, y;
    png_structp png;
    png_infop info;
    png_byte ct, bd;
    png_bytep *rows;
    uint8_t *gray;
    uint8_t *src;
    FILE *fp;

    fp = fopen(path, "rb");

    if (!fp) g_abort("cannot open image file");

    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) g_abort("png_create_read_struct failed");
    info = png_create_info_struct(png);
    if (!info) g_abort("png_create_info_struct failed");
    if (setjmp(png_jmpbuf(png))) g_abort("error during init_io");

    png_init_io(png, fp);
    png_read_info(png, info);

    *w = png_get_image_width(png, info);
    *h = png_get_image_height(png, info);
    ct = png_get_color_type(png, info);
    bd = png_get_bit_depth(png, info);

    if (bd == 16) png_set_strip_16(png);
    if (ct == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (ct == PNG_COLOR_TYPE_RGB ||
        ct == PNG_COLOR_TYPE_RGB_ALPHA) png_set_rgb_to_gray(png, 1, -1, -1);
    if (ct == PNG_COLOR_TYPE_GRAY && bd < 8) png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    png_read_update_info(png, info);

    rows = malloc(sizeof(png_bytep) * (*h));
    for (y = 0; y < (*h); y++) {
        rows[y] = malloc(png_get_rowbytes(png, info));
    }
    png_read_image(png, rows);
    fclose(fp);

    gray = malloc((*w) * (*h));
    for (y = 0; y < (*h); y++) {
        src = rows[y];
        for (x = 0; x < (*w); x++) gray[y * (*w) + x] = src[x];
        free(rows[y]);
    }
    free(rows);
    png_destroy_read_struct(&png, &info, NULL);

    return gray;
}
