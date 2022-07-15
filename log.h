/*
 * JPEG the Ripper
 * Programme for ripping out JPEG files out of a mess
 *
 * Copyright 2005 by Rustam Gilyazov (github: @rusq)
 * May be distributed under the GNU General Public License
 */
#ifndef __JR_LOG
#define __JR_LOG

#define LOG_LEVEL_NORMAL 0
#define LOG_LEVEL_VERBOSE 1
#define LOG_LEVEL_TRACE 2

void set_log_level(int mode);

void llog(const char *fmt, ...);
void lverbose(const char *fmt, ...);
void ltrace(const char *fmt, ...);

#endif /* __JR_LOG */
