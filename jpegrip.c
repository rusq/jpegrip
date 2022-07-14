
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

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "log.h"

#define BUF_SIZE 8192

// our lovely markers
#define IMAGE_START 0xd8 // start of jpeg image
#define IMAGE_END 0xd9   // end of image
#define PAD_CHAR 0xff

#define CODE_ERROR 0
#define CODE_OK 1

#ifndef MAX_FNAME
#define MAX_FNAME 260
#endif

unsigned long int gFileCount; // for extraction

int gSuspect;
int gFound;       // in jpeg_file
int gFound_start; // found start
int gFound_end;   // found end (ready for extraction);

// char* jname;

int findjpeg(const unsigned char *buffer, int bytes_read);
int extract(u_int64_t start, u_int64_t end, const int infile);

int main(int argc, char **argv) {
    int file;
    int bytes_read; // Number of bytes read
    u_int64_t foffset = 0;
    u_int64_t offset_begin = 0;
    u_int64_t offset_end = 0;

    unsigned char *buffer;
    int find_result;

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
                ftracef("Trace mode.\n");
            } else {
                flogf("Verbose mode.\n");
            }
        }
    }

    // (Parsing parameters) --------------------------------

    fprintf(stderr, "Ripping file: %s...\n", argv[1]);

    if ((file = open(argv[1], O_RDONLY)) == -1) {
        perror("open failed");
        free(buffer);
        exit(1);
    }

    flogf("\tfile \"%s\" opened successfully.\n", argv[1]);

    // Initializing ----------------------------------------
    gFileCount = 0;
    gSuspect = 0;

    buffer = (unsigned char *) malloc(BUF_SIZE);
    // -----------------------------------------------------

    // Main cycle ----------------------------------------------------
    do {
        bytes_read = read(file, buffer, BUF_SIZE);
        if (bytes_read == -1) {
            free(buffer);
            perror("read failed");
            exit(1);
        }

        if (bytes_read == 0) {
            fprintf(stderr, "EOF reached. Extraction finished.\n");
            if (gFileCount)
                fprintf(stderr, "Success: %lu JPEG files extracted.\n", gFileCount);
            else
                fprintf(stderr, "No JPEG files found in this file.\n");
            break;
        }

        find_result = findjpeg(buffer, bytes_read);

        if ((find_result) == -1) { // Nothing found
            continue;
        }

        foffset = lseek(file, -find_result, SEEK_CUR);
        if (gFound_start) {
            offset_begin = foffset - 1; //!!!!!!!!!!
            ftracef(" START: Found IMAGE_START @ 0x%.8llX\n", offset_begin);
            gFound_start = 0;
            continue;
        }
        if (gFound_end) {
            offset_end = foffset + 1;
            ftracef(" --- END: Found IMAGE_END @ 0x%.8llX\n", offset_end - 1);
            gFound_end = 0;
            gFound = 0;
            if (extract(offset_begin, offset_end, file) != CODE_OK) {
                free(buffer);
                close(file);
                exit(1);
            }
        }

    } while (bytes_read > 0);
    // (Main cycle) --------------------------------------------------

    close(file);

    free(buffer);

    return 0;
}

int findjpeg(const unsigned char *buffer, int bread) {

    int start_offset = -1;

    int i = 0;

    while (i <= (bread - 1)) {
        if (*buffer != PAD_CHAR) {
            i++;
            buffer++;
            continue;
        }
        i++;
        buffer++;
        if (i >= bread) {
            start_offset = 1;
            break;
        }
        if (!gFound) {
            if ((!gSuspect) && (*buffer == IMAGE_START)) {
                gSuspect = 1;

                if (i >= (bread - 5)) {
                    gSuspect = 0;
                    start_offset = 5;
                    break;
                }
                if ((*(buffer + 1) == 0xff) && (*(buffer + 2) == 0xe0) && (*(buffer + 3) == 0x00) &&
                    (*(buffer + 4) == 0x10)) {
                    gFound_start = gFound = 1;
                    gSuspect = 0;
                    start_offset = bread - i;
                    break;
                }
                gSuspect = 0;
                break;
            }
        }
        if (gFound) {
            while (*buffer == PAD_CHAR) {
                i++;
                buffer++;
            }
            if (*buffer == IMAGE_END) {
                gFound_end = 1;
                start_offset = bread - i;
                break;
            }
            if (*buffer == IMAGE_START) {
                gFound_start = 1;
                start_offset = bread - i;
                break;
            }
        }
        buffer++;
        i++;
    }

    return start_offset;
}

int extract(u_int64_t start, u_int64_t end, const int infile) {
    int f;
    char filename[MAX_FNAME];

    int numbuffs, remainer;
    char *buffer;
    int i;
    int ret = CODE_ERROR;

    u_int64_t stored_pos;

    if ((buffer = (char *) malloc(BUF_SIZE)) == NULL) {
        fprintf(stderr, "buffer memory allocation error\n");
        return 1;
    }

    sprintf(filename, "%s%08lu.jpg", "jpg", gFileCount);
    flogf("\tWriting file: `%s' (%llu bytes)\n", filename, end - start);

    numbuffs = (int) (end - start) / BUF_SIZE;
    remainer = (int) (end - start) % BUF_SIZE;

    ftracef("\t\tnumbuffs=%d,remainer=%d\n", numbuffs, remainer);

    if ((f = open(filename, O_WRONLY | O_CREAT | O_TRUNC,
                  S_IREAD | S_IWRITE | S_IRGRP | S_IROTH)) == -1) {
        perror("extract(): failed to create a file");
        free(buffer);
        return CODE_ERROR;
    }

    stored_pos = lseek(infile, 0, SEEK_CUR); // Savig file position
    // Saving ------------------------------------------------
    ftracef("\t\tWriting numbuffs...\n");
    lseek(infile, start, SEEK_SET);
    if (numbuffs) {
        for (i = 0; i < numbuffs; i++) {
            if ((read(infile, buffer, BUF_SIZE)) == -1) {
                perror("extract(): read()");
                goto cleanup;
            }
            if ((write(f, buffer, BUF_SIZE)) == -1) {
                perror("extract(): write()");
                goto cleanup;
            }
        }
    }

    ftracef("\t\tWriting remainer...");
    read(infile, buffer, remainer);
    write(f, buffer, remainer);

    // (Saving) ----------------------------------------------
    gFileCount++;

    lseek(infile, stored_pos, SEEK_SET);

    ret = CODE_OK;
cleanup:
    close(f);
    free(buffer);
    return ret;
}
