/*
 * Minimal C89 test assertion utilities.
 * Include this header in exactly one .c file per test binary.
 */
#ifndef TEST_ASSERT_H
#define TEST_ASSERT_H

#include <stdio.h>

static int _t_pass = 0;
static int _t_fail = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            printf("  PASS  %s\n", (msg)); \
            _t_pass++; \
        } else { \
            printf("  FAIL  %s  [line %d]\n", (msg), __LINE__); \
            _t_fail++; \
        } \
    } while (0)

static int test_summary(const char *suite) {
    printf("\n%s: %d passed, %d failed\n", suite, _t_pass, _t_fail);
    return (_t_fail > 0) ? 1 : 0;
}

#endif /* TEST_ASSERT_H */
