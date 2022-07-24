#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compat.h"
#include "jpeg.h"
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
	unsigned char *buf;

	if (seq_sz == 0) {
		llog("search_file:  zero sequence size");
		return EOF;
	}

	if (fseek(hFile, start_pos, SEEK_SET) == -1) {
		perror("search_file:  seek failed");
		return ERROR;
	}

	if ((buf = malloc(BUF_SIZE)) == 0) {
		perror("memory allocation error");
		return ERROR;
	}

	for (;;) {
		int bytes_read = 0;
		int buf_offset = 0;
		int file_pos = ftell(hFile);

		if ((bytes_read = fread(buf, sizeof(unsigned char), BUF_SIZE, hFile)) == 0) {
			if (feof(hFile) || (bytes_read < seq_sz)) {
				free(buf);
				return EOF;
			} else {
				perror("search_file:  read error");
				goto looser;
			}
		}

		buf_offset = search_buf(buf, bytes_read, seq, seq_sz);
		if (buf_offset != -1) {
			free(buf);
			/* return the offset of the finding */
			return file_pos + buf_offset;
		}

		if (bytes_read == seq_sz) {
			/* we have scanned the last seq_sz bytes of the file, and did not
			find the sequence we were looking for, it does not make sense to
			continue doing this */
			free(buf);
			return EOF;
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

/* fmt_string creates a format string for format_name.  buf is the output
buffer, and buf_sz should contain a non-zero size of the buffer buf. If it
succeeds it returs number of bytes written to the buffer.  It will return 0 if
it fails. */
int fmt_string(char *buf, const int buf_sz, const char *prefix, const int digits, const char *ext) {
	if (buf_sz == 0) {
		llog("filename size can't be zero");
		return -1;
	}
	memset(buf, 0, buf_sz);
	/* create format string */
	return snprintf(buf, buf_sz, "%s%%0%dld.%s", prefix, digits, ext);
}

int format_name(char *output, const int output_sz, const char *fmt, const int sequence) {
	return snprintf(output, output_sz, fmt, sequence);
}

/* min returns the minimum value of x and y */
int min(const int x, const int y) { return x < y ? x : y; }

/* extract extracts the size chunk of data from hFile starting at start_offset,
   and writes it to the new file which it creates.  The filename is formed by
   sprintfing filename_fmt, and sequence.  Sample filename_fmt: "jpg%05d.jpg".
   If the sequence is 42, then, the output filename will be "jpg00042.jpg".  It
   returns 0 if error occurs or if there was nothing to do, EOF if we were
   trying to read, and encountered an input file EOF (treat as an error), and
   bytes written, if everything went well (it should equal to size).
*/
long extract(FILE *hFile, const char *filename, long start_offset, long size) {
	long remain = size;
	unsigned char *buf; /* temporary buffer */
	FILE *f;			/* output file */

	if (size == 0) {
		llog("extract:  nothing to do");
		return 0;
	}

	if (fseek(hFile, start_offset, SEEK_SET) == -1) {
		perror("extract:  failed to reposition the file to start offset");
		return 0;
	}

	if ((f = fopen(filename, "wb+")) == 0) {
		perror("failed to create the output file");
		return 0;
	}

	if ((buf = malloc(BUF_SIZE)) == 0) {
		perror("extract:  failed to allocate memory");
		goto looser;
	}

	do {
		int bytes_read = 0, bytes_written = 0;
		if ((bytes_read = fread(buf, sizeof(unsigned char), min(BUF_SIZE, remain), hFile)) == 0) {
			if (feof(hFile)) {
				free(buf);
				fclose(f);
				return EOF; /* attempted to read past the file end */
			} else {
				perror("extract:  read error");
				goto looser;
			}
		}
		if ((bytes_written = fwrite(buf, sizeof(unsigned char), bytes_read, f)) == 0) {
			perror("extract:  write error");
			goto looser;
		}
		if (bytes_read != bytes_written) {
			llog("extract:  unexpected number of bytes written: read=%d != written=%d", bytes_read,
				 bytes_written);
			goto looser;
		}
		remain -= bytes_written;
	} while (remain > 0);

	free(buf);
	fclose(f);
	return size;
looser:
	free(buf);
	fclose(f);
	return 0;
}

int rip_jpeg(FILE *hFile) {
	long num_files = 0;
	long blob_start = 0, blob_end = 0;
	char output_fmt[MAX_FNAME];

	if (fmt_string(output_fmt, MAX_FNAME, "jpg", 8, "jpg") < 0) {
		llog("failed to create a output format string");
		return -1;
	}

	for (;;) {
		char output_name[MAX_FNAME];
		int output_file_sz = 0;
		int hdr_sz = 0;

		/* search for start */
		blob_start = search_file(hFile, blob_end, jpeg_begin, sizeof(jpeg_begin));
		if (blob_start == ERROR) {
			perror("rip_jpeg: search for jepg start");
			return -1;
		} else if (blob_start == EOF) {
			return num_files;
		}
		llog("%8ld: found start at %08lX\n", num_files + 1, blob_start);

		/* reset the file position and try to determine the header size */
		if (fseek(hFile, blob_start, SEEK_SET) == -1) {
			perror("seek failed");
			return -1;
		}
		if ((hdr_sz = jpeg_hdr_size(hFile)) == RET_ERROR) {
			llog("unable to determine header size\n");
			return -1;
		}

		/* search for ending */
		blob_end = search_file(hFile, blob_start + hdr_sz, jpeg_end, sizeof(jpeg_end));
		if (blob_end == ERROR) {
			perror("rip_jpeg: search for jpeg end");
			return -1;
		} else if (blob_start == EOF) {
			llog("search terminated prematurely (found start at %ld, but no end)\n");
			return num_files;
		}
		/* blob_end was pointing at the beginning of the sequence, we need end of it.
		 */
		blob_end += sizeof(jpeg_end);
		output_file_sz = blob_end - blob_start;

		llog("%8ld:   found end at %08lX, detected file size: %ld\n", num_files + 1, blob_end,
			 output_file_sz);

		if (format_name(output_name, MAX_FNAME, output_fmt, num_files) < 0) {
			llog("failed to generate a filename");
			return -1;
		}
		ltrace("came up with this brilliant name: %s\n", output_name);
		if (extract(hFile, output_name, blob_start, output_file_sz) != output_file_sz) {
			llog("error extracting %ld bytes to %s\n", output_file_sz, output_name);
			return -1;
		};

		llog("%8ld:   written to:\t%s\n", num_files + 1, output_name);

		num_files++;
	}
}
