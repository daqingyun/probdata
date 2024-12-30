/***************************************************************************
 * ProbData - PRofiling Optimization Based DAta Transfer Advisor
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <pthread.h>
#include "probdata_log.h"

FILE* p_log_file = NULL;
pthread_mutex_t p_log_file_lock;

int probdata_open_log(struct probdata* prob)
{
    if (pthread_mutex_init(&p_log_file_lock, NULL) != 0) {
        p_errno = P_ERR_FILE;
        return -1;
    }
#if P_KEEP_LOG == 1
    time_t t;
    struct tm currtime;
    char log_filename[128];
    struct stat st = {0};
    int processid;
    
    if (!p_log_file) {
        if (stat("./probdata_log", &st) == -1) {
            mkdir("./probdata_log", 0700);
        } else {
            P_LOG(P_HINT, "probdata log folder \'%s\' exists", "./probdata_log");
        }
        
        t = time(NULL);
        currtime = *localtime(&t);
        processid = getpid();
        
        if (prob->role == P_CLIENT) {
            sprintf(log_filename, "./probdata_log/client-%d-%02d-%02d-%02d-%02d-%02d-%d.log",
                    currtime.tm_year + 1900,
                    currtime.tm_mon + 1,
                    currtime.tm_mday,
                    currtime.tm_hour,
                    currtime.tm_min,
                    currtime.tm_sec,
                    processid);
        } else if (prob->role == P_SERVER) {
            sprintf(log_filename, "./probdata_log/server-%d-%02d-%02d-%02d-%02d-%02d-%d.log",
                    currtime.tm_year + 1900,
                    currtime.tm_mon + 1,
                    currtime.tm_mday,
                    currtime.tm_hour,
                    currtime.tm_min,
                    currtime.tm_sec,
                    processid);
        } else {
            p_errno = P_ERR_FILE;
            return -1;
        }
        
        if( (p_log_file = fopen (log_filename, "a+")) == NULL ) {
            p_errno = P_ERR_FILE;
            return -1;
        }
    }
#endif
    
    return 0;
}

void probdata_close_log(void)
{
    if (p_log_file) {
        fclose(p_log_file);
        p_log_file = NULL;
    }
    pthread_mutex_destroy(&p_log_file_lock);
}