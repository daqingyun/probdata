/***************************************************************************
 * ProbData - PRofiling Optimization Based DAta Transfer Advisor
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _PROBDATA_LOG_H_
#define _PROBDATA_LOG_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include "probdata.h"
#include "probdata_error.h"

#ifndef  P_KEEP_LOG
    #define P_KEEP_LOG (0)
#endif

#ifndef  P_AS_TOOLKIT
    #define P_AS_TOOLKIT (1)
#endif

#ifndef  P_DEBUG_MODE
    #define P_DEBUG_MODE (0)
#endif

#define P_ERR   (1)
#define P_HINT  (2)
#define P_PERF  (3)

extern FILE *p_log_file;
extern pthread_mutex_t p_log_file_lock;

int  probdata_open_log(struct probdata* prob);
void probdata_close_log(void);

#define P_LOG(level, ...)                                            \
do{                                                                  \
    pthread_mutex_lock(&p_log_file_lock);                            \
    if(P_KEEP_LOG == 1 && p_log_file != NULL) {                      \
        switch (level)                                               \
        {                                                            \
            case P_ERR:                                              \
            {                                                        \
                fprintf(p_log_file, "[ERROR]  ");                    \
                break;                                               \
            }                                                        \
            case P_HINT:                                             \
            {                                                        \
                fprintf(p_log_file, "[ HINT ] ");                    \
                break;                                               \
            }                                                        \
            case P_PERF:                                             \
            {                                                        \
                fprintf(p_log_file, "[ INFO ] ");                    \
                break;                                               \
            }                                                        \
            default :                                                \
            {                                                        \
                ;                                                    \
            }                                                        \
        }                                                            \
        fprintf(p_log_file, __VA_ARGS__);                            \
        fprintf(p_log_file, " - %s:%d:%s()\n",                       \
                __FILE__, __LINE__, __FUNCTION__);                   \
        fflush(p_log_file);                                          \
    } else {                                                         \
        if (P_DEBUG_MODE == 1) {                                     \
            fprintf(stdout, "%s", "");                               \
            fflush(stdout);                                          \
        }                                                            \
    }                                                                \
    if (P_AS_TOOLKIT == 1) {                                         \
        if (level == P_PERF) {                                       \
            fprintf(stdout, __VA_ARGS__);                            \
            fprintf(stdout, "\n");                                   \
            fflush(stdout);                                          \
        }                                                            \
        if (P_DEBUG_MODE == 1) {                                     \
            switch (level)                                           \
            {                                                        \
                case P_ERR:                                          \
                {                                                    \
                    fprintf(stdout, "[ERROR]  ");                    \
                    break;                                           \
                }                                                    \
                case P_HINT:                                         \
                {                                                    \
                    fprintf(stdout, "[ HINT ] ");                    \
                    break;                                           \
                }                                                    \
                case P_PERF:                                         \
                {                                                    \
                    fprintf(stdout, "[ INFO ] ");                    \
                    break;                                           \
                }                                                    \
                default :                                            \
                { ; }                                                \
            }                                                        \
            fprintf(stdout, __VA_ARGS__);                            \
            fprintf(stdout, " - %s:%d:%s()\n",                       \
                    __FILE__, __LINE__, __FUNCTION__);               \
            fflush(stdout);                                          \
        }                                                            \
    }                                                                \
    pthread_mutex_unlock(&p_log_file_lock);                          \
}while(0)

#endif
