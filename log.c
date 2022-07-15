/*
 * JPEG the Ripper
 * Programme for ripping out JPEG files out of a mess
 *
 * Copyright 2005 by Rustam Gilyazov (github: @rusq)
 * May be distributed under the GNU General Public License
 */
#include <stdarg.h>
#include <stdio.h>
#include "log.h"

int _gLog_level = LOG_LEVEL_NORMAL;

void set_log_level(int mode) { _gLog_level = mode; }

int get_log_level() { return _gLog_level; }

void llog(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void lverbose(const char *fmt, ...) {
    va_list args;

    if (_gLog_level < LOG_LEVEL_VERBOSE) {
        return;
    }
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void ltrace(const char *fmt, ...) {
    va_list args;

    if (_gLog_level < LOG_LEVEL_TRACE) {
        return;
    }
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}
