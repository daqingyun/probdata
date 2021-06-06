/***************************************************************************
 * FastProf: A Stochastic Approximation-based Transport Profiler
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <pthread.h>
#include "fastprof.h"
#include "fastprof_log.h"

FILE* f_log_file = NULL;
pthread_mutex_t f_log_file_lock;

int f_open_log()
{
    if (pthread_mutex_init(&f_log_file_lock, NULL) != 0) {
        f_errno = F_ERR_LOCK;
        return -1;
    }
#if F_KEEP_LOG == 1
    time_t t;
    struct tm currtime;
    char log_filename[128];
    struct stat st = {0};
    int processid;
    
    if (!f_log_file) {
        if (stat("./f_log", &st) == -1) {
            mkdir("./f_log", 0700);
        } else {
            F_LOG(F_HINT, "log folder \'%s\' exists", "./f_log");
        }
        
        t = time(NULL);
        currtime = *localtime(&t);
        processid = getpid();
        
        sprintf(log_filename, "./f_log/fp-%d-%02d-%02d-%02d-%02d-%02d-%d.log",
                currtime.tm_year + 1900, currtime.tm_mon + 1, currtime.tm_mday,
                currtime.tm_hour, currtime.tm_min, currtime.tm_sec, processid);
        
        if( (f_log_file = fopen (log_filename, "a+")) == NULL ) {
            f_errno = F_ERR_FILE;
            return -1;
        }
    }
#endif
    return 0;
}

void f_close_log(void)
{
    if (f_log_file) {
        fclose(f_log_file);
        f_log_file = NULL;
    }
    pthread_mutex_destroy(&f_log_file_lock);
}