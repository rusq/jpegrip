/*
 * JPEG the Ripper
 * Programme for ripping out JPEG files out of a mess
 *
 * Copyright 2005 by Rustam Gilyazov (github: @rusq)
 * May be distributed under the GNU General Public License
 */
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include "log.h"
#include "jpegrip2.h"
#include "compat.h"

void usage(const char *me);
int run2(const char *filename);

int main(int argc, char **argv) {
    char *filename = 0;
    char *verboseFlag = 0;

    if (argc <= 1) {
        usage(argv[0]);
        return 2;
    }

    if (argv[1][0] == '-') {
        /* determine which parameter is which */
        if (argc == 2) {
            /* user specified flag, but not the file */
            usage(argv[0]);
        }
        verboseFlag = argv[1];
        filename = argv[2];
    } else {
        filename = argv[1];
    }

    if (verboseFlag != 0) {
        if (strncmp(verboseFlag, "-v", 2) == 0) {
            set_log_level(LOG_LEVEL_VERBOSE);
            if (strncmp(verboseFlag, "-vv", 3) == 0) {
                set_log_level(LOG_LEVEL_TRACE);
                ltrace("Trace mode.\n");
            } else {
                lverbose("Verbose mode.\n");
            }
        }
    }

    if (run2(filename) == -1) {
        llog("there were errors");
        return 1;
    }
    return 0;
}

void usage(const char *me) {
    printf("jpegrip - find and extract jpeg files in binary files.\n");
    printf("Usage: %s [-v|-vv] <mess.ext>\n", me);
    printf("\t-v\t- verbose mode\n\t-vv\t- very verbose mode\n");
}

int run2(const char *filename) {
    FILE *f;
    int num_files = 0;

    if ((f = fopen(filename, "rb")) == 0) {
        perror("error opening input file");
        return 0;
    }
    num_files = rip_jpeg2(f);
    if (num_files == -1) {
        return -1;
    }
    llog("ok, files extracted: %d\n", num_files);
    return num_files;
}

/*
int run(char *filename) {
    int hSource = 0;
    int num_files = 0;

    llog("Ripping file: %s...\n", filename);

    if ((hSource = open(filename, O_RDONLY)) == -1) {
        perror("open failed");
        return -1;
    }

    lverbose("\tfile \"%s\" opened successfully.\n", filename);

    num_files = rip_jpeg(hSource);
    if (num_files < 0) {
        close(hSource);
        return -1;
    }
    llog("%d files extracted from %s\n", num_files, filename);

    close(hSource);
    return 0;
}
*/
