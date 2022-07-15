
/*
 * JPEGRIP.C
 *
 * JPEG the Ripper
 * Programme for ripping out JPEG files out of a mess
 *
 * Copyright 2005 by Rustam Gilyazov (msdos.sys@bk.ru)
 * May be distributed under the GNU General Public License
 */

#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "log.h"

#define BUF_SIZE 8192

// our lovely markers
#define MARKER 0xff
#define IMAGE_START 0xd8 // start of jpeg image
#define IMAGE_END 0xd9   // end of image

#define CODE_ERROR 0
#define CODE_OK 1

#ifndef MAX_FNAME
#define MAX_FNAME 260
#endif

#define NOT_FOUND 1

struct search_state {
    int bSuspect;
    int bFound;       // in jpeg_file
    int bFound_start; // found start
    int bFound_end;   // found end (ready for extraction)
};

// char* jname;

int findjpeg(struct search_state *ss, unsigned char *buffer, int bytes_read);
int extract(const int hInFile, const uint32_t fileCount, uint64_t start, uint64_t end);

int main(int argc, char **argv) {
    int hSource;
    int bytes_read; // Number of bytes read
    uint64_t foffset = 0;
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;

    unsigned char *buffer; // file data buffer
    int reverse_offset;
    uint32_t file_count = 0;
    struct search_state ss = {0};

    // Parsing parameters ----------------------------------
    if (argc <= 1) {
        printf("Usage: %s <mess.ext> [-v|-vv]\n", argv[0]);
        printf("\t-v\t- verbose mode\n\t-vv\t- very verbose mode\n");
        exit(1);
    }

    if (argc == 3) {
        if (strncmp(argv[2], "-v", 2) == 0) {
            set_log_level(LOG_LEVEL_VERBOSE);
            if (strncmp(argv[2], "-vv", 3) == 0) {
                set_log_level(LOG_LEVEL_TRACE);
                ltrace("Trace mode.\n");
            } else {
                lverbose("Verbose mode.\n");
            }
        }
    }

    // (Parsing parameters) --------------------------------

    llog("Ripping file: %s...\n", argv[1]);

    if ((hSource = open(argv[1], O_RDONLY)) == -1) {
        perror("open failed");
        free(buffer);
        exit(1);
    }

    lverbose("\tfile \"%s\" opened successfully.\n", argv[1]);

    // Initializing ----------------------------------------
    buffer = (unsigned char *) malloc(BUF_SIZE);
    // -----------------------------------------------------

    // Main cycle ----------------------------------------------------
    do {
        bytes_read = read(hSource, buffer, BUF_SIZE);
        if (bytes_read == -1) {
            perror("read failed");
            free(buffer);
            close(hSource);
            exit(1);
        } else if (bytes_read == 0) {
            llog("EOF reached. Extraction finished.\n");
            if (file_count > 0) {
                llog("Success: %d JPEG files extracted.\n", file_count);
            } else {
                llog("No JPEG files found in this file.\n");
            }
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
        }
        if (ss.bFound_end) {
            offset_end = foffset;
            ltrace(" --- END: Found IMAGE_END @ 0x%.8llX\n", offset_end - 1);
            ss.bFound_end = 0;
            ss.bFound = 0;
            if (extract(hSource, file_count, offset_begin, offset_end) != CODE_OK) {
                free(buffer);
                close(hSource);
                exit(1);
            }
            file_count++;
        }

    } while (bytes_read > 0);
    /* (Main cycle) -------------------------------------------------- */

    free(buffer);
    close(hSource);

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
                if ((*(buffer + 1) == 0xff) && (*(buffer + 2) == 0xe0) && (*(buffer + 3) == 0x00) &&
                    (*(buffer + 4) == 0x10)) {
                    ss->bFound_start = ss->bFound = 1;
                    ss->bSuspect = 0;
                    // reverse_offset = -(bytes_read - i);
                    // break;
                    return -(bytes_read - i+1);
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
                reverse_offset = -(bytes_read - i-1);
                break;
            }
        }
        buffer++;
        i++;
    }

    return reverse_offset;
}

int extract(const int hInFile, const uint32_t fileCount, uint64_t start, uint64_t end) {
    int hOutFile; /* output file handle */
    char filename[MAX_FNAME];

    int numbuffs, remainer;
    char *buf;
    int i;
    int ret = CODE_ERROR;

    uint64_t stored_pos;

    if ((buf = (char *) malloc(BUF_SIZE)) == 0) {
        llog("buffer memory allocation error\n");
        return 1;
    }

    sprintf(filename, "jpg%08d.jpg", fileCount);
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

    stored_pos = lseek(hInFile, 0, SEEK_CUR); // Savig file position
    // Saving ------------------------------------------------
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

    ltrace("\t\tWriting remainer...");
    read(hInFile, buf, remainer);
    write(hOutFile, buf, remainer);

    // (Saving) ----------------------------------------------

    lseek(hInFile, stored_pos, SEEK_SET);

    ret = CODE_OK;
cleanup:
    close(hOutFile);
    free(buf);
    return ret;
}
