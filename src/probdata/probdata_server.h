/***************************************************************************
 * ProbData - PRofiling Optimization Based DAta Transfer Advisor
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _PROBDATA_SERVER_H_
#define _PROBDATA_SERVER_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "probdata.h"

int probdata_server(struct probdata* prob);
int probdata_server_listen(struct probdata* prob);
int probdata_server_accept(struct probdata* prob);
int probdata_server_poll_ctrl_channel(struct probdata* prob);
int probdata_server_proc_ctrl_packet(struct probdata* prob);
int probdata_server_clean_up(struct probdata* prob);
int probdata_server_m2m_record_recv(probdata* prob, struct mem_record* rec);

int probdata_server_tcp_prof_new(struct probdata* prob);
int probdata_server_tcp_prof_default(struct probdata* prob);
int probdata_server_tcp_prof_init(struct probdata* prob);
int probdata_server_tcp_prof_delete(struct probdata* prob);
int probdata_server_tcp_fastprof_run(struct probdata* prob);
int probdata_server_tcp_prof_reset(struct probdata *prob);
void* probdata_server_tcp_fastprof_worker(void *prob);
void probdata_server_tcp_fastprof_cleanup_handler(void *prob);

int probdata_server_udt_prof_new(struct probdata* prob);
int probdata_server_udt_prof_default(struct probdata* prob);
int probdata_server_udt_prof_init(struct probdata* prob);
int probdata_server_udt_fastprof_run(struct probdata* prob);
int probdata_server_udt_prof_reset(struct probdata* prob);
void* probdata_server_udt_fastprof_worker(void* prob);
void probdata_server_udt_fastprof_cleanup_handler(void* prob);

// drivers of XDD - to be added
int probdata_server_dw_xdd_tcp(struct probdata* prob);
int probdata_server_dw_xdd_udt(struct probdata* prob);

#endif
