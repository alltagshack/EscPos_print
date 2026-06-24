/* 180dpi mode */

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
/* write, close */
#include <unistd.h>
/* open */
#include <fcntl.h>

#include "globals.h"
#include "load_bmp.h"
#include "load_png.h"
#include "markdown.h"
#include "printing.h"

int allowed_w = 360;

void g_abort (const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

int isMime (const char *filepath, char *extension)
{
    if (!strcmp(filepath + strlen(filepath) - strlen(extension), extension))
        return 1;
    else
        return 0;
}

int main (int argc, char *argv[])
{
    int w, h;
    size_t sz;
    uint8_t *src;
    int fd;

    const char *src_path = argv[1];
    const char *dev_path = argv[2];


    if (argc != 3) {
        fprintf(stderr, "Usage: %s <bmp png or md file> <device>\n", argv[0]);
        return EXIT_FAILURE;
    }

    fd = open(dev_path, O_WRONLY);
    if (fd < 0) g_abort("cannot open device for writing");
    /* reset */
    write(fd, "\x1B\x40", 2);
    /* Codepage 858 */
    write(fd, "\x1B\x74\x13", 3);
    /* CPI Mode */
    write(fd, "\x1B\xC1\x01", 3);

    src = NULL;
    w = 0;
    h = 0;
    if (isMime(src_path, ".bmp") == 1)
    {
        src = loadBmp(src_path, &w, &h);
        printImage(fd, src, w, h, allowed_w);
    }
    else if (isMime(src_path, ".png") == 1)
    {
        src = loadPng(src_path, &w, &h);
        printImage(fd, src, w, h, allowed_w);
    }
    else if (isMime(src_path, ".md") == 1)
    {
        src = loadMarkdown(src_path, &sz);
        printMarkdown(fd, src, &sz);
    }

    close(fd);
    free(src);
    return EXIT_SUCCESS;
}
