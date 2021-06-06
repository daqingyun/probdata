/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _TPG_TCP_H_
#define _TPG_TCP_H_

#include "tpg_profile.h"

int tcp_init(tpg_profile *);
int tcp_listen(tpg_profile *);
int tcp_accept(tpg_profile *, int);
int tcp_connect(tpg_stream *);
int tcp_recvN(int, char*, int);
int tcp_sendN(int, char*, int);
int tcp_send(tpg_stream *);
int tcp_recv(tpg_stream *);
int tcp_close(int* sockfd);
const char* tcp_strerror();
void tcp_client_perf(tpg_profile *);
void tcp_print_header();
void tcp_print_header_multi_nic();
void tcp_print_interval_stream(tpg_stream *);
void tcp_print_interval(tpg_profile *);
void tcp_print_sum(tpg_profile *);

#endif
