/***************************************************************************
 * FastProf: A Stochastic Approximation-based Transport Profiler
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
#include <tpg.h>
#include "fastprof_udt.h"

// int exec(tpg_profile*);
int exec(struct fastprof_udt_prof*);

int main(int argc, char** argv)
{
    struct fastprof_udt_prof* prof = new struct fastprof_udt_prof;
    prof->udt_prof = tpg_new_profile();
    
    if(tpg_prof_default(prof->udt_prof) < 0) {
        F_LOG(F_ERR, "default setting profile error %s", 
                f_errstr(f_errno));
        exit(1);
    }
    
    // TPG will start these timers
    // if (tpg_prof_start_timers(prof->udt_prof) < 0) {
    //     F_LOG(F_ERR, "program timers are started");
    //     exit(1);
    // }
    
    if (parse_cmd_line(prof->udt_prof, argc, argv) < 0) {
        F_LOG(F_PERF, "parameter parsing error: %s",
                f_errstr(f_errno));
        t_print_long_usage();
        exit(1);
    }
    
    if (exec(prof) < 0) {
        F_LOG(F_PERF, "%s", f_errstr(f_errno));
        exit(1);
    }
    
    return 0;
}

int exec(struct fastprof_udt_prof* prof)
{
    struct fastprof_udt_prof prof_max;
    memset(&prof_max, '\0', sizeof(struct fastprof_udt_prof));
    struct fastprof_ctrl ctrl;
    
    ctrl.alpha = 0.602;
    ctrl.gamma = 0.101;
    ctrl.A_val = tpg_get_fastprof_A_value(prof->udt_prof);
    ctrl.c_val = tpg_get_fastprof_c_value(prof->udt_prof);
    ctrl.a_val = tpg_get_fastprof_a_value(prof->udt_prof);
    ctrl.impeded_limit = tpg_get_fastprof_no_imp_limit(prof->udt_prof);
    ctrl.max_limit = tpg_get_fastprof_max_prof_limit(prof->udt_prof);
    ctrl.bw_bps = tpg_get_fastprof_bandwidth(prof->udt_prof);
    ctrl.duration = tpg_get_prof_time_duration(prof->udt_prof);
    memcpy(ctrl.local_ip, 
            tpg_get_local_ip(prof->udt_prof),
            strlen(tpg_get_local_ip(prof->udt_prof)));
    ctrl.local_ip[strlen(tpg_get_local_ip(prof->udt_prof))] = '\0';
    memcpy(ctrl.remote_ip,
            tpg_get_remote_ip(prof->udt_prof),
            strlen(tpg_get_remote_ip(prof->udt_prof)));
    ctrl.remote_ip[strlen(tpg_get_remote_ip(prof->udt_prof))] = '\0';
    ctrl.mss = tpg_get_udt_mss(prof->udt_prof);
    ctrl.pgr = tpg_get_fastprof_perf_gain_ratio(prof->udt_prof);
    ctrl.rtt_ms = tpg_get_fastprof_rtt_delay(prof->udt_prof);
    ctrl.udt_port = tpg_get_ctrl_listen_port(prof->udt_prof);
    
    int err_cnt = 0;
    
    switch (tpg_get_role(prof->udt_prof))
    {
        case T_SERVER:
        {
            if (f_open_log() < 0) {
                return -1;
            }
            for(;;) {
                if (tpg_server_run(prof->udt_prof, -1, -1) < 0) {
                    tpg_server_clean_up(prof->udt_prof);
                    tpg_server_reset(prof->udt_prof);
                    F_LOG(F_PERF,
                            "%s - %d\n",
                            f_errstr(f_errno),
                            f_errno);
                    err_cnt++;
                    if (err_cnt > F_MAX_ERROR_NUM){
                        F_LOG(F_PERF,
                                "[abort]: too many errors - [%d]\n", err_cnt);
                        break;
                    }
                } else {
                    err_cnt = 0;
                }
            }
            f_close_log();
            break;
        }

	case T_CLIENT:
        {
            if (f_open_log() < 0) {
                return -1;
            }
            
            if (f_udt_client_run(prof, &prof_max, &ctrl) < 0) {
                F_LOG(F_PERF, "%s\n",
                        f_errstr(f_errno));
            }
            f_close_log();
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
