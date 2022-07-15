#ifndef __JR_COMPAT
#define __JR_COMPAT

#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

typedef unsigned long offset_t;

/* (1) MAX_FNAME is the maxium size of the filename, defined in DOS/WINDOWS */
#ifndef MAX_FNAME
#define MAX_FNAME 260
#endif
/* (1) END */

/* (2) gcc on linux seems to not understand these */
#ifndef S_IREAD
#define S_IREAD 0x00
#endif

#ifndef S_IWRITE
#define S_IWRITE 0x00
#endif
/* (2) END */

#endif
