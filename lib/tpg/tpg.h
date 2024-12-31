/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _TPG_API_H_
#define _TPG_API_H_

#include <vector>
#include <bitset>

// #include "tpg_common.h"
#define T_HOSTADDR_LEN 64
#define T_IDPKT_SIZE 32
#define T_MAX_STREAM_NUM 128
#define T_PUDT 12
extern int t_errno;
const char *t_errstr(int);
void t_print_short_usage();
void t_print_long_usage();
long long int t_get_currtime_us();

// #include "tpg_conn.h"
struct tpg_nic2nic;

// #include "tpg_protocol.h"
struct tpg_protocol;

// #include "tpg_stream.h"
struct tpg_stream;

// #include "tpg_profile.h"
enum tpg_program_role { T_UNKNOWN, T_SERVER, T_CLIENT };
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
int set_protocol_id(tpg_profile*, int);
int set_role(tpg_profile*, tpg_program_role);
int set_hostname(tpg_profile*, char*);
int tpg_prof_default(tpg_profile*);
int tpg_prof_start_timers(tpg_profile*);
int parse_cmd_line(tpg_profile*, int , char **);

// #include "tpg_client.h"
int tpg_client_run(tpg_profile*);
int tpg_client_clean_up(tpg_profile*);
int tpg_client_reset(tpg_profile*);

// #include "tpg_server.h"
int tpg_server_run(tpg_profile*, int, int);
int tpg_server_clean_up(tpg_profile*);
int tpg_server_reset(tpg_profile*);

#endif