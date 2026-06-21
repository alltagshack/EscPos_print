/*  echo -e "\x1B\x2A\x00\x03\x00\xAA\xAA\xAA" > /dev/usb/lp0
 *
 *  180dpi mode
 * 
 *  Compile (needs libpng and iconv?):
 *      gcc escpos_print.c -Wall -lpng -o escposPrint
 *
 *  Usage:
 *      ./escposPrint <bmp png or md file> <device>
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <png.h>
#include <unistd.h>
#include <fcntl.h>
#include <iconv.h>

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

uint8_t *scaleMax (const uint8_t *src, int w, int h, int *new_h)
{
    double scale = (double)allowed_w/w;
    
    int h2 = scale * h;
    uint8_t *dst = (uint8_t *) malloc(allowed_w * h2);

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

size_t replace_ugly (uint8_t *buf, size_t len)
{
    const struct {
        const uint8_t seq[4];
        size_t       seq_len;
        const char   newchar;
    } table[] = {
        /* small space */                  { {0xE2,0x80,0xAF,0}, 3, ' ' },
        /* Hyphen                U+2010 */ { {0xE2,0x80,0x90,0}, 3, '-' },
        /* Non‑breaking hyphen   U+2011 */ { {0xE2,0x80,0x91,0}, 3, '-' },
        /* Minus sign            U+2212 */ { {0xE2,0x88,0x92,0}, 3, '-' },
        /* En dash               U+2013 */ { {0xE2,0x80,0x93,0}, 3, '-' },
        /* Em dash               U+2014 */ { {0xE2,0x80,0x94,0}, 3, '-' },
        /* Figure dash           U+2012 */ { {0xE2,0x80,0x92,0}, 3, '-' },
        /* Full‑width hyphen‑minusU+FF0D */ { {0xEF,0xBC,0x8D,0}, 3, '-' },
        /* Small hyphen‑minus    U+FE63 */ { {0xEF,0x99,0xA3,0}, 3, '-' }
    };

    size_t i = 0, out = 0;
    while (i < len) {
        int replaced = 0;
        for (size_t t = 0; t < sizeof(table)/sizeof(table[0]); ++t) {
            if (i + table[t].seq_len <= len &&
                memcmp(buf + i, table[t].seq, table[t].seq_len) == 0) {
                buf[out++] = table[t].newchar;
                i += table[t].seq_len;
                replaced = 1;
                break;
            }
        }
        if (!replaced) {
            buf[out++] = buf[i++];
        }
    }
    return out;
}

uint8_t *loadMarkdown (const char *path, size_t *size)
{
    uint8_t *text = NULL;
    FILE *fp = fopen(path, "rb");
    if (!fp) abort_("cannot open text file");

    fseek(fp, 0, SEEK_END);
    *size = ftell(fp);
    rewind(fp);
    text = malloc(*size);
    fread(text, *size, 1, fp);
    fclose(fp);
    *size = replace_ugly(text, *size);
    return text;
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
    
    gray = (uint8_t *) malloc(sizeof(uint8_t) * (*w) * *h);
    
    for(int y = 0; y < *h; ++y)
    {
        row = gray + (*h - y - 1) * (*w);
        fread(row, 1, *w, fp);
        fseek(fp, padding, SEEK_CUR); 
    }
    fclose(fp);

    return gray;
}

