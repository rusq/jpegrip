#ifndef _JPEG_H
#define _JPEG_H

#ifndef RET_ERROR
#define RET_ERROR 0
#endif
#ifndef RET_OK
#define RET_OK 1
#endif

/* jpeg_hdr_size reads the JPEG file and returns the header size, that can be
certainly skipped. On error, it returns 0. */
int jpeg_hdr_size(FILE *f);

#endif
