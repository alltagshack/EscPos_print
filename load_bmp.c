#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "globals.h"
#include "load_bmp.h"

uint8_t *loadBmp (const char *path, int *w, int *h)
{
    uint8_t *row;
    uint8_t *gray;
    int y, rowSize;
    int padding;
    FILE *fp;
    BITMAPFILEHEADER fh = {0};
    BITMAPINFOHEADER ih = {0};
    
    fp = fopen(path, "rb");
    if (!fp) g_abort("cannot open image file");

    fread(&fh, sizeof(BITMAPFILEHEADER), 1, fp);
    fread(&ih, sizeof(BITMAPINFOHEADER), 1, fp);
    fseek(fp, fh.bfOffBits, SEEK_SET);
    
    *w = ih.biWidth;
    *h = ih.biHeight;
    rowSize = ((*w + 3) / 4) * 4;
    padding = rowSize - *w;

    gray = NULL;
    gray = (uint8_t *) malloc(sizeof(uint8_t) * (*w) * *h);
    
    for(y = 0; y < *h; ++y)
    {
        row = gray + (*h - y - 1) * (*w);
        fread(row, 1, *w, fp);
        fseek(fp, padding, SEEK_CUR); 
    }
    fclose(fp);

    return gray;
}
