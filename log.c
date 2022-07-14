#include <stdarg.h>
#include <stdio.h>
#include "log.h"

int _gLog_level = LOG_LEVEL_NORMAL;

void set_log_level(int mode) { _gLog_level = mode; }

int get_log_level() { return _gLog_level; }

void flogf(const char *fmt, ...) {
    if (_gLog_level < LOG_LEVEL_VERBOSE) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void ftracef(const char *fmt, ...) {
    if (_gLog_level < LOG_LEVEL_TRACE) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}
