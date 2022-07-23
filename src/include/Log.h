#ifndef TELODENDRIA_LOG_H
#define TELODENDRIA_LOG_H

#include <stdio.h>
#include <stddef.h>

typedef enum LogLevel {
    LOG_ERROR,
    LOG_WARNING,
    LOG_TASK,
    LOG_MESSAGE,
    LOG_DEBUG
} LogLevel;

typedef enum LogFlag {
	LOG_FLAG_COLOR = (1 << 0)
} LogFlag;

typedef struct LogConfig LogConfig;

extern LogConfig *
LogConfigCreate(void);

extern void
LogConfigFree(LogConfig *config);

extern void
LogConfigLevelSet(LogConfig *config, LogLevel level);

extern LogLevel
LogConfigLevelGet(LogConfig *config);

extern void
LogConfigIndentSet(LogConfig *config, size_t indent);

extern size_t
LogConfigIndentGet(LogConfig *config);

extern void
LogConfigIndent(LogConfig *config);

extern void
LogConfigUnindent(LogConfig *config);

extern void
LogConfigOutputSet(LogConfig *config, FILE *out);

extern void
LogConfigFlagSet(LogConfig *config, int flags);

extern void
LogConfigFlagClear(LogConfig *config, int flags);

extern int
LogConfigFlagGet(LogConfig *config, int flags);

extern void
LogConfigTimeStampFormatSet(LogConfig *config, char *tsFmt);

extern void
Log(LogConfig *config, LogLevel level, const char *msg, ...);

#endif
