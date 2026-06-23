#ifndef __LOAD_PNG_H
#define __LOAD_PNG_H 1

#include <png.h>
#include <stdint.h>

uint8_t *loadPng (const char *path, int *w, int *h);

#endif
