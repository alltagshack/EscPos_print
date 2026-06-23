#ifndef __PRINTING_H
#define __PRINTING_H 1

#include <stdint.h>

#define CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

uint8_t *scaleMax (const uint8_t *src, int w, int h, int new_w, int *new_h);

void ditherAtkinson (uint8_t *img, int w, int h);

void printRaster (int fd, const uint8_t *bmp, uint16_t width, int height);

void printImage (int fd, const uint8_t *img, int w, int h, int max_width);

#endif
