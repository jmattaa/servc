#include "logger.h"
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

#define _X(level, color, fd, sign, err)                                        \
    void servc_logger_##level sign                                             \
    {                                                                          \
        pthread_mutex_lock(&log_mutex);                                        \
        fprintf(fd, color "servc [" #level "]\x1b[0m: ");                      \
        va_list args;                                                          \
        va_start(args, fmt);                                                   \
        vfprintf(fd, fmt, args);                                               \
        va_end(args);                                                          \
        pthread_mutex_unlock(&log_mutex);                                      \
        err                                                                    \
    }
LOG_LEVEL_ITER(_X)
#undef _X
