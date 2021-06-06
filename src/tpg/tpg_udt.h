/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _TPG_UDT_H_
#define _TPG_UDT_H_

#include "tpg_profile.h"

int udt_init(tpg_profile *);
int udt_listen(tpg_profile *);
int udt_accept(tpg_profile *, int);
int udt_connect(tpg_stream *);
int udt_recvN(int fd, char* buf, int count);
int udt_sendN(int fd, char* buf, int count);
int udt_send(tpg_stream *);
int udt_recv(tpg_stream *);
int udt_close(int* sockfd);
const char* udt_strerror();
void udt_client_perf(tpg_profile*);
void udt_print_header();
void udt_print_header_multi_nic();
void udt_print_shortheader();
void udt_print_interval(tpg_profile*);
void udt_print_interval_stream(tpg_stream *);
void udt_print_sum(tpg_profile*);

#endif
