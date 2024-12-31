/***************************************************************************
 * FastProf: A Stochastic Approximation-based Transport Profiler
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _FASTPROF_H_
#define _FASTPROF_H_

#include <cstdio>
#include <cstdlib>
#include <ctime>
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
#ifdef __cplusplus
extern "C"{
#endif 
    #include <iperf_api.h>
#ifdef __cplusplus
}
#endif
#include <tpg.h>
#include "fastprof_error.h"
#include "fastprof_log.h"

#define F_MAX_ERROR_NUM 5
#define F_KB 1000
#define F_MB (1000 * 1000)
#define F_GB (1000 * 1000 * 1000)
#define F_BUF_MAX (2 * F_GB) // 2GB
#define F_BUF_MIN (16 * F_KB) // 16K
#define F_BLK_MAX 25
#define F_BLK_MIN 1
#define F_ITERATIVE_MIN 1.0
#define F_ITERATIVE_MAX 25.0
#define F_STREAM_NUM_MIN 1
#define F_STREAM_NUM_MAX 25
#define F_MAX_LIMIT 60
#define F_IMPEDED_LIMIT 30
#define F_DEFAULT_PGR 0.75
#define F_ONE_THOUSAND 1000
#define F_HOSTADDR_LEN 64
#define F_DEFAULT_DURATION 10
#define F_DEFAULT_M2M_FOLDER  "../../profile/mem"

#define MEM_RECORD_FORMAT_LABEL \
	"%s %s %s %s %s %d %d %d %s %d %s %d %s %lld %s %lf %s"
#define MEM_RECORD_NUM_ELEMENTS 17

struct probdata;

enum f_program_role
{
    F_UNKNOWN,
    F_SERVER,
    F_CLIENT
};

enum f_protocol 
{
    F_PTCP,
    F_PUDT,
    F_PUDP,
    F_PUNKNOWN
};


struct fastprof_ctrl
{
    f_program_role role;
    char local_ip[F_HOSTADDR_LEN];
    char remote_ip[F_HOSTADDR_LEN];
    int tcp_port;
    int udt_port;
    int host_max_buf;
    double bw_bps;
    double rtt_ms;
    int mss;
    double A_val;
    double a_val;
    double a_scale;
    double c_val;
    double c_scale;
    double alpha;
    double gamma;
    double pgr;
    int impeded_limit;
    int max_limit;
    int duration;
}__attribute__ ((packed));

struct fastprof_tcp_prof
{
    struct iperf_test* tcp_prof;
    long long int total_sent;
    double perf_Mbps;
};

struct fastprof_udt_prof
{
    struct tpg_profile* udt_prof;
    double perf_Mbps;
};

struct fastprof_tcp_trace
{
    double buf_size;
    double stream_num;
    char marker;
    double perf_Mbps;
    int run_or_not;
}__attribute__ ((packed));

struct fastprof_udt_trace
{
    double blk_size;
    double buf_size;
    char marker;
    double perf_Mbps;
    int run_or_not;
}__attribute__ ((packed));

int f_ctrl_default(struct fastprof_ctrl* ctrl);
int f_ctrl_set_addr(struct fastprof_ctrl* ctrl, char* addr);
double f_perturb();
double f_round(double x);
double f_rand(int x, int y);

int f_get_currtime_str(char* tm_str);

struct mem_record
{
	char src_ip[32];
	char dst_ip[32];
	char curr_time[32];
	char protocol_name[32];
	char toolkit_name[32];
	int num_streams;
	int mss_size;
	int buffer_size;
	char buffer_size_unit[8];
	int block_size;
	char block_size_unit[8];
	int duration;
	char duration_unit[8];
	long long int trans_size;
	char trans_size_unit[8];
	double average_perf;
	char average_perf_unit[8];
}__attribute__ ((packed));


struct disk_record
{
	char src_ip[32];
	char dst_ip[32];
	char curr_time[32];
	char protocol_name[32];
	char toolkit_name[32];
	char filesystem_name[32];
	int num_streams;
	int mss_size;
	int buffer_size;
	char buffer_size_unit[8];
	int block_size;
	char block_size_unit[8];
	int duration;
	char duration_unit[8];
	long long int trans_size;
	char trans_size_unit[8];
	double average_perf;
	char* average_perf_unit[8];
}__attribute__ ((packed));

int f_m2m_db_search(FILE* f, struct mem_record* record_max, struct fastprof_ctrl* ctrl);
int f_print_m2m_record(struct mem_record *record);


double f_estimate_rtt(char* hostaddr);

#endif
