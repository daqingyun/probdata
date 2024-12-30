/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _TPG_SERVER_H_
#define _TPG_SERVER_H_

#include "tpg_profile.h"

void tpg_server_sig_handler(int sig);
void tpg_server_sig_end(tpg_profile *prof);
void tpg_server_sig_reg(void (*handler)(int));
int tpg_server_start_prof(tpg_profile *prof);
int tpg_server_load_balance(tpg_profile *prof);
int tpg_server_run(tpg_profile *prof, int tmo_sec, int tmo_usec);
int iperf3_server_run(struct iperf_test *test);
int tpg_server_listen(tpg_profile *prof);
int tpg_server_accept(tpg_profile *prof, int tmo_sec, int tmo_usec);
int tpg_server_poll_ctrl_chan(tpg_profile *prof);
int tpg_server_listen_streams(tpg_profile* prof);
int tpg_server_create_stream(tpg_profile* prof, int sockfd, int stream_id);
int tpg_server_accept_streams(tpg_profile* prof);
int tpg_server_proc_ctrl_packet(tpg_profile *prof);
int tpg_server_clean_up(tpg_profile *prof);
int tpg_server_reset(tpg_profile *prof);
void* tpg_server_recv_worker(void *stream);
int tpg_server_proc_data_packet(tpg_stream *);
void* tpg_server_perfmon(void *prof);

#endif
