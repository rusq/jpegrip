/*
 * Unit tests for search_buf() and search_file() in jpegrip.c
 * C89 compliant. Uses only stdio.h / stdlib.h (ANSI C89).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "assert.h"

/* Internal functions under test - declared here to avoid polluting jpegrip.h */
extern int  search_buf(const unsigned char *buf, size_t buf_sz,
                       const unsigned char *seq, size_t seq_sz);
extern long search_file(FILE *hFile, long start_pos,
                        const unsigned char *seq, size_t seq_sz);

/* ---------- helpers ---------- */

static FILE *make_tmpfile(const unsigned char *data, size_t sz) {
    FILE *f = tmpfile();
    if (f == NULL) {
        perror("tmpfile");
        return NULL;
    }
    fwrite(data, sizeof(unsigned char), sz, f);
    rewind(f);
    return f;
}

/* ---------- search_buf tests ---------- */

static void test_search_buf_found_at_zero(void) {
    const unsigned char buf[] = {0xff, 0xd8, 0xff, 0xe0, 0xAA, 0xBB};
    const unsigned char seq[] = {0xff, 0xd8, 0xff, 0xe0};
    TEST_ASSERT(search_buf(buf, sizeof(buf), seq, sizeof(seq)) == 0,
                "search_buf: sequence at offset 0");
}

static void test_search_buf_found_at_offset(void) {
    const unsigned char buf[] = {0x00, 0x11, 0xff, 0xd8, 0xff, 0xe0};
    const unsigned char seq[] = {0xff, 0xd8, 0xff, 0xe0};
    TEST_ASSERT(search_buf(buf, sizeof(buf), seq, sizeof(seq)) == 2,
                "search_buf: sequence at offset 2");
}

static void test_search_buf_not_found(void) {
    const unsigned char buf[] = {0x00, 0x11, 0x22, 0x33};
    const unsigned char seq[] = {0xff, 0xd8};
    TEST_ASSERT(search_buf(buf, sizeof(buf), seq, sizeof(seq)) == -1,
                "search_buf: sequence not found returns -1");
}

static void test_search_buf_smaller_than_seq(void) {
    const unsigned char buf[] = {0xff};
    const unsigned char seq[] = {0xff, 0xd8};
    TEST_ASSERT(search_buf(buf, sizeof(buf), seq, sizeof(seq)) == -1,
                "search_buf: buffer smaller than sequence returns -1");
}

static void test_search_buf_at_end(void) {
    const unsigned char buf[] = {0x00, 0x11, 0xff, 0xd9};
    const unsigned char seq[] = {0xff, 0xd9};
    TEST_ASSERT(search_buf(buf, sizeof(buf), seq, sizeof(seq)) == 2,
                "search_buf: sequence at last valid position");
}

static void test_search_buf_single_byte(void) {
    const unsigned char buf[] = {0xAA, 0xBB, 0xCC};
    const unsigned char seq[] = {0xBB};
    TEST_ASSERT(search_buf(buf, sizeof(buf), seq, sizeof(seq)) == 1,
                "search_buf: single-byte sequence found at offset 1");
}

/* ---------- search_file tests ---------- */

static void test_search_file_found_at_start(void) {
    const unsigned char data[] = {0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0xAA};
    const unsigned char seq[]  = {0xff, 0xd8, 0xff, 0xe0};
    FILE *f = make_tmpfile(data, sizeof(data));
    long result;
    if (f == NULL) { _t_fail++; return; }
    result = search_file(f, 0, seq, sizeof(seq));
    TEST_ASSERT(result == 0, "search_file: found at start of file");
    fclose(f);
}

static void test_search_file_found_at_offset(void) {
    unsigned char data[16];
    const unsigned char seq[] = {0xff, 0xd8};
    FILE *f;
    long result;
    memset(data, 0x00, sizeof(data));
    data[10] = 0xff;
    data[11] = 0xd8;
    f = make_tmpfile(data, sizeof(data));
    if (f == NULL) { _t_fail++; return; }
    result = search_file(f, 0, seq, sizeof(seq));
    TEST_ASSERT(result == 10, "search_file: found at byte offset 10");
    fclose(f);
}

static void test_search_file_not_found(void) {
    const unsigned char data[] = {0x00, 0x11, 0x22, 0x33, 0x44};
    const unsigned char seq[]  = {0xff, 0xd8};
    FILE *f = make_tmpfile(data, sizeof(data));
    long result;
    if (f == NULL) { _t_fail++; return; }
    result = search_file(f, 0, seq, sizeof(seq));
    TEST_ASSERT(result == EOF, "search_file: returns EOF when not found");
    fclose(f);
}

static void test_search_file_empty(void) {
    const unsigned char seq[] = {0xff, 0xd8};
    FILE *f = make_tmpfile(NULL, 0);
    long result;
    if (f == NULL) { _t_fail++; return; }
    result = search_file(f, 0, seq, sizeof(seq));
    TEST_ASSERT(result == EOF, "search_file: returns EOF on empty file");
    fclose(f);
}

static void test_search_file_start_pos(void) {
    /* sequence appears twice; start_pos skips the first occurrence */
    const unsigned char data[] = {0xff, 0xd9, 0x00, 0x00, 0xff, 0xd9};
    const unsigned char seq[]  = {0xff, 0xd9};
    FILE *f = make_tmpfile(data, sizeof(data));
    long result;
    if (f == NULL) { _t_fail++; return; }
    result = search_file(f, 2, seq, sizeof(seq));
    TEST_ASSERT(result == 4, "search_file: start_pos skips earlier occurrence");
    fclose(f);
}

/*
 * Buffer-boundary test: sequence straddles the 16384-byte read buffer edge.
 * Writes 16383 zero bytes, then the target sequence, then checks it is found.
 * This verifies the seq_sz-byte rewind overlap in search_file().
 */
static void test_search_file_buffer_boundary(void) {
    const int BEFORE = 16383; /* BUF_SIZE - 1 */
    const unsigned char seq[] = {0xff, 0xd8, 0xff, 0xe0};
    FILE *f = tmpfile();
    long result;
    int i;
    if (f == NULL) { _t_fail++; return; }
    for (i = 0; i < BEFORE; i++) {
        fputc(0x00, f);
    }
    fwrite(seq, sizeof(unsigned char), sizeof(seq), f);
    rewind(f);
    result = search_file(f, 0, seq, sizeof(seq));
    TEST_ASSERT(result == BEFORE,
                "search_file: sequence spanning buffer boundary is found");
    fclose(f);
}

/* ---------- main ---------- */

int main(void) {
    printf("=== test_search_buf ===\n");

    test_search_buf_found_at_zero();
    test_search_buf_found_at_offset();
    test_search_buf_not_found();
    test_search_buf_smaller_than_seq();
    test_search_buf_at_end();
    test_search_buf_single_byte();

    printf("\n=== test_search_file ===\n");

    test_search_file_found_at_start();
    test_search_file_found_at_offset();
    test_search_file_not_found();
    test_search_file_empty();
    test_search_file_start_pos();
    test_search_file_buffer_boundary();

    return test_summary("search");
}
