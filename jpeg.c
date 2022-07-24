#include <stdio.h>
#include <stdlib.h>
#include "jpeg.h"
#include "log.h"

#define _JPEG_MARKER 0xff
#define _JPEG_SOI 0xd8
#define _JPEG_APP0 0xe0
#define _JPEG_APP1 0xe1
#define _JPEG_DQT 0xdb
#define _JPEG_COMMENT 0xfe

/* jpeg_marker describes the jpeg marker format */
struct jpeg_marker {
	unsigned char marker;
	unsigned char tag;
};

typedef struct jpeg_marker *jpeg_marker_ptr;

/* section_size is the JPEG section size, use SECTION_SIZE macro to calculate
size in bytes */
struct section_size {
	unsigned char b1;
	unsigned char b2;
};

typedef struct section_size *section_size_ptr;

#define SECTION_SIZE(ss) ss.b1 * 256 + ss.b2 - sizeof(struct jpeg_marker)

typedef struct {
	unsigned char tag;
	char *name;
} _tag_name;

_tag_name _tag_names[] = {
	{_JPEG_SOI, "start of image"},		{_JPEG_APP0, "app0"},		{_JPEG_APP1, "app1"},
	{_JPEG_DQT, "quantisation tables"}, {_JPEG_COMMENT, "comment"},
};
size_t _tag_names_sz = sizeof(_tag_names) / sizeof(_tag_name);

/* tag_name returns the tag name string for the tag.  If the tag is not found it
returns NULL */
char *tag_name(unsigned char tag) {
	int i;
	for (i = 0; i < _tag_names_sz; i++) {
		if (_tag_names[i].tag == tag) {
			return _tag_names[i].name;
		}
	}
	return NULL;
}

/* read_marker attempts to read the jpeg marker from the file f.  It returns
RET_ERROR on failure, or RET_OK on success.  jt will be populated with values.
*/
int read_marker(jpeg_marker_ptr jt, FILE *f) {
	char *tn;
	if (fread(jt, sizeof(*jt), 1, f) == 0) {
		perror("failed to read the marker");
		return RET_ERROR;
	}
	if (jt->marker != 0xff) {
		fprintf(stderr, "not a marker\n");
		return RET_ERROR;
	}
	tn = tag_name(jt->tag);
	if (tn == NULL) {
		tn = "unknown";
	}
	lverbose("read_marker: read marker with tag: %s\n", tn);
	return RET_OK;
}

/* read_section_size should be called immediately after read_marker.  It will
attempt to read the section size from the file f.  On success it returns RET_OK
and ss is populated with values from the file.  Use SECTION_SIZE macro to
calculate integer size of the section.  On error it returns RET_ERROR. */
int read_section_size(section_size_ptr ss, FILE *f) {
	if (fread(ss, sizeof(*ss), 1, f) == 0) {
		perror("failed to read section size");
		return RET_ERROR;
	}
	lverbose("read_section_size: [%02X, %02X], size: %d\n", ss->b1, ss->b2, SECTION_SIZE((*ss)));
	return RET_OK;
}

/* jpeg_hdr_size returns the header size in bytes */
int jpeg_hdr_size(FILE *f) {
	struct jpeg_marker jt;
	struct section_size ss;

	/* read the first tag */
	if (!read_marker(&jt, f)) {
		llog("failed to read marker 1");
		return RET_ERROR;
	}

	if (jt.tag != _JPEG_SOI) {
		llog("unexpected tag");
		return RET_ERROR;
	}

	/* read the following tag */
	if (!read_marker(&jt, f)) {
		perror("failed to read marker 2");
		return RET_ERROR;
	}

	switch (jt.tag) {
	default:
		llog("unexpected tag: %X\n", jt.tag);
		return RET_ERROR;
	case _JPEG_APP0:;
	}

	if (!read_section_size(&ss, f)) {
		return RET_ERROR;
	}

	/* skip */
	if (fseek(f, SECTION_SIZE(ss), SEEK_CUR) == -1) {
		perror("unable to seek");
		return RET_ERROR;
	}
	if (!read_marker(&jt, f)) {
		return RET_ERROR;
	}

	/* search for quantisation tables */
	while (jt.tag != _JPEG_DQT) {
		if (!read_section_size(&ss, f)) {
			return RET_ERROR;
		}
		lverbose("\tskipping section size: %d\n", SECTION_SIZE(ss));
		if (fseek(f, SECTION_SIZE(ss), SEEK_CUR) == -1) {
			perror("seek failed");
			return RET_ERROR;
		}
		if (!read_marker(&jt, f)) {
			return RET_ERROR;
		}
	}

	/* current file position - sizeof(last_marker) */
	return ftell(f) - sizeof(struct jpeg_marker);
}
