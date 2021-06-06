/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include "tpg_log.h"
#include "tpg_client.h"
#include "tpg_server.h"

int exec(tpg_profile*);

int main(int argc, char** argv)
{
    tpg_profile* prof;
    prof = new tpg_profile;

    if(tpg_prof_default(prof) < 0) {
        T_LOG(T_ERR, "default setting profile error %s", t_errstr(t_errno));
        exit(1);
    }
    
    if (tpg_prof_start_timers(prof) < 0) {
        T_LOG(T_ERR, "Program timers are started");
        exit(1);
    }
    
    if (parse_cmd_line(prof, argc, argv) < 0) {
        T_LOG(T_PERF, "parameter parsing error: %s", t_errstr(t_errno));
        t_print_long_usage();
        exit(1);
    }
    
    if (exec(prof) < 0) {
        T_LOG(T_PERF, "%s", t_errstr(t_errno));
        exit(1);
    }
    
    return 0;
}

int exec(tpg_profile* prof)
{
    int err_cnt = 0;
    
    switch(prof->role)
    {
	case T_SERVER:
        {
            if (t_open_log(prof) < 0) {
                return -1;
            }
            
            for(;;) {
                if (tpg_server_run(prof, -1, -1) < 0) {
                    tpg_server_clean_up(prof);
                    tpg_server_reset(prof);
                    
                    T_LOG(T_PERF, "%s - %d\n", t_errstr(t_errno), t_errno);
                    err_cnt++;
                    if (err_cnt > T_MAX_ERROR_NUM) {
                        T_LOG(T_PERF, "[abort]: too many errors - [%d]\n", err_cnt);
                        break;
                    }
                } else {
                    err_cnt = 0;
                }
            }
            t_close_log();
            break;
        }

	case T_CLIENT:
        {
            if (t_open_log(prof) < 0) {
                return -1;
            }
            if (tpg_client_run(prof) < 0) {
                T_LOG(T_PERF, "%s\n", t_errstr(t_errno));
            }
            t_close_log();
            break;
        }

	default:
        {
            t_print_short_usage();
            break;
        }
    }
    return 0;
}
