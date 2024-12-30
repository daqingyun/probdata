/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _TPG_PROFILE_H_
#define _TPG_PROFILE_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>
#include <cstring>
#include <bitset>
#include <stdarg.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/types.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/stat.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <pthread.h>
#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif
#ifndef __USE_GNU
    #define __USE_GNU
#endif

#include <sysexits.h>
#include <stdint.h>
#include <error.h>
#include <sched.h>
#include "tpg_common.h"
#include "tpg_conn.h"
#include "tpg_stream.h"
#include "tpg_protocol.h"

//#define ANL_UCHICAGO

enum tpg_program_role
{
    T_UNKNOWN,
    T_SERVER,
    T_CLIENT
};

struct tpg_profile
{
    signed char status;
    pthread_mutex_t client_finish_lock;
    std::bitset<T_MAX_STREAM_NUM> client_finish_mark;
    pthread_mutex_t server_finish_lock;
    int server_finish_mark;
    pthread_t*  send_workers;
    pthread_t*  recv_workers;
    pthread_t   perf_mon;
    char id_buf[T_IDPKT_SIZE];
    long long int start_time_us;
    long long int interval_time_us;
    long long int end_time_us;
    int ctrl_listen_sock;
    int ctrl_listen_port;
    int ctrl_accept_sock;
    int protocol_listen_sock;
    int protocol_listen_port;
    int ctrl_max_sock;
    fd_set  ctrl_read_sockset;
    std::vector<tpg_protocol*>  protocol_list;
    std::vector<tpg_stream*>    stream_list;
    std::vector<tpg_nic2nic *>  connection_list;
    
    tpg_program_role role;
    int is_sender;
    int tcp_mss;
    int tcp_sock_bufsize;
    int udt_send_bufsize;
    int udt_recv_bufsize;
    int udp_send_bufsize;
    int udp_recv_bufsize;
    int udt_mss;
    double udt_maxbw;
    int blocksize;
    char remote_ip[T_HOSTADDR_LEN];
    char local_ip[T_HOSTADDR_LEN];
    int local_port;
    int prof_time_duration;
    int is_multi_nic_enabled;
    int total_stream_num;
    int accept_stream_num;
    int interval_report_flag;
    int server_report_flag;
    int cpu_affinity_flag;
    double report_interval;
    tpg_protocol* selected_protocol;
    long long int total_sent_bytes;
    double client_perf_mbps;
    double server_perf_mbps;
    
    int fastprof_enable_flag;
    double fastprof_bandwidth;
    double fastprof_rtt_delay;
    double fastprof_A_value; // not yet implemented
    double fastprof_a_value; // not yet implemented
    double fastprof_c_value; // not yet implemented
    double fastprof_perf_gain_ratio;
    int fastprof_no_imp_limit;
    int fastprof_max_prof_limit;
    double fastprof_block_size_min;
    double fastprof_block_size_max;
    double fastprof_block_size_unit;
    double fastprof_block_resolution;
    double fastprof_buffer_size_min;
    double fastprof_buffer_size_max;
    double fastprof_buffer_resolution;
    double fastprof_buffer_size_unit;
};


int tpg_prof_default(tpg_profile* prof);
int tpg_prof_start_timers(tpg_profile* prof);
int set_protocol_id(tpg_profile* prof, int protid);
int set_role(tpg_profile* prof, tpg_program_role role);
int set_hostname(tpg_profile* prof, char* hostname);
int set_srcfile(tpg_profile* prof, char* src_filename);
int set_dstfile(tpg_profile* prof, char* dst_filename);
int set_bindaddr(tpg_profile* prof, char* hostname);
void print_sum_perf(tpg_profile *);
void print_partial_perf(tpg_profile* prof);
int parse_cmd_line(tpg_profile *prof, int argc, char **argv);
unsigned long long int record_curr_tm_usec();
double perturb();
double rounding(double x);
double rand_between(int x, int y);
int generate_profile_filename(char* filename, int no_imp_limit, int max_prof_limit, int perf_gain_ratio_int, const char* enable_disable);

#endif
