/* 180dpi mode */

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> // write, close
#include <fcntl.h> // open

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
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <bmp png or md file> <device>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int src_w = 0;
    int src_h = 0;

    const char *src_path = argv[1];
    const char *dev_path = argv[2];

    uint8_t *src = NULL;
    int fd;

    fd = open(dev_path, O_WRONLY);
    if (fd < 0) g_abort("cannot open device for writing");
    // reset
    write(fd, "\x1B\x40", 2);
    // Codepage 858
    write(fd, "\x1B\x74\x13", 3);
    // CPI Mode
    write(fd, "\x1B\xC1\x01", 3);

    if (isMime(src_path, ".bmp") == 1)
    {
        src = loadBmp(src_path, &src_w, &src_h);
        printImage(fd, src, src_w, src_h, allowed_w);
    }
    else if (isMime(src_path, ".png") == 1)
    {
        src = loadPng(src_path, &src_w, &src_h);
        printImage(fd, src, src_w, src_h, allowed_w);
    }
    else if (isMime(src_path, ".md") == 1)
    {
        size_t size;
        src = loadMarkdown(src_path, &size);
        printMarkdown(fd, src, &size);
    }

    close(fd);
    free(src);
    return EXIT_SUCCESS;
}
