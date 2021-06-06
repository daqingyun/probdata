/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _TPG_LOG_H_
#define _TPG_LOG_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include "tpg_profile.h"

#ifndef T_KEEP_LOG
    #define T_KEEP_LOG (0)
#endif

#ifndef T_AS_TOOLKIT
    #define T_AS_TOOLKIT (0)
#endif

#ifndef T_DEBUG_MODE
    #define T_DEBUG_MODE (0)
#endif

#ifndef T_FASTPROF
    #define T_FASTPROF (0)
#endif


#define T_ERR (1)
#define T_HINT (2)
#define T_PERF (3)

extern FILE *t_log_file;
extern pthread_mutex_t t_log_file_lock;

int  t_open_log(tpg_profile* prof);
void t_close_log(void);

#define T_LOG(level, ...)                                        \
        do{                                                      \
            pthread_mutex_lock(&t_log_file_lock);                \
            if(T_KEEP_LOG == 1 && t_log_file != NULL) {          \
                switch (level)                                   \
                {                                                \
                case T_ERR:                                      \
                {                                                \
                    fprintf(t_log_file, "[ERROR]  ");            \
                    break;                                       \
                }                                                \
                case T_HINT:                                     \
                {                                                \
                    fprintf(t_log_file, "[ HINT ] ");            \
                    break;                                       \
                }                                                \
                case T_PERF:                                     \
                {                                                \
                    fprintf(t_log_file, "[ INFO ] ");            \
                    break;                                       \
                }                                                \
                default :                                        \
                {                                                \
                    ;                                            \
                }                                                \
                }                                                \
                fprintf(t_log_file, __VA_ARGS__);                \
                fprintf(t_log_file, " - %s:%d:%s()\n",           \
                __FILE__, __LINE__, __FUNCTION__);               \
                fflush(t_log_file);                              \
            } else {                                             \
                if (T_DEBUG_MODE == 1) {                         \
                    fprintf(stdout, "%s", "");                   \
                    fflush(stdout);                              \
                }                                                \
            }                                                    \
            if (T_AS_TOOLKIT == 1 || T_FASTPROF == 1) {          \
                if (level == T_PERF) {                           \
                    fprintf(stdout, __VA_ARGS__);                \
                    fprintf(stdout, "\n");                       \
                    fflush(stdout);                              \
                }                                                \
                if (T_DEBUG_MODE == 1) {                         \
                    switch (level)                               \
                    {                                            \
                    case T_ERR:                                  \
                    {                                            \
                        fprintf(stdout, "[ERROR]  ");            \
                        break;                                   \
                    }                                            \
                    case T_HINT:                                 \
                    {                                            \
                        fprintf(stdout, "[ HINT ] ");            \
                        break;                                   \
                    }                                            \
                    case T_PERF:                                 \
                    {                                            \
                        fprintf(stdout, "[ INFO ] ");            \
                        break;                                   \
                    }                                            \
                    default :                                    \
                    { ; }                                        \
                    }                                            \
                    fprintf(stdout, __VA_ARGS__);                \
                    fprintf(stdout, " - %s:%d:%s()\n",           \
                    __FILE__, __LINE__, __FUNCTION__);           \
                    fflush(stdout);                              \
                }                                                \
            }                                                    \
            pthread_mutex_unlock(&t_log_file_lock);              \
    } while(0)

#endif
