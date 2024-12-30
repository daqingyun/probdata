/***************************************************************************
 * ProbData - PRofiling Optimization Based DAta Transfer Advisor
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _PROBDATA_H_
#define _PROBDATA_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <pthread.h>
#include "../fastprof/fastprof.h"
#include "probdata_log.h"
#include "probdata_error.h"
#include "probdata_client.h"
#include "probdata_server.h"

// status code for probdata
#define P_PROGRAM_START 1
#define P_CTRL_CHAN_EST 2
#define P_TWOWAY_HANDSHAKE 3
#define P_RUN_M2M_DB 4
#define P_RUN_M2M_TCP 5
#define P_RUN_M2M_UDT 6
#define P_RUN_D2D_DB 7
#define P_RUN_D2D_TCP 8
#define P_RUN_D2D_UDT 9
#define P_PROGRAM_DONE 10
#define P_SERVER_ERROR (-20)
#define P_CLIENT_TERMINATE (-21)
#define P_ACCESS_DENIED (-22)

// constants for probdata
#define P_KB  1000
#define P_MB  (1000 * 1000)
#define P_GB  (1000 * 1000 * 1000)
#define P_FILENAME_LEN 128
#define P_HOSTADDR_LEN 64
#define P_MAX_BACKLOG 10
#define P_IDPKT_SIZE 32
#define P_MAX_ERROR_NUM 10
#define P_DEFAULT_DURATION 10
#define P_DEFAULT_REPT_INTERVAL 5
#define P_DEFAULT_STAT_INTERVAL 5
#define P_DEFAULT_PACKET 1500
#define P_DEFAULT_BW 10.0
#define P_DEFAULT_PORT 7001
#define P_DEFAULT_TCP_PORT 5020
#define P_DEFAULT_UDT_PORT 5005
#define P_DEFAULT_SPSA_BIG_A_VAL 0.0
// #define P_DEFAULT_SPSA_SMALL_A_VAL 30.0
// #define P_DEFAULT_SPSA_SMALL_C_VAL 9.5
#define P_DEFAULT_SPSA_SMALL_A_VAL 15.0
#define P_DEFAULT_SPSA_SMALL_C_VAL 5.50
#define P_DEFAULT_SCALE_FACTOR_A_VAL 1.25
#define P_DEFAULT_SCALE_FACTOR_C_VAL 1.15
#define P_DEFAULT_SPSA_ALPHA 0.602
#define P_DEFAULT_SPSA_GAMMA 0.101
#define P_DEFAULT_PGR 0.75
// #define P_DEFAULT_MAX_PROF_LIMIT INT_MAX
#define P_DEFAULT_MAX_PROF_LIMIT 60
#define P_DEFAULT_IMPEDED_LIMIT 30
#define P_DEFAULT_DST_FILE "/tmp/probdata_tmp.dat"
#define P_PROGRAM_VERSION "1.0"
#define P_PROGRAM_VERSION_DATE "November 20, 2019"
#define P_BUG_REPORT_EMAIL  "daqingyun@gmail.com"

extern const char  p_program_version[];
extern const char  p_bug_report_contact[];
extern const char  p_usage_shortstr[];
extern const char  p_usage_longstr[];

#define PROBDATA_DEBUG

enum p_program_role
{
    P_UNKNOWN,
    P_SERVER,
    P_CLIENT
};

struct probdata
{
    signed char status;
    p_program_role role;
    int listen_sock;
    int accept_sock;
    int max_sock;
    fd_set read_sock_set;
    int ctrl_port;
    int tcp_port;
    int udt_port;
    char local_ip[P_HOSTADDR_LEN];
    char remote_ip[P_HOSTADDR_LEN];
    char src_file[P_FILENAME_LEN];
    char dst_file[P_FILENAME_LEN];
    char id[P_IDPKT_SIZE];
    int (*instance)(struct probdata*);
    double max_perf_mbps;
    long long int start_time_us;
    long long int end_time_us;
    int mss;
    double rtt_ms;
    int prof_duration;
    int max_prof_limit;
    int impeded_limit;
    double pgr;
    int do_udt_only;
    int host_max_buf;
    double bandwidth;
    
    double spsa_A_val;
    double spsa_a_val;
    double spsa_a_scale;
    double spsa_c_val;
    double spsa_c_scale;
    double spsa_alpha_val;
    double spsa_gamma_val;
    struct fastprof_udt_prof* prob_udt_prof;
    struct fastprof_udt_prof* prob_udt_prof_max;
    struct fastprof_tcp_prof* prob_tcp_prof;
    struct fastprof_tcp_prof* prob_tcp_prof_max;
    struct fastprof_ctrl* prob_control;
    struct fastprof_ctrl* prob_control_max;
    pthread_t* prob_udt_worker;
    pthread_t* prob_tcp_worker;

    FILE* db_file_handler;
    struct mem_record *m2m_record, *m2m_record_max;
    struct disk_record d2d_record, *d2d_record_max;
};

int probdata_new_and_default(struct probdata* prob);
int probdata_delete(struct probdata* prob);
int probdata_load_conf(struct probdata* prob);
int probdata_set_role(struct probdata* prob, p_program_role role);
int probdata_set_local_addr(probdata* prob, char* addr);
int probdata_set_peer_addr(struct probdata* prob, char* addr);
int probdata_set_src_file(struct probdata* prob, char* src_filename);
int probdata_set_dst_file(struct probdata* prob, char* dst_filename);
int probdata_cmd_parse(struct probdata* prob, int argc, char** argv);
int probdata_get_max_buf(struct probdata* prob);
int probdata_set_control(struct probdata* prob);
int probdata_print_short_usage();
int probdata_print_long_usage();

#endif
