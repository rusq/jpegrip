/*
 * Unit tests for fmt_string() and format_name() in jpegrip.c
 * C89 compliant. Uses only stdio.h / string.h (ANSI C89).
 */
#include <stdio.h>
#include <string.h>
#include "assert.h"

/* Internal functions under test */
extern int fmt_string(char *buf, int buf_sz, const char *prefix,
                      int digits, const char *ext);
extern int format_name(char *output, int output_sz, const char *fmt,
                       int sequence);

/* ---------- fmt_string tests ---------- */

static void test_fmt_string_basic(void) {
    char buf[64];
    char name[64];
    int r = fmt_string(buf, sizeof(buf), "jpg", 8, "jpg");
    TEST_ASSERT(r > 0, "fmt_string: returns > 0 on success");
    /* apply the format string to sequence 0 and check the result */
    snprintf(name, sizeof(name), buf, 0L);
    TEST_ASSERT(strcmp(name, "jpg00000000.jpg") == 0,
                "fmt_string: sequence 0 yields 'jpg00000000.jpg'");
}

static void test_fmt_string_sequence_42(void) {
    char buf[64];
    char name[64];
    fmt_string(buf, sizeof(buf), "jpg", 8, "jpg");
    snprintf(name, sizeof(name), buf, 42L);
    TEST_ASSERT(strcmp(name, "jpg00000042.jpg") == 0,
                "fmt_string: sequence 42 yields 'jpg00000042.jpg'");
}

static void test_fmt_string_different_prefix(void) {
    char buf[64];
    char name[64];
    fmt_string(buf, sizeof(buf), "out", 4, "jpeg");
    snprintf(name, sizeof(name), buf, 7L);
    TEST_ASSERT(strcmp(name, "out0007.jpeg") == 0,
                "fmt_string: prefix='out' digits=4 ext='jpeg' seq=7");
}

static void test_fmt_string_zero_buf(void) {
    char buf[64];
    int r = fmt_string(buf, 0, "jpg", 8, "jpg");
    TEST_ASSERT(r < 0, "fmt_string: zero buf_sz returns < 0");
}

/* ---------- format_name tests ---------- */

static void test_format_name_zero(void) {
    char fmt[64];
    char name[64];
    fmt_string(fmt, sizeof(fmt), "jpg", 8, "jpg");
    format_name(name, sizeof(name), fmt, 0);
    TEST_ASSERT(strcmp(name, "jpg00000000.jpg") == 0,
                "format_name: sequence 0");
}

static void test_format_name_large(void) {
    char fmt[64];
    char name[64];
    fmt_string(fmt, sizeof(fmt), "jpg", 8, "jpg");
    format_name(name, sizeof(name), fmt, 99999999);
    TEST_ASSERT(strcmp(name, "jpg99999999.jpg") == 0,
                "format_name: sequence 99999999 fills all 8 digits");
}

static void test_format_name_sequential(void) {
    char fmt[64];
    char a[64], b[64];
    fmt_string(fmt, sizeof(fmt), "jpg", 8, "jpg");
    format_name(a, sizeof(a), fmt, 1);
    format_name(b, sizeof(b), fmt, 2);
    TEST_ASSERT(strcmp(a, b) != 0, "format_name: sequential names differ");
}

/* ---------- main ---------- */

int main(void) {
    printf("=== test_fmt_string ===\n");

    test_fmt_string_basic();
    test_fmt_string_sequence_42();
    test_fmt_string_different_prefix();
    test_fmt_string_zero_buf();

    printf("\n=== test_format_name ===\n");

    test_format_name_zero();
    test_format_name_large();
    test_format_name_sequential();

    return test_summary("fmt");
}
