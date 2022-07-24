#ifndef _JPEG_H
#define _JPEG_H

#ifndef RET_ERROR
#define RET_ERROR 0
#endif
#ifndef RET_OK
#define RET_OK 1
#endif

/* jpeg_hdr_size reads the JPEG file and returns the header size, that can be
certainly skipped. On error, it returns RET_ERROR (0). */
long jpeg_hdr_size(FILE *f);

#endif
