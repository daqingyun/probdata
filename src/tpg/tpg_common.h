/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _TPG_COMMON_H_
#define _TPG_COMMON_H_

#include <cstdio>
#include <cstdlib>
#include <error.h>
#include <string>
#include <cstring>
#include <climits>
#include <cerrno>
#include <cassert>
#include <ctime>
#include <sys/timeb.h>
#include <sys/time.h>
#include <udt.h>

#define	T_ONE_MILLION 1000000
#define	T_THOUSAND 1000
#define	T_BILLION 1.0e9
#define	T_MAX_ERROR_NUM 5
#define T_DEFAULT_PORT 6001
#define T_DEFAULT_UDT_PORT 5005
#define T_DEFAULT_TCP_PORT 5025
#define T_REPT_INTERVAL 1.0
#define T_CTRL_BLOCK_SIZE 32
#define T_MAX_BACKLOG 10
#define T_IDPKT_SIZE 32
#define T_KB 1000
#define T_MB (1000 * 1000)
#define T_GB (1000 * 1000 * 1000)
#define T_MAX_TCP_BUFFER (INT_MAX)
#define T_MAX_UDT_BUFFER (INT_MAX)
#define T_MAX_UDP_BUFFER (INT_MAX)
#define T_MAX_BLOCKSIZE T_MB
#define T_MIN_BLOCKSIZE 4 // "NML" + '\0'
#define T_MAX_MSS (9 * 1024)
#define T_MAX_STREAM_NUM 128
#define T_MAX_DURATION 3600
#define T_DEFAULT_DURATION 10
#define T_MIN_DURATION 1
#define T_CONF_FILENAME   "../conf/tpg.conf"
#define T_LOG_DIR ".\\tpglog"
#define T_PTCP 10
#define T_PUDP 11
#define T_PUDT 12
#define T_DEFAULT_UDP_BLKSIZE 8192
#define T_DEFAULT_TCP_BLKSIZE (32 * 1000)
#define T_DEFAULT_UDT_BLKSIZE 64000 // 64K
#define T_HOSTADDR_LEN 64
#define T_STRNAME_LEN 8

// TPG states
#define T_PROGRAM_START 9
#define T_TWOWAY_HANDSHAKE 10
#define T_CTRLCHAN_EST 11
#define T_CREATE_DATA_STREAMS 12
#define T_PROFILING_START 13
#define T_PROFILING_RUNNING   14
#define T_PROFILING_END   15
#define T_EXCHANGE_PROFILE    16
#define T_PROGRAM_DONE    17
#define T_SERVER_ERROR    (-20)
#define T_ACCESS_DENIED   (-21)
#define T_SERVER_TERM 18
#define T_CLIENT_TERM 19
#define T_PROGRAM_VERSION "3.0"
#define T_PROGRAM_VERSION_DATE "Nov 20, 2016"
#define T_BUG_REPORT_EMAIL "daqingyun@gmail.com"

enum
{
    T_ERR_NONE = 0,
    T_ERR_UNKNOWN = 300,
    T_ERR_SERV_CLIENT = 1,
    T_ERR_NO_ROLE = 2,
    T_ERR_SERVER_ONLY = 3,
    T_ERR_CLIENT_ONLY = 4,
    T_ERR_DURATION = 5,
    T_ERR_NUM_STREAMS = 6,
    T_ERR_BLOCK_SIZE = 7,
    T_ERR_BUF_SIZE = 8,
    T_ERR_INTERVAL = 9,
    T_ERR_MSS = 10,
    T_ERR_NO_SENDFILE = 11,
    T_ERR_OMIT = 12,
    T_ERR_NO_IMP = 13,
    T_ERR_FILE = 14,
    T_ERR_BURST = 15,
    T_ERR_END_CONDITIONS = 16,
    T_ERR_MULTI_NIC = 17,
    T_ERR_BW_NEEDED = 18,
    T_ERR_BIND_CONFLICT = 19,
    
    T_ERR_NEW_TEST = 100,
    T_ERR_INIT_PROFILE = 101,
    T_ERR_LISTEN = 102,
    T_ERR_CONNECT = 103,
    T_ERR_ACCEPT = 104,
    T_ERR_SEND_COOKIE = 105,
    T_ERR_RECV_COOKIE = 106,
    T_ERR_CTRL_WRITE = 107,
    T_ERR_CTRL_READ = 108,
    T_ERR_CTRL_CLOSE = 109,
    T_ERR_MESSAGE = 110,
    T_ERR_SEND_MESSAGE = 111,
    T_ERR_RECV_MESSAGE = 112,
    T_ERR_SEND_PARAMS = 113,
    T_ERR_RECV_PARAMS = 114,
    T_ERR_PACKAGE_RESULTS = 115,
    T_ERR_SEND_RESULTS = 116,
    T_ERR_RECV_RESULTS = 117,
    T_ERR_SELECT = 118,
    T_ERR_CLIENT_TERM = 119,
    T_ERR_SERVER_TERM = 120,
    T_ERR_ACCESS_DENIED = 121,
    T_ERR_SET_NODELAY = 122,
    T_ERR_SET_MSS = 123,
    T_ERR_SET_BUF = 124,
    T_ERR_SET_TOS = 125,
    T_ERR_SET_COS = 126,
    T_ERR_SET_FLOW = 127,
    T_ERR_REUSEADDR = 128,
    T_ERR_NON_BLOCKING = 129,
    T_ERR_SET_WIN_SIZE = 130,
    T_ERR_PROTOCOL = 131,
    T_ERR_AFFINITY = 132,
    T_ERR_DAEMON = 133,
    T_ERR_SET_CONGESTION = 134,
    
    // 
    T_ERR_CREATE_STREAM = 200,
    T_ERR_INIT_STREAM = 201,
    T_ERR_STREAM_LISTEN = 202,
    T_ERR_STREAM_CONNECT = 203,
    T_ERR_STREAM_ACCEPT = 204,
    T_ERR_STREAM_WRITE = 205,
    T_ERR_STREAM_READ = 206,
    T_ERR_STREAM_CLOSE = 207,
    T_ERR_STREAM_INIT = 208,
    T_ERR_UDT_ERROR = 209,
    T_ERR_THREAD_ERROR = 210,
    T_ERR_UDT_SEND_ERROR = 211,
    T_ERR_UDT_RECV_ERROR = 212,
    T_ERR_UDT_OPT_ERROR = 213,
    T_ERR_UDT_PERF_ERROR = 214,
    T_ERR_CLIENT_PACK = 215,
    T_ERR_SOCK_CLOSE_ERROR = 216,
    
    //
    T_ERR_PHY_CONN_INIT = 217,
    
    //
    T_ERR_DSTFILE_ONLY = 218,
    T_ERR_DISABLED_PROBDATA = 219,
};

const char *t_errstr(int);
extern int  t_errno;
extern const char   tpg_program_version[];
extern const char   tpg_bug_report_contact[];
extern const char   tpg_usage_shortstr[];
extern const char   tpg_usage_longstr[];
extern const char*  tpg_byte_label[];
extern const char*  tpg_bit_label[];
void t_print_long_usage();
void t_print_short_usage();
const char* t_status_to_str(signed char tpgstate);
double  t_unit_atof(const char *str);
int t_unit_atoi(const char *str);
long long int t_get_currtime_ms();
long long int t_get_currtime_us();
void t_unit_format(char* s, int len, double bytes, char flag);

#endif
