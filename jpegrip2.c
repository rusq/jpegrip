#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compat.h"
#include "log.h"

#define BUF_SIZE 16384

#define ERROR -2
#define NOT_FOUND -3

/* jpeg signatures we're looking for */
const unsigned char jpeg_begin[6] = {0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10};
const unsigned char jpeg_end[2] = {0xff, 0xd9};

/* search_buf searches the buf of size buf_sz for the presense of byte seq of
size seq_sz.  It will return -1 if the sequence not found, or an offset of the
sequence in buffer (can be 0). */
int search_buf(const unsigned char *buf, const int buf_sz, const unsigned char *seq,
               const int seq_sz) {
    int i;
    int offset = -1;

    if (buf_sz < seq_sz) {
        return -1;
    }

    for (i = 0; i <= (buf_sz - seq_sz); ++i) {
        if ((memcmp((buf + i), seq, seq_sz)) == 0) {
            offset = i;
            break;
        }
    }
    return offset;
}

/* search_file searches the hFile, starting at start_pos, for the presense of
seq, that has seq_sz length.  It will return the offset of the first byte of the
sequence in the file.  If it is unsuccessful, it will return an ERROR, or EOF,
if end of file is encountered while searching for the sequence */
long search_file(FILE *hFile, long start_pos, const unsigned char *seq, const int seq_sz) {
    int bytes_read = 0;
    unsigned char *buf;
    int file_pos; /* current file offset */
    int buf_offset;

    if (fseek(hFile, start_pos, SEEK_SET) == -1) {
        perror("search_file:  seek failed");
        return ERROR;
    }

    if ((buf = (unsigned char *) malloc(BUF_SIZE)) == 0) {
        perror("memory allocation error");
        return ERROR;
    }

    for (;;) {
        file_pos = ftell(hFile);
        if ((bytes_read = fread(buf, sizeof(unsigned char), BUF_SIZE, hFile)) == 0) {
            if (feof(hFile)) {
                free(buf);
                return EOF;
            } else {
                perror("search_file:  read error");
                goto looser;
            }
        }

        /* we may read less than seq_sz, need to account for it */

        buf_offset = search_buf(buf, bytes_read, seq, seq_sz);
        if (buf_offset != -1) {
            free(buf);
            /* return the offset of the finding */
            return file_pos + buf_offset;
        }

        /* reversing the file pointer for seq_sz bytes to make sure that
        there's a buffer overlap, in case the sequence is on the border
        between buffers */
        if (fseek(hFile, -seq_sz, SEEK_CUR) == -1) {
            perror("search_file:  seek error");
            goto looser;
        }
    }
looser:
    free(buf);
    return ERROR;
}

int rip_jpeg2(FILE *hFile) {
    long blob_start = 0, blob_end = 0;
    long num_files = 0;

    for (;;) {
        blob_start = search_file(hFile, blob_end, jpeg_begin, sizeof(jpeg_begin));
        if (blob_start == ERROR) {
            perror("rip_jpeg: search for jepg start");
            return -1;
        } else if (blob_start == EOF) {
            return num_files;
        }
        llog("found start at %08lX\n", blob_start);

        blob_end = search_file(hFile, blob_start+sizeof(jpeg_begin), jpeg_end, sizeof(jpeg_end));
        if (blob_end == ERROR) {
            perror("rip_jpeg: search for jpeg end");
            return -1;
        } else if (blob_start == EOF) {
            llog("search terminated prematurely (found start at %ld, but no end)\n");
            return num_files;
        }
        /* blob_end was pointing at the beginning of the sequence, we need end
        */
        blob_end+=sizeof(jpeg_end); 

        llog("found end   at %08lX\n", blob_end);
        
        /* extract */
        
        num_files++;
    }
}
