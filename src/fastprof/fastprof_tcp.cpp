/***************************************************************************
 * FastProf: A Stochastic Approximation-based Transport Profiler
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <ctime>
#include <sysexits.h>
#include <stdint.h>
/* #include "../tpg/tpg_client.h"
#include "../tpg/tpg_log.h"
#include "../tpg/tpg_profile.h" */
#include <tpg_client.h>
#include <tpg_log.h>
#include <tpg_profile.h>
#include "fastprof_tcp.h"
#include "fastprof_error.h"

struct fastprof_tcp_prof* f_tcp_prof_new()
{
    struct fastprof_tcp_prof *prof = new struct fastprof_tcp_prof;
    prof->tcp_prof = iperf_new_test();
    if (!prof) {
        return NULL;
    } else {
        prof->perf_Mbps = -1.0;
        prof->total_sent = 0;
        return prof;
    }
}

int f_tcp_prof_init(struct fastprof_tcp_prof* prof, struct fastprof_ctrl * ctrl)
{
    iperf_set_verbose(prof->tcp_prof, 0);
    iperf_set_test_role(prof->tcp_prof, (ctrl->role == F_CLIENT) ? 'c' : 's');
    // iperf_set_test_bind_address(p_i_test->i_test, p_i_ctrl->local_ip);
    iperf_set_test_server_hostname(prof->tcp_prof, ctrl->remote_ip);
    iperf_set_test_server_port(prof->tcp_prof, ctrl->tcp_port);
    prof->tcp_prof->settings->mss = ctrl->mss - 40;
    iperf_set_test_reverse(prof->tcp_prof, 0);
    iperf_set_test_omit(prof->tcp_prof, 1);
    iperf_set_test_duration(prof->tcp_prof, ctrl->duration);
    iperf_set_test_reporter_interval(prof->tcp_prof, 5);
    iperf_set_test_stats_interval(prof->tcp_prof, 5);
    iperf_set_test_json_output(prof->tcp_prof, 0);

    // now make sure libiperf3 will not output messy info
    prof->tcp_prof->logfile = strdup("/dev/null");
    if (prof->tcp_prof->logfile) {
        prof->tcp_prof->outfile = fopen(prof->tcp_prof->logfile, "a+");
        if (prof->tcp_prof->outfile == NULL) {
            prof->tcp_prof->outfile = stdout;
        }
    }
    
    return 0;
}

int f_tcp_prof_reset(struct fastprof_tcp_prof* prof, struct fastprof_ctrl* ctrl)
{
    iperf_test_reset(prof->tcp_prof);
    if (iperf_defaults(prof->tcp_prof) ) {
        f_errno = F_ERR_IPERF3;
        return -1;
    }
    if (f_tcp_prof_init(prof, ctrl) < 0) {
        return -1;
    }
    return 0;
}

int f_tcp_prof_delete(struct fastprof_tcp_prof* prof)
{
    iperf_free_test(prof->tcp_prof);
    return 0;
}


int f_tcp_prof_default(struct fastprof_tcp_prof* prof)
{
    if(iperf_defaults(prof->tcp_prof) < 0) {
        f_errno = F_ERR_IPERF3;
        return -1;
    } else {
        return 0;
    }
}

int f_tcp_theta_to_idx(double theta_val, int scale, int min_val, int max_val)
{
    int idx_val = -1;
    idx_val = (int)(rounding(theta_val * scale));
    idx_val = (idx_val <= min_val) ? (min_val) : idx_val;
    idx_val = (idx_val >= max_val) ? (max_val) : idx_val;
    return idx_val;
}

double f_tcp_theta_to_val(double theta_min, double theta_max, double val_min, double val_max, double theta)
{
    return (theta - theta_min) * (val_max - val_min) / (theta_max - theta_min) + val_min;
}

int f_tcp_get_prof_name(char* filename, int no_imp_limit, int max_prof_limit, int perf_gain_ratio_int, const char* enable_disable)
{
    time_t t;
    struct tm currtime;
    struct stat st = {0};
    int processid;
    
    // to hold all profiles
    if (stat("./tcp_profile", &st) == -1) {
        mkdir("./tcp_profile", 0700);
    } else {
        F_LOG(F_HINT, "fastprof profile folder exists");
    }
    
    t = time(NULL);
    currtime = *localtime(&t);
    processid = getpid();
    
    sprintf(filename, "./tcp_profile/%d%02d%02d_%02d%02d%02d_%d_%d-%d-%d-%s.fpp",
            currtime.tm_year + 1900, currtime.tm_mon + 1, currtime.tm_mday,
            currtime.tm_hour, currtime.tm_min, currtime.tm_sec,
            processid,
            no_imp_limit,
            max_prof_limit,
            perf_gain_ratio_int,
            enable_disable);
    
    return 0;
}

