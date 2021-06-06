/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <pthread.h>
#include "tpg_log.h"

FILE* t_log_file = NULL;
pthread_mutex_t t_log_file_lock;

int t_open_log(tpg_profile *prof)
{
    if (pthread_mutex_init(&t_log_file_lock, NULL) != 0) {
        t_errno = T_ERR_FILE;
        return -1;
    }

#if T_KEEP_LOG == 1
    time_t t;
    struct tm currtime;
    char log_filename[128];
    struct stat st = {0};
    int processid;
    
    if (!t_log_file) {
        if (stat("./t_log", &st) == -1) {
            mkdir("./t_log", 0700);
        } else {
            T_LOG(T_HINT, "log folder \'%s\' exists", "./t_log");
        }
        
        t = time(NULL);
        currtime = *localtime(&t);
        processid = getpid();
        
        if (prof->role == T_CLIENT) {
            sprintf(log_filename,
                    "./t_log/client-%d-%02d-%02d-%02d-%02d-%02d-%d.log",
                    currtime.tm_year + 1900,
                    currtime.tm_mon + 1,
                    currtime.tm_mday,
                    currtime.tm_hour,
                    currtime.tm_min,
                    currtime.tm_sec,
                    processid);
        } else if (prof->role == T_SERVER) {
            sprintf(log_filename,
                    "./t_log/server-%d-%02d-%02d-%02d-%02d-%02d-%d.log",
                    currtime.tm_year + 1900,
                    currtime.tm_mon + 1,
                    currtime.tm_mday,
                    currtime.tm_hour,
                    currtime.tm_min,
                    currtime.tm_sec,
                    processid);
        } else {
            t_errno = T_ERR_FILE;
            return -1;
        }
        if( (t_log_file = fopen (log_filename, "a+")) == NULL ) {
            t_errno = T_ERR_FILE;
            return -1;
        }
    }
#endif
    return 0;
}

void t_close_log(void)
{
    if (t_log_file) {
        fclose(t_log_file);
        t_log_file = NULL;
    }
    pthread_mutex_destroy(&t_log_file_lock);
}