static uint8_t *loadPng (const char *path, int *w, int *h) {
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

void printImage (int fd, const uint8_t *img)
{
    int new_h;
    uint8_t *scaled = NULL;
    
    printf("load with width=%d height=%d\n", src_w, src_h);
    scaled = scaleMax(img, src_w, src_h, &new_h);
    ditherAtkinson(scaled, allowed_w, new_h);
    // linefeed to 0
    write(fd, "\x1B\x33\x00", 3);
    printRaster(fd, scaled, allowed_w, new_h);
    free(scaled);
}


/**
 * print utf-8 binary (without bom, and only \n) with markdown format
 * 
 * @todo
 * - handle #, ##, ###
 * - handle bold
 * - handle italic as 2nd font?
 * - handle `code` as Font A
 * - handle unordered lists with "- "
 * - handle links (as bold)
 * - handle ordered list with "1. "
 */
void printMarkdown (int fd, const uint8_t *src, const size_t *size)
{
    int e;
    int is_bold = 0;
    const char *in_encoding  = "UTF-8";
    const char *out_encoding = "CP858";

    char *inbuf = (char *)src;
    size_t inbytesleft = *size;

    size_t outbuf_cap = *size;
    char *data = malloc(outbuf_cap);
    if (!data) abort_("malloc failed");

    char *outptr = data;
    size_t outbytesleft = outbuf_cap;

    iconv_t cd = iconv_open(out_encoding, in_encoding);
    if (cd == (iconv_t)-1) abort_("Failed to open conversion descriptor\n");

    size_t res = iconv(cd, &inbuf, &inbytesleft, &outptr, &outbytesleft);
    e = errno;
    if (res == (size_t)-1) {
        const char *name = strerror(e);
        size_t bad_index = (size_t)(inbuf - (char *)src);
        printf("iconv failed: errno=%d (%s), byte position %ld\n", e, name, bad_index);
        inbuf[bad_index] = '\0';
        printf("%s\n", inbuf);
        iconv_close(cd);
        free(data);
        abort_("iconv failed");
    }

    size_t out_used = outbuf_cap - outbytesleft;

    // font B (default)
    write(fd, "\x1B\x4D\x01", 3);

    for (size_t i = 0; i < (out_used-2); ++i)
    {
        if (data[i] == '\n' && data[i+1] == '\n')
        {
            write(fd, "\n", 1);
            // reset to small size
            write(fd, "\x1D\x21\x00", 3);
            //linefeed mini
            write(fd, "\x1B\x33\x10", 3);
        }
        else if (data[i] == '*' && data[i+1] == '*')
        {
            if (is_bold == 0) {
                is_bold = 1;
                write(fd, "\x1B\x45\x01", 3);
            } else {
                is_bold = 0;
                write(fd, "\x1B\x45\x00", 3);
            }
            i++;
        }
        else if (data[i] == '\n' && data[i+1] == '-' && data[i+2] == ' ')
        {
            // simple unordered list has newlines
            write(fd, "\n", 1);
        }
        else if (data[i] == '\n' && data[i+1] == '#')
        {
            // simple headlines
            
            write(fd, "\n", 1);
            // max font size
            write(fd, "\x1D\x21\x11", 3);
            i++;
            
            if ((i+1) < out_used && data[i+1] == '#') {
                // max-1 font size
                write(fd, "\x1D\x21\x10", 3);
                i++;
            }
            if ((i+1) < out_used && data[i+1] == '#') {
                // max-2 font size
                write(fd, "\x1D\x21\x01", 3);
                i++;
            }
            if ((i+1) < out_used && data[i+1] == ' ') i++;
            
        }
        else if (data[i] == '\n' && data[i+1] != '\n')
        {
            // ignore a single newline
        }
        else
        {
            write(fd, &(data[i]), 1);
        }
    }

    write(fd, &(data[out_used-2]), 2);
    // final newline to print rest
    write(fd, "\n", 1);
    
    iconv_close(cd);
    free(data);
}



void printMarkdown2 (int fd, uint8_t *src, const size_t *size)
{
    const char *in_encoding = "UTF-8";
    const char *out_encoding = "CP858";

    char *inbuf = (char *)src;
    size_t inbytesleft = *size;
    size_t outbytesleft = *size;
    char *data = NULL;

    iconv_t cd = iconv_open(out_encoding, in_encoding);
    if (cd == (iconv_t) -1)
    {
        abort_("Failed to open conversion descriptor\n");
    } 
    
    data = (char *) malloc(sizeof(char) * (*size));
    
    size_t result = iconv(cd, &inbuf, &inbytesleft, &data, &outbytesleft);
    if (result == (size_t)-1)
    {
        iconv_close(cd);
        free(data);
        abort_("iconv failed");
    }
    
    // font B (default)
    write(fd, "\x1B\x4D\x01", 3);
    
    // process first byte makes the rest easier
    write(fd, &(data[0]), 1);

    for (size_t i = 1; i < outbytesleft; ++i)
    {
        if (data[i-1] == '\n' && data[i] == '\n')
        {
            write(fd, "\n\n", 2);
            // reset to not-bold
            write(fd, "\x1B\x45\x00", 3);
        }
        else if (data[i-1] == '\n' && data[i] == '-')
        {
            // simple unordered list
            write(fd, "\n-", 2);
        }
        else if (data[i-1] == '\n' && data[i] == '#')
        {
            // simple headlines
            if ((i+1) < outbytesleft && data[i+1] == '#') i++;
            if ((i+1) < outbytesleft && data[i+1] == '#') i++;
            if ((i+1) < outbytesleft && data[i+1] == ' ') i++;
            
            // bold
            write(fd, "\x1B\x45\x01", 3);
        }
        else if (data[i-1] != '\n' && data[i] == '\n')
        {
            // ignore a single newline
        }
        else
        {
            write(fd, &(data[i]), 1);
        }
    }
    
    iconv_close(cd);
    free(data);
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

    const char *src_path = argv[1];
    const char *dev_path = argv[2];

    uint8_t *src = NULL;
    int fd;

    fd = open(dev_path, O_WRONLY);
    if (fd < 0) abort_("cannot open device for writing");
    // reset
    write(fd, "\x1B\x40", 2);
    // Codepage 858
    write(fd, "\x1B\x74\x13", 3);
    // CPI Mode
    write(fd, "\x1B\xC1\x01", 3);

    if (isMime(src_path, ".bmp") == 1)
    {
        src = loadBmp(src_path, &src_w, &src_h);
        printImage(fd, src);
    }
    else if (isMime(src_path, ".png") == 1)
    {
        src = loadPng(src_path, &src_w, &src_h);
        printImage(fd, src);
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
