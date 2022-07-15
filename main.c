#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include "log.h"
#include "jpegrip.h"

int run(char* filename);

int main(int argc, char **argv) {
    if (argc <= 1) {
        printf("Usage: %s <mess.ext> [-v|-vv]\n", argv[0]);
        printf("\t-v\t- verbose mode\n\t-vv\t- very verbose mode\n");
        return 1;
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

    if(run(argv[1]) == -1) {
        llog("there were errors");
        return 1;
    }
    return 0;
}

int run(char* filename) {
    int hSource=0;
    int num_files = 0;


    llog("Ripping file: %s...\n", filename);

    if ((hSource = open(filename, O_RDONLY)) == -1) {
        perror("open failed");
        return -1;
    }

    lverbose("\tfile \"%s\" opened successfully.\n", filename);

    num_files = rip(hSource);
    if (num_files < 0) {
        close(hSource);
        return -1;
    }
    llog("%d files extracted from %s\n", num_files, filename);

    close(hSource);
    return 0;
}
