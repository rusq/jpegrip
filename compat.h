#ifndef __JR_COMPAT
#define __JR_COMPAT

#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

/* MAX_FNAME is the maxium size of the filename, defined in DOS/WINDOWS */
#ifndef MAX_FNAME
#define MAX_FNAME 260
#endif

#endif
