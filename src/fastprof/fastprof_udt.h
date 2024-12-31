/***************************************************************************
 * FastProf: A Stochastic Approximation-based Transport Profiler
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _FASTPROF_UDT_H_
#define _FASTPROF_UDT_H_

#include <cmath>
#include <tpg.h>
#include "fastprof.h"
#include "fastprof_log.h"
#include "fastprof_error.h"

struct fastprof_udt_prof* f_udt_prof_new();
int f_udt_prof_default(struct fastprof_udt_prof* prof);
int f_udt_prof_init(struct fastprof_udt_prof* prof, struct fastprof_ctrl* ctrl);
int f_udt_prof_reset(struct fastprof_udt_prof* prof, struct fastprof_ctrl* ctrl);
int f_udt_prof_delete(struct fastprof_udt_prof* prof);
int emul_load_2d_tab(double** two_d_table, int row, int col);
double f_udt_theta_to_val(double theta_min, double theta_max, double value_min, double value_max, double theta);
int f_udt_get_prof_name(char* filename, int no_imp_limit, int max_prof_limit, int perf_gain_ratio_int, const char* enable_disable);
int f_udt_client_run(struct fastprof_udt_prof* prof, struct fastprof_udt_prof* prof_max, struct fastprof_ctrl* ctrl);
int f_udt_m2m_record_compose(struct fastprof_udt_prof * prof, struct mem_record *record, struct fastprof_ctrl* ctrl);
int f_udt_m2m_record_add(struct fastprof_udt_prof * prof, struct mem_record *record, struct fastprof_ctrl* ctrl);

#endif