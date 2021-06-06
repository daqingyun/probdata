/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _TPG_UDP_H_
#define _TPG_UDP_H_

#include "tpg_profile.h"

int udp_profile(tpg_profile *);
int udp_profile(tpg_profile *);
int udp_init(tpg_profile *);
int udp_listen(tpg_profile *);
int udp_accept(tpg_profile *, int);
int udp_connect(tpg_stream *);
int udp_send(tpg_stream *);
int udp_recv(tpg_stream *);
int udp_close(int* sockfd);
const char* udp_strerror();

#endif