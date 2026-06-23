#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <iconv.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "globals.h"
#include "markdown.h"

size_t replaceUgly (uint8_t *buf, size_t len)
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
    if (!fp) g_abort("cannot open text file");

    fseek(fp, 0, SEEK_END);
    *size = ftell(fp);
    rewind(fp);
    text = malloc(*size);
    fread(text, *size, 1, fp);
    fclose(fp);
    *size = replaceUgly(text, *size);
    return text;
}

/**
 * print utf-8 binary (without bom, and only \n) with markdown format
 * 
 * - handle #, ##, ###
 * - handle bold (font B 2x width)
 * - handle emphasize as underline + bold
 * - handle `code` as font B
 * - handle unordered lists with "- "
 */
void printMarkdown (int fd, const uint8_t *src, const size_t *size)
{
    int e;
    int is_bold = 0;
    int is_emphasize = 0;
    int is_code = 0;
    int is_list = 0;
    int line_width = 0;
    int char_width = 0;
    const char *in_encoding  = "UTF-8";
    const char *out_encoding = "CP858";

    char *inbuf = (char *)src;
    size_t inbytesleft = *size;

    size_t outbuf_cap = *size;
    char *data = malloc(outbuf_cap);
    if (!data) g_abort("malloc failed");

    char *outptr = data;
    size_t outbytesleft = outbuf_cap;

    iconv_t cd = iconv_open(out_encoding, in_encoding);
    if (cd == (iconv_t)-1) g_abort("Failed to open conversion descriptor\n");

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
        g_abort("iconv failed");
    }

    size_t out_used = outbuf_cap - outbytesleft;

    // font A
    write(fd, "\x1B\x4D\x00", 3);
    
    for (size_t i = 0; i < (out_used-2); ++i)
    {
        if (data[i] == '\n' && data[i+1] == '\n')
        {
            write(fd, "\n", 1);
            // font A
            write(fd, "\x1B\x4D\x00", 3);
            char_width = 12;
            // reset to small size
            write(fd, "\x1D\x21\x00", 3);
            // non bold
            write(fd, "\x1B\x45\x00", 3);
            //linefeed mini
            write(fd, "\x1B\x33\x10", 3);
            // first line without margin
            write(fd, "\x1D\x4C\x00\x00", 4);
            
            is_list = 0;
        }
        else if (data[i] == '*' && data[i+1] == '*')
        {
            if (is_bold == 0) {
                char_width = 18;
                is_bold = 1;
                // font B
                write(fd, "\x1B\x4D\x01", 3);
                // 2x width
                write(fd, "\x1D\x21\x10", 3);

            } else {
                char_width = 12;
                is_bold = 0;
                // font A
                write(fd, "\x1B\x4D\x00", 3);
                // normal width and height
                write(fd, "\x1D\x21\x00", 3);
            }
            i++;
        }
        else if (data[i] == ' ' && data[i+1] == ' ')
        {
            // shrink 2 or 3 spaces
            if (data[i+2] == ' ') i++;
            i++;
        }
        else if (i > 0 && data[i] == '`' && data[i+1] != '`')
        {
            if (data[i-1] != '`') {
                if (is_code == 0) {
                    char_width = 9;
                    is_code = 1;
                    // font B
                    write(fd, "\x1B\x4D\x01", 3);
                } else {
                    char_width = 12;
                    is_code = 0;
                    // font A
                    write(fd, "\x1B\x4D\x00", 3);
                }
            }
        }
        else if (i > 0 && data[i] == '*' && data[i+1] != '*')
        {
            if (data[i-1] != '*') {
                if (is_emphasize == 0) {
                    char_width = 12;
                    is_emphasize = 1;
                    // underline 2
                    write(fd, "\x1B\x2D\x02", 3);
                    // and bold
                    write(fd, "\x1B\x45\x01", 3);
                } else {
                    char_width = 12;
                    is_emphasize = 0;
                    // no-underline
                    write(fd, "\x1B\x2D\x00", 3);
                    // and non bold
                    write(fd, "\x1B\x45\x00", 3);
                }
            }
        }
        else if (data[i] == '\n' && data[i+1] == '-' && data[i+2] == ' ')
        {
            // simple unordered list has newlines
            write(fd, "\n", 1);
            line_width = 0;
            is_list = 1;
            // first line without margin
            write(fd, "\x1D\x4C\x00\x00", 4);
        }
        else if (
            (data[i] == '\n' && data[i+1] == '#') ||
            (i == 0 && data[i] == '#')
        ) {
            // simple headlines
            
            write(fd, "\n", 1);
            // font A
            write(fd, "\x1B\x4D\x00", 3);
            // 2x height 2x width
            write(fd, "\x1D\x21\x11", 3);
            char_width = 24;
            i++;
            
            if ((i+1) < out_used && data[i+1] == '#') {
                // font B
                write(fd, "\x1B\x4D\x01", 3);
                // 2x height 2x width
                write(fd, "\x1D\x21\x11", 3);
                char_width = 18;
                i++;
            }
            if ((i+1) < out_used && data[i+1] == '#') {
                // font A
                write(fd, "\x1B\x4D\x00", 3);
                // 2x height 1x width
                write(fd, "\x1D\x21\x01", 3);
                char_width = 12;
                // bold
                write(fd, "\x1B\x45\x01", 3);
                i++;
            }
            if ((i+1) < out_used && data[i+1] == ' ') i++;
            
        }
        else if (data[i] == '\n' && data[i+1] != '\n')
        {
            // ignore a single newline
            if (i > 0 && data[i-1] != '\n') {
                write(fd, " ", 1);
                line_width += char_width;
            }
        }
        else
        {
            line_width += char_width;
            if (is_list > 0 && line_width > 384) {
                // force newline
                write(fd, "\n", 1);
                // left margin 24
                write(fd, "\x1D\x4C\24\x00", 4);
                // new intial line width
                line_width = 24 + char_width;
            }
            write(fd, &(data[i]), 1);
        }
    }

    /// @todo this end is a bit to dirty
    write(fd, &(data[out_used-2]), 2);
    // final newline to print rest
    write(fd, "\n", 1);
    
    iconv_close(cd);
    free(data);
}
