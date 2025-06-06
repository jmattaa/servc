#ifndef SERVC_LOGGER_H
#define SERVC_LOGGER_H

#define LOG_LEVEL_ITER(_X, ...)                                                \
    _X(info, "\x1b[34m", stdout, (const char *fmt, ...), {})                   \
    _X(error, "\x1b[31m", stderr, (const char *fmt, ...), {})                  \
    _X(fatal, "\x1b[31m", stderr, (int errcode, const char *fmt, ...),         \
       exit(errcode);)

#define _X(level, color, fd, sign, ...) void servc_logger_##level sign;
LOG_LEVEL_ITER(_X)
#undef _X

#endif
