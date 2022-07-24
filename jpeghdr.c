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