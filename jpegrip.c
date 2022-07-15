
/*
 * JPEGRIP.C
 *
 * JPEG the Ripper
 * Programme for ripping out JPEG files out of a mess
 *
 * Copyright 2005 by Rustam Gilyazov (msdos.sys@bk.ru)
 * May be distributed under the GNU General Public License
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "compat.h"
#include "log.h"

#define BUF_SIZE 8192

/* our lovely markers */
#define MARKER 0xff
#define IMAGE_START 0xd8 /* start of jpeg image */
#define IMAGE_END 0xd9   /* end of image */

#define CODE_ERROR 0
#define CODE_OK 1

#define NOT_FOUND 1

struct search_state {
    int bSuspect;
    int bFound;       /* in jpeg_file */
    int bFound_start; /* found start */
    int bFound_end;   /* found end (ready for extraction) */
};

const unsigned char jpeg_signature[5] = {0xd8, 0xff, 0xe0, 0x00,
                                         0x10}; /* jpeg signature we're looking for */

int findjpeg(struct search_state *ss, unsigned char *buffer, int bytes_read);
int extract(const int hInFile, const int sequence, offset_t start, offset_t end);

/* rip_jpeg scans through the open file hSource, and extracts all jpeg files
found in it.  It will return the number of files extracted, or -1 on error */
int rip_jpeg(const int hSource) {
    offset_t foffset = 0;
    offset_t offset_begin = 0;
    offset_t offset_end = 0;

    int reverse_offset = 0;
    int bytes_read = 0; /* Number of bytes read */
    int file_count = 0;
    struct search_state ss = {0};

    unsigned char *buffer; /* file data buffer */
    buffer = (unsigned char *) malloc(BUF_SIZE);

    /* Main cycle ---------------------------------------------------- */
    do {
        bytes_read = read(hSource, buffer, BUF_SIZE);
        if (bytes_read == -1) {
            perror("read failed");
            free(buffer);
            return -1;
        } else if (bytes_read == 0) {
            llog("EOF reached. Extraction finished.\n");
            break;
        }

        reverse_offset = findjpeg(&ss, buffer, bytes_read);

        if ((reverse_offset) == NOT_FOUND) {
            /* Nothing found */
            continue;
        }

        foffset = lseek(hSource, reverse_offset, SEEK_CUR);
        if (ss.bFound_start) {
            offset_begin = foffset;
            llog(" START: Found IMAGE_START @ 0x%.8llX\n", offset_begin);
            ss.bFound_start = 0;
            continue;
        } else if (ss.bFound_end) {
            offset_end = foffset;
            ltrace(" --- END: Found IMAGE_END @ 0x%.8llX\n", offset_end);
            ss.bFound_end = 0;
            ss.bFound = 0;
            if (extract(hSource, file_count, offset_begin, offset_end) != CODE_OK) {
                free(buffer);
                return -1;
            }
            file_count++;
        }
    } while (bytes_read > 0);

    free(buffer);
    return file_count;
}

int findjpeg(struct search_state *ss, unsigned char *buffer, int bytes_read) {
    int reverse_offset = NOT_FOUND;
    int i = 0;

    while (i < bytes_read) {
        if (*buffer != MARKER) {
            i++;
            buffer++;
            continue;
        }
        i++;
        buffer++;
        if (i >= bytes_read) {
            /* we found a marker at the end of the buffer, suspect that we
            might have a positive match, need to tell the reader to read a new
            buffer 1 byte back */
            reverse_offset = -1;
            break;
        }
        if (!ss->bFound) {
            if ((!ss->bSuspect) && (*buffer == IMAGE_START)) {
                ss->bSuspect = 1;

                if (i >= (bytes_read - 5)) {
                    /* we are approaching the end of the buffer, and we have
                    a suspect, we might need to reverse a bit to read the full
                    file signature */
                    /* reset the suspect flag, so we can start from scratch. */
                    ss->bSuspect = 0;
                    reverse_offset = -5;
                    break;
                }
                if ((memcmp(buffer, &jpeg_signature, sizeof(jpeg_signature))) == 0) {
                    ss->bFound_start = ss->bFound = 1;
                    ss->bSuspect = 0;
                    return -(bytes_read - i + 1);
                }
                ss->bSuspect = 0;
                break;
            }
        }
        if (ss->bFound) {
            while (*buffer == MARKER) {
                i++;
                buffer++;
            }
            if (*buffer == IMAGE_END) {
                ss->bFound_end = 1;
                reverse_offset = -(bytes_read - i - 1);
                break;
            }
        }
        buffer++;
        i++;
    }

    return reverse_offset;
}

/* extract extracts the portion of the file hInFile into a file with a generated
name, having some suffix and a sequence in its filename */
int extract(const int hInFile, const int sequence, offset_t start, offset_t end) {
    int hOutFile; /* output file handle */
    char filename[MAX_FNAME];
    int numbuffs, remainer;
    int i;
    int ret = CODE_ERROR;
    char *buf;

    offset_t stored_pos;

    if ((buf = (char *) malloc(BUF_SIZE)) == 0) {
        llog("buffer memory allocation error\n");
        return 1;
    }

    sprintf(filename, "jpg%08d.jpg", sequence);
    lverbose("\tWriting file: `%s' (%llu bytes)\n", filename, end - start);

    numbuffs = (int) (end - start) / BUF_SIZE;
    remainer = (int) (end - start) % BUF_SIZE;

    ltrace("\t\tnumbuffs=%d,remainer=%d\n", numbuffs, remainer);

    if ((hOutFile = open(filename, O_WRONLY | O_CREAT | O_TRUNC,
                         S_IREAD | S_IWRITE | S_IRGRP | S_IROTH)) == -1) {
        perror("extract(): failed to create a file");
        free(buf);
        return CODE_ERROR;
    }

    stored_pos = lseek(hInFile, 0, SEEK_CUR); /* Savig file position */
    /* Saving ------------------------------------------------ */
    ltrace("\t\tWriting numbuffs...\n");
    lseek(hInFile, start, SEEK_SET);
    if (numbuffs) {
        for (i = 0; i < numbuffs; i++) {
            if ((read(hInFile, buf, BUF_SIZE)) == -1) {
                perror("extract(): read()");
                goto cleanup;
            }
            if ((write(hOutFile, buf, BUF_SIZE)) == -1) {
                perror("extract(): write()");
                goto cleanup;
            }
        }
    }

    ltrace("\t\tWriting remainer...\n");
    if ((read(hInFile, buf, remainer)) == -1) {
        perror("source read error");
        goto cleanup;
    }
    if ((write(hOutFile, buf, remainer)) == -1) {
        perror("target write error");
        goto cleanup;
    }

    /* (Saving) ---------------------------------------------- */

    lseek(hInFile, stored_pos, SEEK_SET);

    ret = CODE_OK;
cleanup:
    close(hOutFile);
    free(buf);
    return ret;
}
