/***************************************************************************
 * FastProf: A Stochastic Approximation-based Transport Profiler
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include "fastprof_udt.h"

int emul_load_2d_tab(double** two_d_table, int row, int col)
{
    FILE *fp = NULL;
    
    fp = fopen("../conf/fastprof_2d_table.dat", "r");
    if(fp == NULL) {
        f_errno = F_ERR_FILE;
        return -1;
    }
    
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            fscanf (fp, "%lf", &(two_d_table[i][j]));
        }
    }
    
    fclose(fp);
    fp = NULL;
    
    return 0;
}

struct fastprof_udt_prof* f_udt_prof_new()
{
    struct fastprof_udt_prof* prof = new struct fastprof_udt_prof;
    prof->udt_prof = tpg_new_profile();
    return prof;
}

int f_udt_prof_default(struct fastprof_udt_prof* prof)
{
    if (tpg_prof_default(prof->udt_prof) < 0) {
        f_errno = F_ERR_TPG;
        return -1;
    } else {
        return 0;
    }
}

int f_udt_prof_init(struct fastprof_udt_prof* prof, struct fastprof_ctrl* ctrl)
{
    set_role(prof->udt_prof, (ctrl->role == F_CLIENT) ? (T_CLIENT) : (T_SERVER) );
    set_protocol_id(prof->udt_prof, T_PUDT);
#ifdef ANL_UCHICAGO
    memcpy(prof->udt_prof->local_ip, ctrl->local_ip, strlen(ctrl->local_ip));
    (prof->udt_prof->local_ip)[strlen(ctrl->local_ip)] = '\0';
#endif
    set_hostname(prof->udt_prof, ctrl->remote_ip);
    tpg_set_ctrl_listen_port(prof->udt_prof, ctrl->udt_port);
    tpg_set_udt_mss(prof->udt_prof, ctrl->mss);
    tpg_set_prof_time_duration(prof->udt_prof, ctrl->duration);
    
    return 0;
}

int f_udt_prof_reset(struct fastprof_udt_prof* prof, struct fastprof_ctrl* ctrl)
{
    if (ctrl->role == F_SERVER) {
        if (tpg_server_clean_up(prof->udt_prof) < 0) {
            f_errno = F_ERR_TPG;
            return -1;
        }
        if (tpg_server_reset(prof->udt_prof) < 0) {
            f_errno = F_ERR_TPG;
            return -1;
        }
    } else if (ctrl->role == F_CLIENT) {
        if (tpg_client_clean_up(prof->udt_prof) < 0) {
            f_errno = F_ERR_TPG;
            return -1;
        }
        if (tpg_client_reset(prof->udt_prof) < 0) {
            f_errno = F_ERR_TPG;
            return -1;
        }
    } else {
        return -1;
    }
    if (f_udt_prof_default(prof) < 0) {
        return -1;
    }
    if (f_udt_prof_init(prof, ctrl) < 0) {
        return -1;
    }
    return 0;
}

int f_udt_prof_delete(struct fastprof_udt_prof* prof)
{
    if (tpg_client_clean_up(prof->udt_prof) < 0) {
        return -1;
    }
    if (prof->udt_prof != NULL) {
        tpg_delete_profile(prof->udt_prof);
    }
    
    return 0;
}

double f_udt_theta_to_val(double theta_min, double theta_max, double value_min, double value_max, double theta)
{
    return (theta - theta_min) *  (value_max - value_min) / (theta_max - theta_min) + value_min;
}

int f_udt_get_prof_name(char* filename, int no_imp_limit, int max_prof_limit, int perf_gain_ratio_int, const char* enable_disable)
{
    time_t t;
    struct tm currtime;
    struct stat st = {0};
    int processid;
    
    if (stat("./udt_profile", &st) == -1) {
        mkdir("./udt_profile", 0700);
    } else {
        F_LOG(F_HINT, "fastprof profile folder exists");
    }
    
    t = time(NULL);
    currtime = *localtime(&t);
    processid = getpid();
    
    sprintf(filename, "./udt_profile/%d%02d%02d_%02d%02d%02d_%d_%d-%d-%d-%s.fpp",
            currtime.tm_year + 1900, currtime.tm_mon + 1, currtime.tm_mday, // year month day
            currtime.tm_hour, currtime.tm_min, currtime.tm_sec, // hour min sec
            processid, // process id
            no_imp_limit, // -L
            max_prof_limit, // -N
            perf_gain_ratio_int, // -C
            enable_disable);
    return 0;
}

int f_udt_client_run(struct fastprof_udt_prof* prof, struct fastprof_udt_prof* prof_max, struct fastprof_ctrl* ctrl)
{
    struct mem_record record;
    
    double alpha = ctrl->alpha;
    double gamma = ctrl->gamma;
    double A = ctrl->A_val;
    double a = ctrl->a_val;
    double a_scale = ctrl->a_scale;
    double c = ctrl->c_val;
    double c_scale = ctrl->c_scale;
    
    int block_size_min = F_BLK_MIN;
    int block_size_max = F_BLK_MAX;
    int block_unit = ctrl->mss - (20 + 8 + 16);
    double block_theta_min = F_ITERATIVE_MIN;
    double block_theta_max = F_ITERATIVE_MAX;
    
    int buffer_size_min = F_BUF_MIN;
    int buffer_size_max = (ctrl->host_max_buf < F_BUF_MAX) ? ctrl->host_max_buf : F_BUF_MAX;
    int buffer_size_min_safeguard = buffer_size_min;
    int buffer_size_max_safeguard = buffer_size_max;
    int buffer_unit = 1;
    double buffer_theta_min = F_ITERATIVE_MIN;
    double buffer_theta_max = F_ITERATIVE_MAX;

    double block_size = -1.0;
    double buffer_size = -1.0;

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
    
    int profile_number = 0;
    int no_imp_counter = 0;
    int repeat_number = 0;
    int flag = -1;
    
    std::vector<fastprof_udt_trace *> visited_list;
    visited_list.clear();
    fastprof_udt_trace* visited_point = NULL;
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
    } else {
        max_prof_limit = ctrl->max_limit;
    }
    if (ctrl->pgr < 0) {
        perf_gain_ratio = F_DEFAULT_PGR;
    } else {
        perf_gain_ratio = ctrl->pgr;
    }
    
    F_LOG(F_PERF, "     Impeded (-L): %d", no_imp_limit);
    F_LOG(F_PERF, "     Maximal (-N): %d", max_prof_limit);
    F_LOG(F_PERF, "         PGR (-C): %.3lf", perf_gain_ratio);
    F_LOG(F_PERF, "          BW (-y): %.3lf bps", ctrl->bw_bps);
    F_LOG(F_PERF, "         RTT (-T): %.3lf ms", ctrl->rtt_ms);
    F_LOG(F_PERF, "    Duration (-d): %d sec", ctrl->duration);
    F_LOG(F_PERF, " Buffer safeguard: %d %d", buffer_size_min_safeguard, buffer_size_max_safeguard);
    
    theta[0] = (f_rand((int)(block_theta_min) + 1, (int)(block_theta_max) - 1));
    theta[1] = (f_rand((int)(buffer_theta_min) + 1, (int)(buffer_theta_max) - 1));
    
    while (true) {
        F_LOG(F_PERF, "***         Upper: %d of %d", profile_number, max_prof_limit);
        F_LOG(F_PERF, "***       Impeded: %d of %d", no_imp_counter, no_imp_limit);
        F_LOG(F_PERF, "***       MaxPerf: %.3lf Gb/s", max_perf_Gbps);
        
        flag = -1;
        if (profile_number >= max_prof_limit) {
            F_LOG(F_HINT, "profile#/max#: %d/%d", profile_number, max_prof_limit);
            flag = 1;
            break;
        }
        if (no_imp_counter >= no_imp_limit / 2) {
            a *= a_scale;
            F_LOG(F_PERF, "          a scale: %.3lf %.3lf", a_scale, a);
            c *= c_scale;
            F_LOG(F_PERF, "          c scale: %.3lf %.3lf", c_scale, c);
        }
        if ( no_imp_counter >= no_imp_limit ) {
            F_LOG(F_HINT, "impeded#/max#: %d/%d", no_imp_counter, no_imp_limit);
            flag = 2;
            break;
        }
        if ( ( max_perf_Gbps / BW_Gbps  ) >= perf_gain_ratio ) {
            F_LOG(F_HINT, "max_perf/BW: %.3lf/%.3lf", max_perf_Gbps, BW_Gbps);
            flag = 3;
            break;
        }
        
        BDP_B = -1.0;
        if (ctrl->bw_bps < 0) {
            sleep(5);
        } else {
            BW_Bps = ctrl->bw_bps / 8.0;
            F_LOG(F_HINT, "BW is given as: %3.lfBps", BW_Bps);

            if ((SRTT_ms = f_estimate_rtt(ctrl->remote_ip))<0) {
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
            buffer_size_min = f_round(0.75 * BDP_B / buffer_unit);
            buffer_size_max = f_round(4.00 * BDP_B / buffer_unit);
            // safe guard
            buffer_size_min = (buffer_size_min < buffer_size_min_safeguard) ? (buffer_size_min_safeguard) : (buffer_size_min);
            buffer_size_min = (buffer_size_min > buffer_size_max_safeguard) ? (buffer_size_max_safeguard) : (buffer_size_min);
            buffer_size_max = (buffer_size_max < buffer_size_min_safeguard) ? (buffer_size_min_safeguard) : (buffer_size_max);
            buffer_size_max = (buffer_size_max > buffer_size_max_safeguard) ? (buffer_size_max_safeguard) : (buffer_size_max);
            F_LOG(F_PERF, "*** New buf range: [%d, %d]", buffer_size_min, buffer_size_max);
        }
        
        theta[0] = (theta[0] > block_theta_max) ? block_theta_max : theta[0];
        theta[0] = (theta[0] < block_theta_min) ? block_theta_min : theta[0];
        theta[1] = (theta[1] > buffer_theta_max) ? buffer_theta_max : theta[1];
        theta[1] = (theta[1] < buffer_theta_min) ? buffer_theta_min : theta[1];
        
        k = k + 1;
        
        ak = a / pow( (A + k + 1), alpha );
        ck = c / pow( (k + 1), gamma );
        
        for (int i = 0; i < 2; i++) {
            delta[i] = f_perturb();
        }
        
        F_LOG(F_PERF, "       Iteration#: %d", k);
        F_LOG(F_HINT, "profile#/max#: %d/%d", profile_number, max_prof_limit);
        F_LOG(F_HINT, "repeat#: %d", repeat_number);
        F_LOG(F_HINT, "impeded#/limit: %d/%d", no_imp_counter, no_imp_limit);
        F_LOG(F_HINT, "max_perf/BW: %.3lf/%.3lf Gb/s", max_perf_Gbps, BW_Gbps);
        F_LOG(F_HINT, "max points: %.3lf, %.3lf", max_theta[0], max_theta[1]);
        F_LOG(F_HINT, "pgr(-C): %.3lf", perf_gain_ratio);
        
        F_LOG(F_PERF,"            ak/ck: %.3lf/%.3lf", ak, ck);
        F_LOG(F_HINT, "delta: %.3lf, %.3lf", delta[0], delta[1]);
        F_LOG(F_HINT, "theta: %.3lf, %.3lf", theta[0], theta[1]);
        F_LOG(F_HINT, "blk theta range: %.3lf, %.3lf", block_theta_min, block_theta_max);
        F_LOG(F_HINT, "blk size range: %d, %d", block_size_min, block_size_max);
        F_LOG(F_HINT, "buf theta range: %.3lf, %.3lf", buffer_theta_min, buffer_theta_max);
        F_LOG(F_HINT, "buf size range: %d, %d", buffer_size_min, buffer_size_max);
        
        for (int i = 0; i < 2; i++)	{
            theta_plus[i] = theta[i] + ck * delta[i];
            theta_minus[i] = theta[i] - ck * delta[i];
        }
        
        F_LOG(F_HINT, "theta(+): %.3lf, %.3lf", theta_plus[0], theta_plus[1]);
        F_LOG(F_HINT, "theta(-): %.3lf, %.3lf", theta_minus[0], theta_minus[1]);
        
        if (theta_plus[0] > block_theta_max) {
            ck_adjust_plus[0] =  ck * fabs((double)(block_theta_max - theta[0]) / (double)(theta_plus[0] - theta[0]) );
            theta_plus[0] = block_theta_max;
        } else if (theta_plus[0] < block_theta_min) {
            ck_adjust_plus[0] = ck * fabs( (double)(block_theta_min - theta[0]) / (double)(theta_plus[0] - theta[0]) );
            theta_plus[0] = block_theta_min;
        } else {
            ck_adjust_plus[0] = ck;
        }
        if (theta_plus[1] > buffer_theta_max) {
            ck_adjust_plus[1] = ck * fabs( (double)(buffer_theta_max - theta[1]) / (double)(theta_plus[1] - theta[1]) );
            theta_plus[1] = buffer_theta_max;
        } else if (theta_plus[1] < buffer_theta_min) {
            ck_adjust_plus[1] = ck * fabs( (double)(buffer_theta_min - theta[1]) / (double)(theta_plus[1] - theta[1]) );
            theta_plus[1] = buffer_theta_min;
        } else {
            ck_adjust_plus[1] = ck;
        }
        
        if (theta_minus[0] > block_theta_max) {
            ck_adjust_minus[0] = ck * fabs( (double)(block_theta_max - theta[0]) / (double)(theta_minus[0] - theta[0]) );
            theta_minus[0] = block_theta_max;
        } else if (theta_minus[0] < block_theta_min) {
            ck_adjust_minus[0] = ck * fabs( (double)(block_theta_min - theta[0]) / (double)(theta_minus[0] - theta[0]) );
            theta_minus[0] = block_theta_min;
        } else {
            ck_adjust_minus[0] = ck;
        }
        
        if (theta_minus[1] > buffer_theta_max) {
            ck_adjust_minus[1] = ck * fabs( (double)(buffer_theta_max - theta[1]) /  (double)(theta_minus[1] - theta[1]) );
            theta_minus[1] = buffer_theta_max;
        } else if (theta_minus[1] < buffer_theta_min) {
            ck_adjust_minus[1] =  ck * fabs( (double)(buffer_theta_min - theta[1]) / (double)(theta_minus[1] - theta[1]) );
            theta_minus[1] = buffer_theta_min;
        } else {
            ck_adjust_minus[1] = ck;
        }
        
        F_LOG(F_HINT, "adjusted theta(+): %.3lf, %.3lf", theta_plus[0], theta_plus[1]);
        F_LOG(F_HINT, "adjusted ck(+): %.3lf, %.3lf", ck_adjust_plus[0], ck_adjust_plus[1]);
        F_LOG(F_HINT, "adjusted theta(-): %.3lf, %.3lf", theta_minus[0], theta_minus[1]);
        F_LOG(F_HINT, "adjusted ck(-): %.3lf, %.3lf", ck_adjust_minus[0], ck_adjust_minus[1]);
        
        block_size = f_udt_theta_to_val(block_theta_min, block_theta_max, block_size_min, block_size_max, theta_plus[0]);
        buffer_size = f_udt_theta_to_val(buffer_theta_min, buffer_theta_max, buffer_size_min, buffer_size_max, theta_plus[1]);
        
        tpg_set_blocksize(prof->udt_prof, (int)(f_round(block_size * block_unit)));
        tpg_set_udt_send_bufsize(prof->udt_prof, (int)(f_round(buffer_size * buffer_unit)));
        tpg_set_udt_recv_bufsize(prof->udt_prof, (int)(f_round(buffer_size * buffer_unit)));
        tpg_set_udp_send_bufsize(prof->udt_prof, (int)(f_round(buffer_size * buffer_unit * 0.5)));
        tpg_set_udp_recv_bufsize(prof->udt_prof, (int)(f_round(buffer_size * buffer_unit * 0.5)));
        
        F_LOG(F_PERF, "*** y(+): %.3lf %.3lf [%d, %d]", block_size, buffer_size, tpg_get_blocksize(prof->udt_prof), tpg_get_udt_send_bufsize(prof->udt_prof));
        
        tpg_set_client_perf_mbps(prof->udt_prof, 0.0);
        tpg_set_server_perf_mbps(prof->udt_prof, 0.0);

        y_plus_Gbps = 0.0;
        
        if (tpg_client_run(prof->udt_prof) < 0) {
            F_LOG(F_HINT, "tpg_client_run() fails %s", f_errstr(f_errno));
            return -1;
        } else {
            prof->perf_Mbps = tpg_get_client_perf_mbps(prof->udt_prof);
            y_plus_Gbps = prof->perf_Mbps / 1000.0;
            
            F_LOG(F_HINT, "record the current tpg prof");
            if (f_udt_m2m_record_compose(prof, &record, ctrl) < 0) {
                return 0;
            }
            if (f_udt_m2m_record_add(prof, &record, ctrl) < 0) {
                return 0;
            }
        }
        
        profile_number = profile_number + 1;
        F_LOG(F_HINT, "profile#: %d", profile_number);
        
        F_LOG(F_PERF, "    y(+): %.3lf Gb/s", y_plus_Gbps);

        visited_point = new fastprof_udt_trace;
        visited_point->blk_size = block_size;
        visited_point->buf_size = buffer_size;
        visited_point->marker = '+';
        visited_point->perf_Mbps = y_plus_Gbps * (1000.0);
        visited_point->run_or_not = 1;
        visited_list.push_back(visited_point);
        F_LOG(F_HINT, "pushed: [%.3lf, %.3lf]", block_size, buffer_size);
        
        if (y_plus_Gbps > max_perf_Gbps) {
            F_LOG(F_HINT, "a better y(+) is found [%.3lf, %.3lf]", block_size, buffer_size);
            max_perf_Gbps = y_plus_Gbps;
            max_theta[0] = theta_plus[0];
            max_theta[1] = theta_plus[1];
            F_LOG(F_HINT, "record the current best tpg prof");
            memcpy(prof_max, prof, sizeof(struct fastprof_udt_prof));            
            F_LOG(F_HINT, "No improvement counter is cleared");
            no_imp_counter = 0;
            if ( ( max_perf_Gbps / BW_Gbps ) >= perf_gain_ratio ) {
                flag = 3;
                break;
            }
        } else {
            F_LOG(F_HINT, "this time y(+) makes no progress [%.3lf, %.3lf]", block_size, buffer_size);
            no_imp_counter = no_imp_counter + 1;
        }
        sleep(5);
        
        block_size = f_udt_theta_to_val(block_theta_min, block_theta_max, block_size_min, block_size_max, theta_minus[0]);
        buffer_size = f_udt_theta_to_val(buffer_theta_min, buffer_theta_max, buffer_size_min, buffer_size_max, theta_minus[1]);
        
        tpg_set_blocksize(prof->udt_prof, (int)(f_round(block_size * block_unit)));
        tpg_set_udt_send_bufsize(prof->udt_prof, (int)(f_round(buffer_size * buffer_unit)));
        tpg_set_udt_recv_bufsize(prof->udt_prof, (int)(f_round(buffer_size * buffer_unit)));
        tpg_set_udp_send_bufsize(prof->udt_prof, (int)(f_round(buffer_size * buffer_unit * 0.5)));
        tpg_set_udp_recv_bufsize(prof->udt_prof, (int)(f_round(buffer_size * buffer_unit * 0.5)));

        F_LOG(F_PERF, "*** y(-): %.3lf %.3lf [%d, %d]", block_size, buffer_size, tpg_get_blocksize(prof->udt_prof), tpg_get_udt_send_bufsize(prof->udt_prof));
        
        tpg_set_client_perf_mbps(prof->udt_prof, 0.0);
        tpg_set_server_perf_mbps(prof->udt_prof, 0.0);
        
        y_minus_Gbps = 0.0;
        
        if (tpg_client_run(prof->udt_prof) < 0) {
            f_errno = F_ERR_ERROR;
            return -1;
        } else {
            prof->perf_Mbps = tpg_get_client_perf_mbps(prof->udt_prof);
            y_minus_Gbps = prof->perf_Mbps / 1000.0;
            
            F_LOG(F_HINT, "record the current tpg prof");
            if (f_udt_m2m_record_compose(prof, &record, ctrl) < 0) {
                return 0;
            }
            if (f_udt_m2m_record_add(prof, &record, ctrl) < 0) {
                return 0;
            }
        }
        
        profile_number = profile_number + 1;
        F_LOG(F_HINT, "profile#: %d", profile_number);
        
        F_LOG(F_PERF, "    y(-): %.3lf Gb/s", y_minus_Gbps);
        visited_point = new fastprof_udt_trace;
        visited_point->blk_size = block_size;
        visited_point->buf_size = buffer_size;
        visited_point->marker = '-';
        visited_point->perf_Mbps = y_minus_Gbps * (1000.0);
        visited_point->run_or_not = 1;
        visited_list.push_back(visited_point);
        F_LOG(F_HINT, "pushed: [%.3lf, %.3lf]", block_size, buffer_size);
        if (y_minus_Gbps > max_perf_Gbps) {
            F_LOG(F_HINT, "a better y(-) is found [%.3lf, %.3lf]", block_size, buffer_size);
            max_perf_Gbps = y_minus_Gbps;
            max_theta[0] = theta_minus[0];
            max_theta[1] = theta_minus[1];
            F_LOG(F_HINT, "record the current best tpg prof");
            memcpy(prof_max, prof, sizeof(struct fastprof_udt_prof));            
            F_LOG(F_HINT, "No improvement counter is cleared");
            no_imp_counter = 0;
            if ( ( max_perf_Gbps / BW_Gbps ) >= perf_gain_ratio ) {
                flag = 3;
                break;
            }
        } else {
            F_LOG(F_HINT, "this time y(-) makes no progress [%.3lf, %.3lf]", block_size, buffer_size);
            no_imp_counter = no_imp_counter + 1;
        }
        
        sleep(5);
        
        ghat[0] = (y_plus_Gbps - y_minus_Gbps) / ( (ck_adjust_plus[0] + ck_adjust_minus[0]) * delta[0] );
        ghat[1] = (y_plus_Gbps - y_minus_Gbps) / ( (ck_adjust_plus[1] + ck_adjust_minus[1]) * delta[1] );
        
        theta[0] = theta[0] + (ak * ghat[0]);
        theta[1] = theta[1] + (ak * ghat[1]);
        theta[0] = (theta[0] >= block_theta_max) ? block_theta_max : theta[0];
        theta[0] = (theta[0] <= block_theta_min) ? block_theta_min : theta[0];
        theta[1] = (theta[1] >= buffer_theta_max) ? buffer_theta_max : theta[1];
        theta[1] = (theta[1] <= buffer_theta_min) ? buffer_theta_min : theta[1];
        
        block_size = f_udt_theta_to_val(block_theta_min, block_theta_max, block_size_min, block_size_max, theta[0]);
        buffer_size = f_udt_theta_to_val(buffer_theta_min, buffer_theta_max, buffer_size_min, buffer_size_max, theta[1]);

        tpg_set_blocksize(prof->udt_prof, (int)(f_round(block_size * block_unit)));
        tpg_set_udt_send_bufsize(prof->udt_prof, (int)(f_round(buffer_size * buffer_unit)));
        tpg_set_udt_recv_bufsize(prof->udt_prof, (int)(f_round(buffer_size * buffer_unit)));
        tpg_set_udp_send_bufsize(prof->udt_prof, (int)(f_round(buffer_size * buffer_unit * 0.5)));
        tpg_set_udp_recv_bufsize(prof->udt_prof, (int)(f_round(buffer_size * buffer_unit * 0.5)));

        F_LOG(F_PERF, "*** y(*): %.3lf %.3lf [%d, %d]", block_size, buffer_size, tpg_get_blocksize(prof->udt_prof), tpg_get_udt_send_bufsize(prof->udt_prof));

        tpg_set_client_perf_mbps(prof->udt_prof, 0.0);
        tpg_set_server_perf_mbps(prof->udt_prof, 0.0);
        
        y_star_Gbps = 0.0;
        
        if (tpg_client_run(prof->udt_prof) < 0) {
            f_errno = F_ERR_ERROR;
            return -1;
        } else {
            prof->perf_Mbps = tpg_get_client_perf_mbps(prof->udt_prof);
            y_star_Gbps = prof->perf_Mbps / 1000.0;
            
            F_LOG(F_HINT, "record the current tpg prof");
            if (f_udt_m2m_record_compose(prof, &record, ctrl) < 0) {
                return 0;
            }
            if (f_udt_m2m_record_add(prof, &record, ctrl) < 0) {
                return 0;
            }
        }
        
        profile_number = profile_number + 1;        
        F_LOG(F_HINT, "profile#: %d", profile_number);
        
        F_LOG(F_PERF, "    y(*): %.3lf Gb/s", y_star_Gbps);
        visited_point = new fastprof_udt_trace;
        visited_point->blk_size = block_size;
        visited_point->buf_size = buffer_size;
        visited_point->marker = '*';
        visited_point->perf_Mbps = y_star_Gbps * (1000.0);
        visited_point->run_or_not = 1;
        visited_list.push_back(visited_point);
        F_LOG(F_HINT, "pushed: [%.3lf, %.3lf]", block_size, buffer_size);

        if (y_star_Gbps > max_perf_Gbps) {
            F_LOG(F_HINT, "a better y(*) is found [%.3lf, %.3lf]", block_size, buffer_size);
            max_perf_Gbps = y_star_Gbps;
            max_theta[0] = theta[0];
            max_theta[1] = theta[1];
            F_LOG(F_HINT, "record the current best iperf test");
            memcpy(prof_max, prof, sizeof(struct fastprof_udt_prof));
            F_LOG(F_HINT, "no improvement counter is cleared");
            no_imp_counter = 0;
            if ( ( max_perf_Gbps / BW_Gbps ) >= perf_gain_ratio ) {
                flag = 3;
                break;
            }
        } else {
            F_LOG(F_HINT, "this time y(*) makes no progress [%.3lf, %.3lf]", block_size, buffer_size);
            no_imp_counter = no_imp_counter + 1;
        }
        
        theta[0] = max_theta[0];
        theta[1] = max_theta[1];
    } // end of while
    
    char curr_time[32];
    F_LOG(F_PERF, "\n+++++++++++++++++++++++++++++++++++++++++++++++++");
    F_LOG(F_PERF, "*** FASTPROF UDT IS DONE WITH BELOW RESULT:");
    F_LOG(F_PERF, "    -----------------------------------------");
    if (f_get_currtime_str(curr_time) < 0) {
        f_errno = F_ERR_ERROR;
        return -1;
    }
    F_LOG(F_PERF, "    Time:     %s", curr_time);
    F_LOG(F_PERF, "    SrcIP:    %s", ctrl->local_ip);
    F_LOG(F_PERF, "    DstIP:    %s", ctrl->remote_ip);
    F_LOG(F_PERF, "    Protocol: %s", "UDT");
    F_LOG(F_PERF, "    PktSize:  %d", tpg_get_udt_mss(prof_max->udt_prof) - 44);
    F_LOG(F_PERF, "    BlkSize:  %d", tpg_get_blocksize(prof_max->udt_prof));
    F_LOG(F_PERF, "    BufSize:  %d", tpg_get_udt_send_bufsize(prof_max->udt_prof));
    F_LOG(F_PERF, "    Stream#:  %d", tpg_get_total_stream_num(prof_max->udt_prof));
    F_LOG(F_PERF, "    MaxPerf:  %.3lf Mbps", prof_max->perf_Mbps);
    F_LOG(F_PERF, "    Impeded:  %d", ctrl->impeded_limit);
    F_LOG(F_PERF, "    Maximal:  %d", ctrl->max_limit);
    F_LOG(F_PERF, "    PGR:      %.3lf", ctrl->pgr);
    F_LOG(F_PERF, "    Profile#: %d", profile_number);
	F_LOG(F_PERF, "    TermCond: %d", flag);
    F_LOG(F_PERF, "    [1: Upper Bound; 2: Impeded; 3: PGR]");
    F_LOG(F_PERF, "    -----------------------------------------");
    
    if (f_udt_get_prof_name(visited_list_filename, no_imp_limit, max_prof_limit, (int)(perf_gain_ratio * 100), ((ctrl->bw_bps > 0) && (ctrl->rtt_ms > 0)) ? "enabled" : "disabled") < 0) {
        f_errno = F_ERR_ERROR;
        return -1;
    }
    
    if((visited_list_fp = fopen (visited_list_filename, "a+")) == NULL) {
        F_LOG(F_PERF, "cannot open visit list file");
    } else {
        fprintf(visited_list_fp, "max perf: %.3lf Gb/s\n", max_perf_Gbps);
        fprintf(visited_list_fp, "profile#: %d\n", profile_number);
        fprintf(visited_list_fp, "terminated: %d\n", flag);
        fprintf(visited_list_fp, "  1: max profile#\n  2: no improvement\n  3: desired performance\n");
        fprintf(visited_list_fp, "blk \tbuf \ttype \tperf \tnew_or_old\n");
        fflush(visited_list_fp);
        
        std::vector<fastprof_udt_trace *>::iterator it;
        for (it = visited_list.begin(); it != visited_list.end(); ++it) {
            fprintf(visited_list_fp, "%.3lf \t%.3lf \t%c \t%.3lf \t%d\n", (*it)->blk_size, (*it)->buf_size, (*it)->marker, (*it)->perf_Mbps, (*it)->run_or_not);
            fflush(visited_list_fp);
        }
        fclose(visited_list_fp);
        visited_list_fp = NULL;
    }

    std::vector<fastprof_udt_trace *>::iterator itt;
    for (itt = visited_list.begin(); itt != visited_list.end(); ++itt) {
        delete (*itt);
    }
    visited_list.clear();

    f_close_log();
    
    return 0;
}

int f_udt_m2m_record_compose(struct fastprof_udt_prof * prof, struct mem_record *record, struct fastprof_ctrl* ctrl)
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

    memcpy(record->protocol_name, "UDT", strlen("UDT"));
    record->protocol_name[strlen("UDT")] = '\0';

    memcpy(record->toolkit_name, "tpg", strlen("tpg"));
    record->toolkit_name[strlen("tpg")] = '\0';

    record->num_streams = tpg_get_total_stream_num(prof->udt_prof);
    record->mss_size = tpg_get_udt_mss(prof->udt_prof);
    record->buffer_size = tpg_get_udt_send_bufsize(prof->udt_prof);
    memcpy(record->buffer_size_unit, "B", strlen("B"));
    record->buffer_size_unit[strlen("B")] = '\0';
    record->block_size = tpg_get_blocksize(prof->udt_prof);
    memcpy(record->block_size_unit, "B", strlen("B"));
    record->block_size_unit[strlen("B")] = '\0';
    record->duration = tpg_get_prof_time_duration(prof->udt_prof);
    memcpy(record->duration_unit, "sec", strlen("sec"));
    record->duration_unit[strlen("sec")] = '\0';
    record->trans_size = tpg_get_total_sent_bytes(prof->udt_prof);
    memcpy(record->trans_size_unit, "B", strlen("B"));
    record->trans_size_unit[strlen("B")] = '\0';
    record->average_perf = tpg_get_client_perf_mbps(prof->udt_prof);
    memcpy(record->average_perf_unit, "Mbps", strlen("Mbps"));
    record->average_perf_unit[strlen("Mbps")] = '\0';

    return 0;
}

int f_udt_m2m_record_add(struct fastprof_udt_prof * prof, struct mem_record *record, struct fastprof_ctrl* ctrl)
{
    struct stat st = {0};
    FILE *f = NULL;
    char filename[256];
    char command[128];

    snprintf(command, sizeof(command), "mkdir -p %s", F_DEFAULT_M2M_FOLDER);
    if (stat(F_DEFAULT_M2M_FOLDER, &st) == -1) {
        if (system(command) != 0) {
            F_LOG(F_ERR, "Cannot create folder %s %s", F_DEFAULT_M2M_FOLDER, strerror(errno));
        } else {
            F_LOG(F_HINT, "profile folder <%s> created", F_DEFAULT_M2M_FOLDER);
        }
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
