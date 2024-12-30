/***************************************************************************
 * ProbData - PRofiling Optimization Based DAta Transfer Advisor
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _PROBDATA_CONTROL_H_
#define _PROBDATA_CONTROL_H_

#include "probdata.h"

struct probdata_ctrlmsg
{
    int do_udt_only;
    int host_max_buf;
}__attribute__ ((packed));


int probdata_ctrl_chan_listen(probdata* prob);
int probdata_ctrl_chan_connect(probdata* prob);
int probdata_ctrl_chan_accept(probdata* prob, struct sockaddr_in* addr, socklen_t* len);
int probdata_ctrl_chan_close_listen_sock(probdata* prob);
int probdata_ctrl_chan_close_accept_sock(probdata* prob);
int probdata_ctrl_chan_send_ctrl_packet(probdata *prob, signed char sc_statevar);
int probdata_ctrl_chan_recv_Nbytes(int fd, char *buf, int count);
int probdata_ctrl_chan_send_Nbytes(int fd, const char *buf, int count);
int probdata_ctrl_chan_ctrlmsg_exchange_cli(probdata *prob);
int probdata_ctrl_chan_ctrlmsg_exchange_srv(probdata *prob);
int probdata_ctrl_chan_ctrlmsg_exchange(probdata *prob);
int probdata_ctrl_chan_perf_exchange_send(probdata *prob);
int probdata_ctrl_chan_perf_exchange_recv(probdata *prob);
int probdata_ctrl_chan_perf_exchange(probdata *prob);
const char* probdata_status_to_str(signed char prob_state);
double probdata_unit_atof(const char *str);
int probdata_unit_atoi(const char *str);

#endif