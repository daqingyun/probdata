/***************************************************************************
 * FastProf: A Stochastic Approximation-based Transport Profiler
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _FASTPROF_LOG_H_
#define _FASTPROF_LOG_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include "fastprof.h"
#include "fastprof_error.h"

#ifndef  F_KEEP_LOG
    #define F_KEEP_LOG (0)
#endif

#ifndef  F_AS_TOOLKIT
    #define F_AS_TOOLKIT (1)
#endif

#ifndef  F_DEBUG_MODE
    #define F_DEBUG_MODE (0)
#endif

#define F_ERR   (1)
#define F_HINT  (2)
#define F_PERF  (3)

extern FILE *f_log_file;
extern pthread_mutex_t f_log_file_lock;

int  f_open_log(void);
void f_close_log(void);

#define F_LOG(level, ...)                                 \
do{                                                       \
    pthread_mutex_lock(&f_log_file_lock);                 \
    if(F_KEEP_LOG == 1 && f_log_file != NULL) {           \
        switch (level)                                    \
        {                                                 \
            case F_ERR:                                   \
            {                                             \
                fprintf(f_log_file, "[ERROR]  ");         \
                break;                                    \
            }                                             \
            case F_HINT:                                  \
            {                                             \
                fprintf(f_log_file, "[ HINT ] ");         \
                break;                                    \
            }                                             \
            case F_PERF:                                  \
            {                                             \
                fprintf(f_log_file, "[ INFO ] ");         \
                break;                                    \
            }                                             \
            default :                                     \
            {                                             \
                ;                                         \
            }                                             \
        }                                                 \
        fprintf(f_log_file, __VA_ARGS__);                 \
        fprintf(f_log_file, " - %s:%d:%s()\n",            \
                __FILE__, __LINE__, __FUNCTION__);        \
        fflush(f_log_file);                               \
    } else {                                              \
        if (F_DEBUG_MODE == 1) {                          \
            fprintf(stdout, "%s", "");                    \
            fflush(stdout);                               \
        }                                                 \
    }                                                     \
    if (F_AS_TOOLKIT == 1) {                              \
        if (level == F_PERF) {                            \
            fprintf(stdout, __VA_ARGS__);                 \
            fprintf(stdout, "\n");                        \
            fflush(stdout);                               \
        }                                                 \
        if (F_DEBUG_MODE == 1) {                          \
            switch (level)                                \
            {                                             \
                case F_ERR:                               \
                {                                         \
                    fprintf(stdout, "[ERROR]  ");         \
                    break;                                \
                }                                         \
                case F_HINT:                              \
                {                                         \
                    fprintf(stdout, "[ HINT ] ");         \
                    break;                                \
                }                                         \
                case F_PERF:                              \
                {                                         \
                    fprintf(stdout, "[ INFO ] ");         \
                    break;                                \
                }                                         \
                default :                                 \
                { ; }                                     \
            }                                             \
            fprintf(stdout, __VA_ARGS__);                 \
            fprintf(stdout, " - %s:%d:%s()\n",            \
                    __FILE__, __LINE__, __FUNCTION__);    \
            fflush(stdout);                               \
        }                                                 \
    }                                                     \
    pthread_mutex_unlock(&f_log_file_lock);               \
}while(0)

#endif
