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
#include <stdlib.h>
#include "jpeg.h"
#include "log.h"

int main(int argc, char *argv[]) {
	FILE *f;
	int hdr_sz;

	if (argc < 2) {
		printf("jpeghdr prints the header size of the JPEG file\n");
		printf("usage: %s <file.jpg>\n", argv[0]);
		return 1;
	}
	
	if ((f = fopen(argv[1], "rb")) == NULL) {
		perror("open error");
		return 1;
	}
	set_log_level(LOG_LEVEL_TRACE);
	
	if (!(hdr_sz = jpeg_hdr_size(f))) {
		fclose(f);
		fprintf(stderr, "there were errors\n");
		return 1;
	}

	printf("detected header size: %d\n", hdr_sz);

	fclose(f);

	return 0;
}