int f_tcp_client_run(struct fastprof_tcp_prof* prof, struct fastprof_tcp_prof* prof_max, struct fastprof_ctrl* ctrl)
{
    struct mem_record record;
    
    double alpha = ctrl->alpha;
    double gamma = ctrl->gamma;
    double A = ctrl->A_val;
    double a = ctrl->a_val;
    double a_scale = ctrl->a_scale;
    double c = ctrl->c_val;
    double c_scale = ctrl->c_scale;
    
    int buf_size_min = F_BUF_MIN;
    int buf_size_max = (ctrl->host_max_buf < F_BUF_MAX) ? ctrl->host_max_buf : F_BUF_MAX;
    int buf_size_min_safeguard = buf_size_min;
    int buf_size_max_safeguard = buf_size_max;
    int buf_unit = 1;
    double buf_theta_min = F_ITERATIVE_MIN;
    double buf_theta_max = F_ITERATIVE_MAX;
    
    int stream_num_min = F_STREAM_NUM_MIN;
    int stream_num_max = F_STREAM_NUM_MAX;
    int stream_unit = 1;
    double stream_theta_min = F_ITERATIVE_MIN;
    double stream_theta_max = F_ITERATIVE_MAX;
    
    double buf_size = -1.0;
    double stream_num = -1.0;
    
    int no_imp_limit = 0;
    int max_prof_limit = 0;
    double perf_gain_ratio = 0.0;
    double BDP_MB = -1.0;
    double BDP_B = -1.0;
    double BW_Gbps = ctrl->bw_bps / F_GB;
    double BW_Bps = ctrl->bw_bps / 8.0;
    double SRTT_ms = ctrl->rtt_ms;
    
    double theta[2] = {0.0, 0.0};
    double theta_plus[2] = {0.0, 0.0};
    double theta_minus[2] = {0.0, 0.0};
    double delta[2] = {1.0, 1.0};
    double y_plus_Gbps = 0.0;
    double y_minus_Gbps = 0.0;
    double y_star_Gbps = 0.0;
    double max_theta[2] = {theta[0], theta[1]};
    double max_perf_Gbps = 0.0;
    double ak = 0.0;
    double ck = 0.0;
    double ck_adjust_plus[2] = {0.0, 0.0};
    double ck_adjust_minus[2] = {0.0, 0.0};
    double ghat[2] = {0.0, 0.0};
    int k = 0;
    
    int profile_num = 0;
    int no_imp_counter = 0;
    int repeat_number = 0;
    int flag = -1;
    
    std::vector<fastprof_tcp_trace *> visited_list;
    visited_list.clear();
    fastprof_tcp_trace * visited_point = NULL;
    FILE* visited_list_fp = NULL;
    char visited_list_filename[128];

    if (f_open_log() < 0) {
        return -1;
    }

    srand(time(NULL));
    
    if (ctrl->impeded_limit < 0) {
        no_imp_limit = F_IMPEDED_LIMIT;
    } else {
        no_imp_limit = ctrl->impeded_limit;
    }
    if (ctrl->max_limit < 0) {
        max_prof_limit = 2 * no_imp_limit;
    } else{
        max_prof_limit = ctrl->max_limit;
    }
    if (ctrl->pgr < 0.0) {
        perf_gain_ratio = F_DEFAULT_PGR;
    } else {
        perf_gain_ratio = ctrl->pgr;
    }
    
    
    F_LOG(F_PERF, "            alpha: %.3lf", alpha);
    F_LOG(F_PERF, "            gamma: %.3lf", gamma);
    F_LOG(F_PERF, "     Impeded (-L): %d", no_imp_limit);
    F_LOG(F_PERF, "     Maximal (-N): %d", max_prof_limit);
    F_LOG(F_PERF, "         PGR (-C): %.3lf", perf_gain_ratio);
    F_LOG(F_PERF, "          BW (-y): %.3lf bps", ctrl->bw_bps);
    F_LOG(F_PERF, "         RTT (-T): %.3lf ms", ctrl->rtt_ms);
    F_LOG(F_PERF, "    Duration (-d): %d sec", ctrl->duration);
    F_LOG(F_PERF, " Buffer safeguard: %d %d", buf_size_min_safeguard, buf_size_max_safeguard);
    
    theta[0] = f_rand((int)(buf_theta_min) + 1, (int)(buf_theta_max )- 1);
    theta[1] = f_rand((int)(stream_theta_min) + 1, (int)(stream_theta_max) - 1);
    
    while (true) {
        F_LOG(F_PERF, "***         Upper: %d of %d", profile_num, max_prof_limit);
        F_LOG(F_PERF, "***       Impeded: %d of %d", no_imp_counter, no_imp_limit);
        F_LOG(F_PERF, "***       MaxPerf: %.3lf Gb/s", max_perf_Gbps);
        
        flag = -1;        
        if (profile_num >= max_prof_limit) {
            F_LOG(F_HINT, "profile#/max#: %d/%d", profile_num, max_prof_limit);
            flag = 1;
            break;
        }
        if (no_imp_counter >= no_imp_limit / 2) {
            a *= a_scale;
            F_LOG(F_PERF, "          a scale: %.3lf %.3lf", a_scale, a);
            c *= c_scale;
            F_LOG(F_PERF, "          c scale: %.3lf %.3lf", c_scale, c);
        }
        if (no_imp_counter >= no_imp_limit) {
            F_LOG(F_HINT, "impeded#/max#: %d/%d", no_imp_counter, no_imp_limit);
            flag = 2;
            break;
        }
        if ( (max_perf_Gbps / BW_Gbps) >= perf_gain_ratio ) {
            F_LOG(F_HINT, "max_perf/BW: %.3lf/%.3lf", max_perf_Gbps, BW_Gbps);
            flag = 3;
            break;
        }
        
        BDP_B = -1.0;
        if (ctrl->bw_bps < 0) {
            sleep(5);
        } else {
            BW_Bps = ctrl->bw_bps / 8.0;
            F_LOG(F_HINT, "BW is given as %.3lfBps", BW_Bps);
            
            if ((SRTT_ms = f_estimate_rtt(ctrl->remote_ip)) < 0) {
                F_LOG(F_HINT, "Cannot ping...");
            } else {
                F_LOG(F_HINT, "SRTT: %.3lf ms", SRTT_ms);
                if (ctrl->rtt_ms < 0) {
                    ctrl->rtt_ms = SRTT_ms;
                    F_LOG(F_HINT, "use SRTT: %.3lf ms", ctrl->rtt_ms);
                } else {
                    F_LOG(F_HINT, "given RTT: %.3lf ms", ctrl->rtt_ms);
                    ctrl->rtt_ms = ctrl->rtt_ms * 0.125 + SRTT_ms * 0.875;
                    F_LOG(F_HINT, "update SRTT: %.3lf ms", ctrl->rtt_ms);
                }
                F_LOG(F_PERF, "***          SRTT: %.3lf ms", SRTT_ms);
            }

            if (ctrl->rtt_ms < 0) {
                BDP_B = -1.0;
                F_LOG(F_PERF, "    Cannot figure out BDP");
            } else {
                BDP_B = ctrl->rtt_ms * 0.001 * BW_Bps;
                BDP_MB = BDP_B / F_MB;
                F_LOG(F_PERF, "***           BDP: %.3lf MB", BDP_MB);
            }
        }
        
        if (BDP_B < 0) {
            ;
        } else {
            buf_size_min = f_round(0.75 * BDP_B / buf_unit);
            buf_size_max = f_round(4.00 * BDP_B / buf_unit);

            buf_size_min = (buf_size_min < buf_size_min_safeguard) ? (buf_size_min_safeguard) : (buf_size_min);
            buf_size_min = (buf_size_min > buf_size_max_safeguard) ? (buf_size_max_safeguard) : (buf_size_min);
            buf_size_max = (buf_size_max < buf_size_min_safeguard) ? (buf_size_min_safeguard) : (buf_size_max);
            buf_size_max = (buf_size_max > buf_size_max_safeguard) ? (buf_size_max_safeguard) : (buf_size_max);
            F_LOG(F_PERF, "*** New buf range: [%d, %d]", buf_size_min, buf_size_max);
        }

        theta[0] = (theta[0] > buf_theta_max) ? buf_theta_max : theta[0];
        theta[0] = (theta[0] < buf_theta_min) ? buf_theta_min : theta[0];
        theta[1] = (theta[1] > stream_theta_max) ? stream_theta_max : theta[1];
        theta[1] = (theta[1] < stream_theta_min) ? stream_theta_min : theta[1];
        

        k = k + 1;
        
        ak = a / pow( (A + k + 1), alpha );
        ck = c / pow( (k + 1), gamma );
        
        for (int i = 0; i < 2; i++) {
            delta[i] = f_perturb();
        }
        
        F_LOG(F_PERF, "       Iteration#: %d", k);
        F_LOG(F_HINT, "profile#/max: %d/%d", profile_num, max_prof_limit);
        F_LOG(F_HINT, "repeat#: %d", repeat_number);
        F_LOG(F_HINT, "impeded#/limit: %d/%d", no_imp_counter, no_imp_limit);
        F_LOG(F_HINT, "max_perf/BW: %.3lf/%.3lf Gb/s", max_perf_Gbps, BW_Gbps);
        F_LOG(F_HINT, "max points: %.3lf, %.3lf", max_theta[0], max_theta[1]);
        F_LOG(F_HINT, "pgr(-C): %.3lf", perf_gain_ratio);
        
        F_LOG(F_PERF,"            ak/ck: %.3lf/%.3lf", ak, ck);
        F_LOG(F_HINT,"delta: %.3lf, %.3lf", delta[0], delta[1]);
        F_LOG(F_HINT,"theta: %.3lf, %.3lf", theta[0], theta[1]);
        F_LOG(F_HINT,"buf theta range: %.3lf, %.3lf]", buf_theta_min, buf_theta_max);
        F_LOG(F_HINT,"buf size range: %5d, %5d", buf_size_min, buf_size_max);
        F_LOG(F_HINT,"stream theta range: %.3lf,%.3lf", stream_theta_min, stream_theta_max);
        F_LOG(F_HINT,"stream num range: %5d, %5d", stream_num_min, stream_num_max);
        
        for (int i = 0; i < 2; i++) {
            theta_plus[i] = theta[i] + ck * delta[i];
            theta_minus[i] = theta[i] - ck * delta[i];
        }
        
        F_LOG(F_HINT, "theta(+): %.3lf, %.3lf", theta_plus[0], theta_plus[1]);
        F_LOG(F_HINT, "theta(-): %.3lf, %.3lf", theta_minus[0], theta_minus[1]);
        
        if (theta_plus[0] > buf_theta_max) {
            ck_adjust_plus[0] = ck * fabs((double)(buf_theta_max - theta[0]) / (double)(theta_plus[0] - theta[0]));
            theta_plus[0] = buf_theta_max;
        } else if (theta_plus[0] < buf_theta_min) {
            ck_adjust_plus[0] = ck * fabs((double)(buf_theta_min - theta[0]) / (double)(theta_plus[0] - theta[0]));
            theta_plus[0] = buf_theta_min;
        } else {
            ck_adjust_plus[0] = ck;
        }
        if (theta_plus[1] > stream_theta_max) {
            ck_adjust_plus[1] = ck * fabs((double)(stream_theta_max - theta[1]) / (double)(theta_plus[1] - theta[1]));
            theta_plus[1] = stream_theta_max;
        } else if (theta_plus[1] < stream_theta_min) {
            ck_adjust_plus[1] = ck * fabs((double)(stream_theta_min - theta[1]) / (double)(theta_plus[1] - theta[1]));
            theta_plus[1] = stream_theta_min;
        } else {
            ck_adjust_plus[1] = ck;
        }
        
        if (theta_minus[0] > buf_theta_max) {
            ck_adjust_minus[0] = ck * fabs((double)(buf_theta_max - theta[0]) / (double)(theta_minus[0] - theta[0]));
            theta_minus[0] = buf_theta_max;
        } else if (theta_minus[0] < buf_theta_min) {
            ck_adjust_minus[0] = ck * fabs((double)(buf_theta_min - theta[0]) / (double)(theta_minus[0] - theta[0]));
            theta_minus[0] = buf_theta_min;
        } else {
            ck_adjust_minus[0] = ck;
        }
        if (theta_minus[1] > stream_theta_max) {
            ck_adjust_minus[1] = ck * fabs((double)(stream_theta_max - theta[1]) / (double)(theta_minus[1] - theta[1]));
            theta_minus[1] = stream_theta_max;
        } else if (theta_minus[1] < stream_theta_min) {
            ck_adjust_minus[1] = ck * fabs((double)(stream_theta_min - theta[1]) / (double)(theta_minus[1] - theta[1]));
            theta_minus[1] = stream_theta_min;
        } else {
            ck_adjust_minus[1] = ck;
        }
        
        F_LOG(F_HINT, "adjusted theta(+): %.3lf, %.3lf", theta_plus[0], theta_plus[1]);
        F_LOG(F_HINT, "ck adjust(+): %.3lf, %.3lf", ck_adjust_plus[0], ck_adjust_plus[1]);
        F_LOG(F_HINT, "adjusted theta(-): %.3lf, %.3lf", theta_minus[0], theta_minus[1]);
        F_LOG(F_HINT, "ck adjust(-): %.3lf, %.3lf", ck_adjust_minus[0], ck_adjust_minus[1]);
        
        buf_size = f_tcp_theta_to_val(buf_theta_min, buf_theta_max, buf_size_min, buf_size_max, theta_plus[0]);
        stream_num = f_tcp_theta_to_val(stream_theta_min, stream_theta_max, stream_num_min, stream_num_max, theta_plus[1]);
        
        f_tcp_prof_reset(prof, ctrl);
        
        prof->tcp_prof->settings->socket_bufsize = (int)(f_round(buf_size * buf_unit * 0.5));
        prof->tcp_prof->num_streams = (int)(f_round(stream_num * stream_unit));

        
        F_LOG(F_PERF, "*** y(+): %.3lf %.3lf [%d, %d]", buf_size, stream_num, prof->tcp_prof->settings->socket_bufsize, prof->tcp_prof->num_streams);

        y_plus_Gbps = 0.0;
        
        if (iperf_run_client(prof->tcp_prof) < 0) {
            F_LOG(F_HINT, "iperf_run_client() fails %s", f_errstr(f_errno));
            return -1;
        } else {
            prof->perf_Mbps = f_get_perf_from_iperf(prof);
            y_plus_Gbps = prof->perf_Mbps / 1000.0;
            
            F_LOG(F_HINT, "record the current iperf3 test");
            if (f_tcp_m2m_record_compose(prof, &record, ctrl) < 0) {
                return 0;
            }
            if (f_tcp_m2m_record_add(prof, &record, ctrl) < 0) {
                return 0;
            }
        }
        
        profile_num = profile_num + 1;
        F_LOG(F_HINT, "profile number: %d", profile_num);
        
        F_LOG(F_PERF, "    y(+): %.3lf Gb/s", y_plus_Gbps);

        visited_point = new fastprof_tcp_trace;
        visited_point->buf_size = buf_size;
        visited_point->stream_num = stream_num;
        visited_point->marker = '+';
        visited_point->perf_Mbps = y_plus_Gbps * (1000.0);
        visited_point->run_or_not = 1;
        visited_list.push_back(visited_point);
        F_LOG(F_HINT, "pushed: [%.3lf, %.3lf]", buf_size, stream_num);
        
        if (y_plus_Gbps > max_perf_Gbps) {
            F_LOG(F_HINT, "a better y(+) is found [%.3lf, %.3lf]", buf_size, stream_num);
            max_perf_Gbps = y_plus_Gbps;
            max_theta[0] = theta_plus[0];
            max_theta[1] = theta_plus[1];
            F_LOG(F_HINT, "record the current best iperf test");
            memcpy(prof_max, prof, sizeof(struct fastprof_tcp_prof));
            F_LOG(F_HINT, "no improvement counter is cleared");
            no_imp_counter = 0;
            if ( (max_perf_Gbps / BW_Gbps) >= perf_gain_ratio ) {
                flag = 3;
                break;
            } else {
                ;
            }
        } else {
            F_LOG(F_HINT, "this time y(+) makes no progress [%.3lf, %.3lf]", buf_size, stream_num);
            no_imp_counter = no_imp_counter + 1;
        }
        
        sleep(5);
        
        buf_size = f_tcp_theta_to_val(buf_theta_min, buf_theta_max, buf_size_min, buf_size_max, theta_minus[0]);
        stream_num = f_tcp_theta_to_val(stream_theta_min, stream_theta_max, stream_num_min, stream_num_max, theta_minus[1]);
        f_tcp_prof_reset(prof, ctrl);
        prof->tcp_prof->settings->socket_bufsize = (int)(f_round(buf_size * buf_unit * 0.5));
        prof->tcp_prof->num_streams = (int)(f_round(stream_num * stream_unit));

        F_LOG(F_PERF, "*** y(-): %.3lf %.3lf [%d, %d]", buf_size, stream_num, prof->tcp_prof->settings->socket_bufsize, prof->tcp_prof->num_streams);
        
        y_minus_Gbps =0.0;

        if (iperf_run_client(prof->tcp_prof) < 0) {
            f_errno = F_ERR_ERROR;
            return -1;
        } else {
            prof->perf_Mbps = f_get_perf_from_iperf(prof);
            y_minus_Gbps =  prof->perf_Mbps / 1000.0;
            
            F_LOG(F_HINT, "record the current iperf test");
            if (f_tcp_m2m_record_compose(prof, &record, ctrl) < 0) {
                return 0;
            }
            if (f_tcp_m2m_record_add(prof, &record, ctrl) < 0) {
                return 0;
            }
        }
        
        profile_num = profile_num + 1;
        F_LOG(F_HINT, "profile number: %d", profile_num);
        
        F_LOG(F_PERF, "    y(-): %.3lf Gb/s", y_minus_Gbps);
        visited_point = new fastprof_tcp_trace;
        visited_point->buf_size = buf_size;
        visited_point->stream_num = stream_num;
        visited_point->marker = '-';
        visited_point->perf_Mbps = y_minus_Gbps * (1000.0);
        visited_point->run_or_not = 1;
        visited_list.push_back(visited_point);
        F_LOG(F_HINT, "pushed: [%.3lf, %.3lf]", buf_size, stream_num);

        if (y_minus_Gbps > max_perf_Gbps) {
            F_LOG(F_HINT, "a better y(-) is found [%.3lf, %.3lf]", buf_size, stream_num);
            max_perf_Gbps = y_minus_Gbps;
            max_theta[0] = theta_minus[0];
            max_theta[1] = theta_minus[1];
            F_LOG(F_HINT, "record the current best iperf test");			
            memcpy(prof_max, prof, sizeof(struct fastprof_tcp_prof));            
			F_LOG(F_HINT, "no improvement counter is cleared");
            no_imp_counter = 0;
            if ( ( max_perf_Gbps / BW_Gbps ) >= perf_gain_ratio ) {
                flag = 3;
                break;
            }
        } else {
            F_LOG(F_HINT, "this time y(-) makes no progress [%.3lf, %.3lf]", buf_size, stream_num);
            no_imp_counter = no_imp_counter + 1;
        }

        sleep(5);
        
        ghat[0] = (y_plus_Gbps - y_minus_Gbps) / ( (ck_adjust_plus[0] + ck_adjust_minus[0]) * delta[0] );
        ghat[1] = (y_plus_Gbps - y_minus_Gbps) / ( (ck_adjust_plus[1] + ck_adjust_minus[1]) * delta[1] );
        
        theta[0] = theta[0] + (ak * ghat[0]);
        theta[1] = theta[1] + (ak * ghat[1]);
        theta[0] = (theta[0] >= buf_theta_max) ? buf_theta_max : theta[0];
        theta[0] = (theta[0] <= buf_theta_min) ? buf_theta_min : theta[0];
        theta[1] = (theta[1] >= stream_theta_max) ? stream_theta_max : theta[1];
        theta[1] = (theta[1] <= stream_theta_min) ? stream_theta_min : theta[1];
        
        buf_size = f_tcp_theta_to_val(buf_theta_min, buf_theta_max, buf_size_min, buf_size_max, theta[0]);
        stream_num = f_tcp_theta_to_val(stream_theta_min, stream_theta_max, stream_num_min, stream_num_max, theta[1]);
        
        f_tcp_prof_reset(prof, ctrl);
        prof->tcp_prof->settings->socket_bufsize = (int)(f_round(buf_size * buf_unit * 0.5));
        prof->tcp_prof->num_streams = (int)(f_round(stream_num * stream_unit));
        F_LOG(F_PERF, "*** y(*): %.3lf %.3lf [%d, %d]", buf_size, stream_num, prof->tcp_prof->settings->socket_bufsize, prof->tcp_prof->num_streams);
        
        y_star_Gbps = 0.0;

        if (iperf_run_client(prof->tcp_prof) < 0) {
            f_errno = F_ERR_ERROR;
            return -1;
        } else {
            prof->perf_Mbps = f_get_perf_from_iperf(prof);
            y_star_Gbps = prof->perf_Mbps / 1000.0;
            F_LOG(F_HINT, "record the current iperf test");
            if (f_tcp_m2m_record_compose(prof, &record, ctrl) < 0) {
                return 0;
            }
            if (f_tcp_m2m_record_add(prof, &record, ctrl) < 0) {
                return 0;
            }
        }
        
        profile_num = profile_num + 1;        
        F_LOG(F_HINT, "profile# %d", profile_num);

        F_LOG(F_PERF, "    y(*): %.3lf Gb/s", y_star_Gbps);
        visited_point = new fastprof_tcp_trace;
        visited_point->buf_size = buf_size;
        visited_point->stream_num = stream_num;
        visited_point->marker = '*';
        visited_point->perf_Mbps = y_star_Gbps * (1000.0);
        visited_point->run_or_not = 1;
        visited_list.push_back(visited_point);
        F_LOG(F_HINT, "pushed: [%.3lf, %.3lf]", buf_size, stream_num);
        
        if (y_star_Gbps > max_perf_Gbps) {
            F_LOG(F_HINT, "a better y(*) is found [%.3lf, %.3lf]", buf_size, stream_num);
            max_perf_Gbps = y_star_Gbps;
            max_theta[0] = theta[0];
            max_theta[1] = theta[1];
            
            F_LOG(F_HINT, "record the current best iperf test");
            memcpy(prof_max, prof, sizeof(struct fastprof_tcp_prof));            
            F_LOG(F_HINT, "no improvement counter is cleared");
            no_imp_counter = 0;
            
            if ( ( max_perf_Gbps / BW_Gbps ) >= perf_gain_ratio ) {
                flag = 3;
                break;
            }
        } else {
            F_LOG(F_HINT, "this time y(*) makes no progress [%.3lf, %.3lf]", buf_size, stream_num);
            no_imp_counter = no_imp_counter + 1;
        }

        // always restart from the so-far best one
        theta[0] = max_theta[0];
        theta[1] = max_theta[1];
    
    } // end of while
    
    char curr_time[32];
    F_LOG(F_PERF, "*** FASTPROF TCP IS DONE WITH BELOW RESULT:");
    F_LOG(F_PERF, "    -----------------------------------------");
    if (f_get_currtime_str(curr_time) < 0) {
        f_errno = F_ERR_ERROR;
        return -1;
    }
    F_LOG(F_PERF, "    Time:     %s", curr_time);
    F_LOG(F_PERF, "    SrcIP:    %s", ctrl->local_ip);
    F_LOG(F_PERF, "    DstIP:    %s", ctrl->remote_ip);
    F_LOG(F_PERF, "    Protocol: %s", "TCP");
    F_LOG(F_PERF, "    PktSize:  %d", prof_max->tcp_prof->settings->mss);
    F_LOG(F_PERF, "    BlkSize:  %d", prof_max->tcp_prof->settings->blksize);
    F_LOG(F_PERF, "    BufSize:  %d", prof_max->tcp_prof->settings->socket_bufsize);
    F_LOG(F_PERF, "    Stream#:  %d", prof_max->tcp_prof->num_streams);
    F_LOG(F_PERF, "    MaxPerf:  %.3lf Mbps", prof_max->perf_Mbps);
    F_LOG(F_PERF, "    Impeded:  %d", ctrl->impeded_limit);
    F_LOG(F_PERF, "    Maximal:  %d", ctrl->max_limit);
    F_LOG(F_PERF, "    PGR:      %.3lf", ctrl->pgr);
    F_LOG(F_PERF, "    Profile#: %d", profile_num);
    F_LOG(F_PERF, "    TermCond: %d", flag);
    F_LOG(F_PERF, "    [1: Upper Bound; 2: Impeded; 3: PGR]");
    F_LOG(F_PERF, "    -----------------------------------------");

    if (f_tcp_get_prof_name(visited_list_filename, no_imp_limit, max_prof_limit, (int)(perf_gain_ratio * 100), ((ctrl->bw_bps > 0) && (ctrl->rtt_ms > 0)) ? "enabled" : "disabled") < 0) {
        f_errno = F_ERR_ERROR;
        return -1;
    }
    
    if ( (visited_list_fp = fopen(visited_list_filename, "a+")) == NULL) {
        F_LOG(F_HINT, "cannot open visit list file");
    } else {
        fprintf(visited_list_fp, "max perf: %.3lf Gb/s\n", max_perf_Gbps);
        fprintf(visited_list_fp, "profile#: %d\n", profile_num);
        fprintf(visited_list_fp, "terminated: %d\n", flag);
        fprintf(visited_list_fp, "1: max profile number\n  2: no improvement\n  3: desired performance\n");
        fprintf(visited_list_fp, "buf \tstream# \ttype \tperf \tnew_or_old\n");
        fflush(visited_list_fp);
        std::vector<fastprof_tcp_trace *>::iterator it;
        for (it = visited_list.begin(); it != visited_list.end(); ++it) {
            fprintf(visited_list_fp, "%.3lf \t%.3lf \t%c \t%.3lf \t%d\n", (*it)->buf_size, (*it)->stream_num, (*it)->marker, (*it)->perf_Mbps, (*it)->run_or_not);
            fflush(visited_list_fp);
        }
        fclose(visited_list_fp);
        visited_list_fp = NULL;
    }
    
    std::vector<fastprof_tcp_trace *>::iterator itt;
    for (itt = visited_list.begin(); itt != visited_list.end(); ++itt) {
        delete (*itt);
    }
    visited_list.clear();
    
    f_close_log();
    
    return 0;
}

