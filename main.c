/*
JPEG the Ripper
Copyright (c) 2005,2022 github.com/rusq

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include "log.h"
#include "jpegrip.h"

void usage(const char *me);
int run(const char *filename);

int main(int argc, char **argv) {
	char *filename = 0;
	int i=0;

	if (argc <= 1) {
		goto looser;
	}

	for (i=1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == 'v') {
				set_log_level(LOG_LEVEL_VERBOSE);
				if (argv[i][2] == 'v') {
					set_log_level(LOG_LEVEL_TRACE);
					ltrace("Trace mode");
				} else {
					lverbose("Verbose mode");
				}
			}
		} else {
			filename = argv[i];
		}
	}

	if (filename == NULL) {
		goto looser;
	}

	if (run(filename) == -1) {
		llog("there were errors");
		return 1;
	}
	return 0;

looser:
	usage(argv[0]);
	return 2;
}

void usage(const char *me) {
	printf("jpegrip %s - find and extract jpeg files in binary files.\n", JPEGRIP_VERSION);
	printf("Usage: %s [-v|-vv] <mess.ext>\n", me);
	printf("\t-v\t- verbose mode\n\t-vv\t- very verbose mode\n");
}

int run(const char *filename) {
	FILE *f;
	int num_files = 0;

	llog("Ripping file: %s...\n", filename);
	if ((f = fopen(filename, "rb")) == 0) {
		perror("error opening input file");
		return 0;
	}
	num_files = rip_jpeg(f);
	if (num_files == -1) {
		return -1;
	}
	llog("%d files extracted from %s\n", num_files, filename);
	return num_files;
}
