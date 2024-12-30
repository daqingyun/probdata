/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _TPG_CLIENT_H_
#define _TPG_CLIENT_H_

#ifdef __linux
    #include <sched.h>
#elif __APPLE__
    #define _GNU_SOURCE
    #include <sched.h>
#endif

#include "tpg_profile.h"
#include "tpg_ctrl.h"


void tpg_client_sig_handler(int sig);
void tpg_client_sig_handler_end(tpg_profile *prof);
void tpg_client_sig_reg(void (*handler)(int));

int tpg_client_start_prof(tpg_profile* prof);
int tpg_client_load_balance(tpg_profile* prof);
int tpg_client_end_prof(tpg_profile* prof);
int tpg_client_run(tpg_profile* prof);
int tpg_client_check_id(tpg_profile* prof);
int tpg_client_connect(tpg_profile* prof);
int tpg_client_poll_ctrl_chan(tpg_profile *prof);
int tpg_client_create_connection(tpg_profile* prof);
int tpg_client_read_multi_nic_conf(tpg_profile *prof);
int tpg_client_create_stream(tpg_profile* prof, int stream_id);
int tpg_client_connect_streams(tpg_profile* prof);
int tpg_client_proc_ctrl_packet(tpg_profile* prof);
int tpg_client_clean_up(tpg_profile* prof);
int tpg_client_reset(tpg_profile* prof);
int pack_data_block(tpg_stream *pst, const char* type);
void* tpg_client_send_worker(void *pstream);
void* tpg_client_perfmon(void *prof);

#endif
