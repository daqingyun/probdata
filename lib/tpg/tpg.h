/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _TPG_API_H_
#define _TPG_API_H_

#include <vector>
#include <bitset>


enum { T_PTCP = 10, T_PUDP = 11, T_PUDT = 12 };
extern int t_errno;
const char *t_errstr(int);
void t_print_short_usage();
void t_print_long_usage();

enum tpg_program_role { T_UNKNOWN, T_SERVER, T_CLIENT };
struct tpg_profile;
int set_protocol_id(tpg_profile*, int);
int set_role(tpg_profile*, tpg_program_role);
int set_hostname(tpg_profile*, char*);
int tpg_prof_default(tpg_profile*);
int parse_cmd_line(tpg_profile*, int , char **);

/* getters and setter wrapper */
tpg_profile* tpg_new_profile();
void tpg_delete_profile(tpg_profile*);
tpg_program_role tpg_get_role(tpg_profile*);
char* tpg_get_local_ip(tpg_profile*);
char* tpg_get_remote_ip(tpg_profile*);
int tpg_get_ctrl_listen_port(tpg_profile*);
void tpg_set_ctrl_listen_port(tpg_profile*, int);
int tpg_get_blocksize(tpg_profile*);
void tpg_set_blocksize(tpg_profile*, int);
int tpg_get_udt_mss(tpg_profile*);
void tpg_set_udt_mss(tpg_profile*, int);
int tpg_get_udt_send_bufsize(tpg_profile*);
int tpg_get_total_stream_num(tpg_profile*);
void tpg_set_udt_send_bufsize(tpg_profile*, int);
void tpg_set_udt_recv_bufsize(tpg_profile*, int);
void tpg_set_udp_send_bufsize(tpg_profile*, int);
void tpg_set_udp_recv_bufsize(tpg_profile*, int);
double tpg_get_client_perf_mbps(tpg_profile*);
void tpg_set_client_perf_mbps(tpg_profile*, double);
void tpg_set_server_perf_mbps(tpg_profile*, double);
long long int tpg_get_total_sent_bytes(tpg_profile*);
int tpg_get_prof_time_duration(tpg_profile*);
void tpg_set_prof_time_duration(tpg_profile*, int);

/* placeholders */
double tpg_get_fastprof_A_value(tpg_profile*);
double tpg_get_fastprof_c_value(tpg_profile*);
double tpg_get_fastprof_a_value(tpg_profile*);
double tpg_get_fastprof_rtt_delay(tpg_profile*);
double tpg_get_fastprof_perf_gain_ratio(tpg_profile*);
double tpg_get_fastprof_bandwidth(tpg_profile*);
int tpg_get_fastprof_max_prof_limit(tpg_profile*);
int tpg_get_fastprof_no_imp_limit(tpg_profile*);

int tpg_client_run(tpg_profile*);
int tpg_client_clean_up(tpg_profile*);
int tpg_client_reset(tpg_profile*);

int tpg_server_run(tpg_profile*, int, int);
int tpg_server_clean_up(tpg_profile*);
int tpg_server_reset(tpg_profile*);

#endif