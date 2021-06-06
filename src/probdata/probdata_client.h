/***************************************************************************
 * ProbData - PRofiling Optimization Based DAta Transfer Advisor
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _PROBDATA_CLIENT_H_
#define _PROBDATA_CLIENT_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "probdata.h"

int probdata_client(struct probdata* prob);
int probdata_client_connect(struct probdata* prob);
int probdata_client_poll_ctrl_channel(struct probdata* prob);
int probdata_client_proc_ctrl_packet(struct probdata* prob);
void probdata_client_id_gen(char *id);
int probdata_client_check_id(struct probdata* prob);
int probdata_client_clean_up(struct probdata* prob);

double probdata_client_search_d2d(struct probdata* prob);
double probdata_client_search_m2m(struct probdata* prob);

int probdata_client_dw_xdd_tcp(probdata* prob);
int probdata_client_dw_xdd_udt(probdata* prob);

int probdata_client_tcp_prof_new(probdata* prob);
int probdata_client_tcp_prof_default(probdata* prob);
int probdata_client_tcp_prof_init(probdata* prob);
int probdata_client_tcp_fastprof_run(probdata *prob);
int probdata_client_tcp_prof_reset(probdata *prob);

int probdata_client_udt_prof_new(probdata* prob);
int probdata_client_udt_prof_default(struct probdata* prob);
int probdata_client_udt_prof_init(struct probdata* prob);
int probdata_client_udt_fastprof_run(struct probdata* prob);
int probdata_client_udt_prof_reset(struct probdata* prob);

int probdata_client_m2m_record_send(probdata* prob, struct mem_record *rec);

#endif