double f_timeval_diff(struct timeval * tv0, struct timeval * tv1)
{
    double time1, time2;
    
    time1 = tv0->tv_sec + (tv0->tv_usec / 1000000.0);
    time2 = tv1->tv_sec + (tv1->tv_usec / 1000000.0);
    time1 = time1 - time2;
    if (time1 < 0) {
        time1 = -time1;
    }
    return time1;
}

double f_get_perf_from_iperf(struct fastprof_tcp_prof* prof)
{
    double perf_Mbps = -1.0;
    double start_time = 0.0;
    double end_time = 0.0;
    iperf_size_t bytes_sent = 0, total_sent = 0;
    struct iperf_stream *sp = NULL;
    
    start_time = 0.0;
    sp = SLIST_FIRST(&(prof->tcp_prof->streams));
    if (sp) {
        end_time = f_timeval_diff(&sp->result->start_time, &sp->result->end_time);
        
        SLIST_FOREACH(sp, &(prof->tcp_prof->streams), streams) {
            bytes_sent = sp->result->bytes_sent - sp->result->bytes_sent_omit;
            total_sent += bytes_sent;
        }
        prof->total_sent = total_sent;
        perf_Mbps = 8.0 * (double)total_sent / ((double)(end_time - start_time) * 1000.0 * 1000.0);
    } else {
        f_errno = F_ERR_ERROR;
        return -1.0;
    }
    return perf_Mbps;
}

