#ifndef __MARKDOWN_H
#define __MARKDOWN_H 1

#include <stddef.h>
#include <stdint.h>

size_t replaceUgly (uint8_t *buf, size_t len);

uint8_t *loadMarkdown (const char *path, size_t *sz);

/**
 * print utf-8 binary (without bom, and only \n) with markdown format
 * 
 * - handle #, ##, ###
 * - handle bold (font B 2x width)
 * - handle emphasize as underline + bold
 * - handle `code` as font B
 * - handle unordered lists with "- "
 */
void printMarkdown (int fd, const uint8_t *src, const size_t *sz);

#endif
