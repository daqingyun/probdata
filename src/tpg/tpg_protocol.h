/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _TPG_PROTOCOL_H_
#define _TPG_PROTOCOL_H_

#include <cstdio>
#include <cstdlib>

#include "tpg_stream.h"

struct tpg_profile;

struct tpg_protocol
{
    int protocol_id;
    char protocol_name[T_STRNAME_LEN];
    int (*protocol_init)(struct tpg_profile *);
    int (*protocol_listen)(struct tpg_profile *);
    int (*protocol_connect)(struct tpg_stream *);
    int (*protocol_accept)(struct tpg_profile *, int);
    int (*protocol_send)(struct tpg_stream *);
    int (*protocol_recv)(struct tpg_stream *);
    int (*protocol_close)(int* sockfd);
    const char*(*protocol_str_error)();
    void (*protocol_client_perfmon)(tpg_profile *);
    void (*protocol_server_perfmon)(tpg_profile *);
};

#endif
