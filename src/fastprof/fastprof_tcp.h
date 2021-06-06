/***************************************************************************
 * FastProf: A Stochastic Approximation-based Transport Profiler
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _FASTPROF_TCP_H_
#define _FASTPROF_TCP_H_

#include <cmath>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sysexits.h>
#include <stdint.h>
#include "fastprof.h"
#include "fastprof_log.h"
#include "fastprof_error.h"

#ifdef __cplusplus
extern "C"{
#endif
    #include <iperf.h>
    #include <iperf_api.h>
#ifdef __cplusplus
}
#endif

struct fastprof_tcp_prof* f_tcp_prof_new();
int f_tcp_prof_default(struct fastprof_tcp_prof* prof);
int f_tcp_prof_init(struct fastprof_tcp_prof* prof, struct fastprof_ctrl* ctrl);
int f_tcp_prof_reset(struct fastprof_tcp_prof* prof, struct fastprof_ctrl* ctrl);
int f_tcp_prof_delete(struct fastprof_tcp_prof * prof);
int f_tcp_theta_to_idx(double theta_val, int scale, int min_val, int max_val);
double f_tcp_theta_to_val(double theta_min, double theta_max, double val_min, double val_max, double theta);
int f_tcp_get_prof_name(char* filename, int no_imp_limit, int max_prof_limit, int perf_gain_ratio_int, const char* enable_disable);
int f_tcp_client_run(struct fastprof_tcp_prof * prof, struct fastprof_tcp_prof * prof_max, struct fastprof_ctrl* ctrl);
int f_tcp_m2m_record_compose(struct fastprof_tcp_prof * prof, struct mem_record *record, struct fastprof_ctrl* ctrl);
int f_tcp_m2m_record_add(struct fastprof_tcp_prof * prof, struct mem_record *record, struct fastprof_ctrl* ctrl);
double f_timeval_diff(struct timeval * tv0, struct timeval * tv1);
double f_get_perf_from_iperf(struct fastprof_tcp_prof* prof);

#endif