int f_tcp_m2m_record_compose(struct fastprof_tcp_prof * prof, struct mem_record *record, struct fastprof_ctrl* ctrl)
{
    time_t t;
    struct tm currtime;
    t = time(NULL);
    currtime = *localtime(&t);

    memset(record, 0, sizeof(struct mem_record));

    memcpy(record->src_ip, ctrl->local_ip, strlen(ctrl->local_ip));
    record->src_ip[strlen(ctrl->local_ip)] = '\0';

    memcpy(record->dst_ip, ctrl->remote_ip, strlen(ctrl->remote_ip));
    record->src_ip[strlen(ctrl->remote_ip)] = '\0';

    sprintf(record->curr_time, "%02d:%02d:%d:%02d:%02d:%02d", currtime.tm_mon + 1, currtime.tm_mday, currtime.tm_year + 1900, currtime.tm_hour, currtime.tm_min, currtime.tm_sec);

    memcpy(record->protocol_name, "TCP", strlen("TCP"));
    record->protocol_name[strlen("TCP")] = '\0';

    memcpy(record->toolkit_name, "iperf3", strlen("iperf3"));
    record->toolkit_name[strlen("iperf3")] = '\0';

    record->num_streams = prof->tcp_prof->num_streams;
    record->mss_size = prof->tcp_prof->settings->mss;
    record->buffer_size = prof->tcp_prof->settings->socket_bufsize;
    memcpy(record->buffer_size_unit, "B", strlen("B"));
    record->buffer_size_unit[strlen("B")] = '\0';
    record->block_size = prof->tcp_prof->settings->blksize;
    memcpy(record->block_size_unit, "B", strlen("B"));
    record->block_size_unit[strlen("B")] = '\0';
    record->duration = prof->tcp_prof->duration;
    memcpy(record->duration_unit, "sec", strlen("sec"));
    record->duration_unit[strlen("sec")] = '\0';
    record->trans_size = prof->total_sent;
    memcpy(record->trans_size_unit, "B", strlen("B"));
    record->trans_size_unit[strlen("B")] = '\0';
    record->average_perf = prof->perf_Mbps;
    memcpy(record->average_perf_unit, "Mbps", strlen("Mbps"));
    record->average_perf_unit[strlen("Mbps")] = '\0';

    return 0;
}

