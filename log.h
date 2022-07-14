#ifndef __JR_LOG
#define __JR_LOG

#define LOG_LEVEL_NORMAL 0
#define LOG_LEVEL_VERBOSE 1
#define LOG_LEVEL_TRACE 2

void set_log_level(int mode);
void flogf(const char *fmt, ...);
void ftracef(const char *fmt, ...);

#endif /* __JR_LOG */
