/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _TPG_CTRL_H_
#define _TPG_CTRL_H_

#include "tpg_profile.h"

struct ctrlmsg
{
    int  prot_id;
    int  blk_size;
    int  stream_num;
    int  affinity;
    int  udt_mss;
    int  is_multi_nic_enabled;
    int  udt_sock_snd_bufsize;
    int  udt_sock_rcv_bufsize;
    int  udp_sock_snd_bufsize;
    int  udp_sock_rcv_bufsize;
    double udt_maxbw;
    
}__attribute__ ((packed));

typedef struct ctrlmsg ctrl_message;


int ctrl_chan_listen(tpg_profile* prof);
int ctrl_chan_connect(tpg_profile* prof);
int ctrl_chan_accept(tpg_profile* prof, struct sockaddr_in* addr, socklen_t* len, int sec, int usec);
int ctrl_chan_close_listen_sock(tpg_profile* prof);
int ctrl_chan_close_accept_sock(tpg_profile* prof);
int ctrl_chan_send_ctrl_packet(tpg_profile *prof, signed char sc_statevar);
int ctrl_chan_recv_Nbytes(int fd, char *buf, int count);
int ctrl_chan_send_Nbytes(int fd, const char *buf, int count);
int ctrl_chan_ctrlmsg_exchange_cli(tpg_profile *prof);
int ctrl_chan_ctrlmsg_exchange_srv(tpg_profile *prof);
int ctrl_chan_ctrlmsg_exchange(tpg_profile *prof);
int ctrl_chan_perf_exchange_send(tpg_profile *prof);
int ctrl_chan_perf_exchange_recv(tpg_profile *prof);
int ctrl_chan_perf_exchange(tpg_profile *prof);

#endif