int f_tcp_m2m_record_add(struct fastprof_tcp_prof * prof, struct mem_record *record, struct fastprof_ctrl* ctrl)
{
    struct stat st = {0};
    FILE *f = NULL;
    char filename[256];

    if (stat(F_DEFAULT_M2M_FOLDER, &st) == -1) {
        mkdir(F_DEFAULT_M2M_FOLDER, 0700);
    } else {
        F_LOG(F_HINT, "profile folder <%s> exists", F_DEFAULT_M2M_FOLDER);
    }
    sprintf(filename, "%s/%s_%s.PROF", F_DEFAULT_M2M_FOLDER, ctrl->local_ip, ctrl->remote_ip);

    f = fopen(filename, "a+");
    if (!f) {
        F_LOG(F_HINT, "open profile file error");
        return -1;
    }

    fprintf(f, MEM_RECORD_FORMAT_LABEL,
        (record->src_ip),
        (record->dst_ip),
        (record->curr_time),
        (record->protocol_name),
        (record->toolkit_name),
        (record->num_streams),
        (record->mss_size),
        (record->buffer_size),
        (record->buffer_size_unit),
        (record->block_size),
        (record->block_size_unit),
        (record->duration),
        (record->duration_unit),
        (record->trans_size),
        (record->trans_size_unit),
        (record->average_perf),
        (record->average_perf_unit));
    fprintf(f, "\n");
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    return 0;
}
