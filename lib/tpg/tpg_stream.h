/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _TPG_STREAM_H_
#define _TPG_STREAM_H_

#include <cstdio>
#include <cstdlib>

#include "tpg_common.h"
#include "tpg_conn.h"

struct stream_perf
{
    long long int start_time_usec;
    long long int interval_time_usec;
    long long int end_time_usec;
    long long int total_sent_bytes;
    long long int interval_sent_bytes;
    long long int total_recv_bytes;
    long long int interval_recv_bytes;
};

struct tpg_nic2nic;
struct tpg_profile;

struct tpg_stream
{
    tpg_profile* stream_to_profile;
    tpg_nic2nic* stream_to_nic2nic;
    int stream_id;
    int stream_sockfd;
    char stream_src_ip[T_HOSTADDR_LEN];
    int stream_src_port;
    char stream_dst_ip[T_HOSTADDR_LEN];
    int stream_dst_port;
    char* stream_blkbuf;
    int stream_blkbuf_fd;
    int stream_blksize;
    struct stream_perf stream_result;
    int (*stream_recv)(struct tpg_stream *);
    int (*stream_send)(struct tpg_stream *);
};

int tpg_stream_init(tpg_stream* pst);
int tpg_stream_free(tpg_stream* pst);

#endif
