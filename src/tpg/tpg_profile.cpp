/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cmath>
//#include <client.h> // host profiler
//#include <profile.h>
#include <udt.h>
#include "tpg_profile.h"
#include "tpg_log.h"
#include "tpg_udt.h"
#include "tpg_tcp.h"
#include "tpg_udp.h"
#include "tpg_common.h"

#ifndef WIN32
    #include <sys/time.h>
    #include <sys/uio.h>
    #include <pthread.h>
#else
    #include <windows.h>
#endif

int tpg_prof_default(tpg_profile* prof)
{
    tpg_protocol *udt;
    tpg_protocol *tcp;
    tpg_protocol *udp;
    
    prof->prof_time_duration = T_DEFAULT_DURATION;
    prof->report_interval = T_REPT_INTERVAL;
    prof->cpu_affinity_flag = -1;
    prof->is_multi_nic_enabled = -1;
    prof->interval_report_flag = -1;
    prof->server_report_flag = -1;
    prof->ctrl_listen_port = T_DEFAULT_PORT;
    prof->ctrl_listen_sock = -1;
    prof->ctrl_accept_sock = -1;
    prof->ctrl_max_sock = -1;
    prof->protocol_listen_sock = -1;
    prof->protocol_listen_port = T_DEFAULT_UDT_PORT;
    memset(prof->local_ip, '\0', T_HOSTADDR_LEN);
    prof->local_port = -1;
    memset(prof->remote_ip, '\0', T_HOSTADDR_LEN);
    
    pthread_mutex_init(&(prof->client_finish_lock), NULL);
    pthread_mutex_init(&(prof->server_finish_lock), NULL);
    
    pthread_mutex_lock(&(prof->server_finish_lock));
    prof->server_finish_mark = 0;
    pthread_mutex_unlock(&(prof->server_finish_lock));
    
    pthread_mutex_lock(&(prof->client_finish_lock));
    prof->client_finish_mark.set();
    pthread_mutex_unlock(&(prof->client_finish_lock));
    
    prof->start_time_us = 0;
    prof->interval_time_us = 0;
    prof->end_time_us = 0;
    
    prof->total_stream_num = 1;
    prof->accept_stream_num = 0;
    
    prof->blocksize = 0;
    prof->tcp_sock_bufsize = 0;
    prof->udt_mss = 1500;
    prof->tcp_mss = 0; // not support
    prof->udt_send_bufsize = 0;
    prof->udt_recv_bufsize = 0;
    prof->udp_send_bufsize = 0;
    prof->udp_recv_bufsize = 0;
    prof->udt_maxbw = 0.0; // not specified
    prof->client_perf_mbps = 0.0;
    prof->server_perf_mbps = 0.0;
    
    prof->fastprof_enable_flag = -1;
    prof->fastprof_rtt_delay   = -1;
    prof->fastprof_bandwidth   = -1;
    prof->fastprof_perf_gain_ratio = -1;
    prof->fastprof_no_imp_limit = -1;
    prof->fastprof_max_prof_limit = -1;
    
    prof->protocol_list.clear();
    prof->stream_list.clear();
    prof->connection_list.clear();

    //
    tcp = new tpg_protocol;
    if (!tcp){
        return -1;
    }
    memset(tcp, 0, sizeof(tpg_protocol));
    udt = new tpg_protocol;
    if (!udt){
        return -1;
    }
    memset(udt, 0, sizeof(tpg_protocol));
    udp = new tpg_protocol;
    if (!udp) {
        return -1;
    }
    memset(udp, 0, sizeof(tpg_protocol));
    
    tcp->protocol_id = T_PTCP;
    strncpy(tcp->protocol_name, "TCP", strlen("TCP"));
    tcp->protocol_name[strlen("TCP")] = '\0';
    tcp->protocol_accept = tcp_accept;
    tcp->protocol_listen = tcp_listen;
    tcp->protocol_connect = tcp_connect;
    tcp->protocol_send = tcp_send;
    tcp->protocol_recv = tcp_recv;
    tcp->protocol_close = tcp_close;
    tcp->protocol_str_error = tcp_strerror;
    tcp->protocol_init = tcp_init;
    tcp->protocol_client_perfmon = tcp_client_perf;
    prof->protocol_list.push_back(tcp);
    
    udt->protocol_id = T_PUDT;
    strncpy(udt->protocol_name, "UDT", strlen("UDT"));
    udt->protocol_name[strlen("UDT")] = '\0';
    udt->protocol_accept = udt_accept;
    udt->protocol_listen = udt_listen;
    udt->protocol_connect = udt_connect;
    udt->protocol_send = udt_send;
    udt->protocol_recv = udt_recv;
    udt->protocol_close = udt_close;
    udt->protocol_str_error = udt_strerror;
    udt->protocol_init = udt_init;
    udt->protocol_client_perfmon = udt_client_perf;
    prof->protocol_list.push_back(udt);
    
    udp->protocol_id = T_PUDP;
    strncpy(udp->protocol_name, "UDP", strlen("UDP"));
    udp->protocol_name[strlen("UDP")] = '\0';
    udp->protocol_accept = udp_accept;
    udp->protocol_listen = udp_listen;
    udp->protocol_connect = udp_connect;
    udp->protocol_send = udp_send;
    udp->protocol_recv = udp_recv;
    udp->protocol_close = udp_close;
    udp->protocol_str_error = udp_strerror;
    udp->protocol_init = udp_init;
    prof->protocol_list.push_back(udp);
    
    set_protocol_id(prof, T_PTCP);
    
    return 0;
}

int tpg_prof_start_timers(tpg_profile* prof)
{
    std::vector<tpg_stream *>::iterator it;
    for (it = prof->stream_list.begin(); it != prof->stream_list.end(); ++it) {
        (*it)->stream_result.start_time_usec = (*it)->stream_result.interval_time_usec = (*it)->stream_result.end_time_usec = t_get_currtime_us();
    }
    
    prof->start_time_us = prof->end_time_us = prof->interval_time_us = t_get_currtime_us();
    
    return 0;
}


int set_protocol_id(tpg_profile* prof, int protid)
{
    std::vector<tpg_protocol *>::iterator it;
    for (it = prof->protocol_list.begin(); it != prof->protocol_list.end(); ++it) {
        if ((*it)->protocol_id == protid) {
            prof->selected_protocol = (*it);
        }
    }
    return 0;
}

int set_role(tpg_profile* prof, tpg_program_role role)
{
    prof->role = role;
    if (role == T_CLIENT) {
        (prof->is_sender) = 1;
    } else if (role == T_SERVER) {
        (prof->is_sender) = 0;
    }
    return 0;
}

int set_hostname(tpg_profile* prof, char* hostname)
{
    memcpy(prof->remote_ip, hostname, strlen(hostname));
    (prof->remote_ip)[strlen(hostname)] = '\0';
    return 0;
}

// int set_srcfile(tpg_profile* prof, char* src_filename)
// {
// 	memcpy(prof->probdata_src_file, src_filename, strlen(src_filename));
// 	prof->probdata_src_file[strlen(src_filename)] = '\0';
// 
// 	return 0;
// }

// int set_dstfile(tpg_profile* prof, char* dst_filename)
// {
// 	memcpy(prof->probdata_dst_file, dst_filename, strlen(dst_filename));
// 	prof->probdata_dst_file[strlen(dst_filename)] = '\0';
// 
// 	return 0;
// }

int set_bindaddr(tpg_profile* prof, char* hostname)
{
    if (strlen(hostname) > T_HOSTADDR_LEN) {
        return -1;
    }
    memcpy(prof->local_ip, hostname, strlen(hostname));
    (prof->local_ip)[strlen(hostname)] = '\0';
    
    return 0;
}

int parse_cmd_line(tpg_profile *prof, int argc, char **argv)
{
    static struct option longopts[] =
    {
        {"Port", required_argument, NULL, 'p'},
        {"UDT", no_argument, NULL, 't'},
        {"Duration", required_argument, NULL, 'd'},
        {"BlockSize", required_argument, NULL, 'l'},
        {"Interval", required_argument, NULL, 'i'},
        {"Interval Result", no_argument, NULL, 'j'},
        {"Server PerfMon", no_argument, NULL, 'q'},
        {"Version", no_argument, NULL, 'v'},
        {"Server", no_argument, NULL, 's'},
        {"Client", required_argument, NULL, 'c'},
        {"Parallel", required_argument, NULL, 'P'},
        {"Multi-NIC", no_argument, NULL, 'm'},
        
        {"Bind", required_argument, NULL, 'b'},
        {"Affinity", no_argument, NULL, 'a'},
        {"RTT", required_argument, NULL, 'D'},
        {"Set-MSS", required_argument, NULL, 'M'},
        {"Window", required_argument, NULL, 'w'},
        {"UDT_SNDBUF", required_argument, NULL, 'f'},
        {"UDP_SNDBUF", required_argument, NULL, 'F'},
        {"UDT_RCVBUF", required_argument, NULL, 'r'},
        {"UDP_RCVBUF", required_argument, NULL, 'R'},
        {"UDT Bandwidth", required_argument, NULL, 'B'},
        {"Help", no_argument, NULL, 'h'},
        
        //
        {"FastProf", no_argument, NULL, 'x'},
        {"FastProfBW", required_argument, NULL, 'y'},
        {"RTT Delay", required_argument, NULL, 'T'},
        //{"A value", required_argument, NULL, 'A'},
        //{"a value", required_argument, NULL, ''},
        //{"c value", required_argument, NULL, ''},
        {"Perf Gain Ratio", required_argument, NULL, 'C'},
        {"No Imp. Limit", required_argument, NULL, 'L'},
        {"Max Prof Limit", required_argument, NULL, 'N'},
        //{"Profiling Range", required_argument, NULL, 'G'},
        
        //
        //  I will implement them once I get a chance
        {"UDP", no_argument, NULL, 'u'},
        {"Bytes", required_argument, NULL, 'n'},
        {"BlockCount", required_argument, NULL, 'k'},
        {NULL, 0, NULL, 0}
    };
    
    int flag;
    int block_size;
    double temp_value = 0.0;
    int server_flag, client_flag;
    int bind_flag, multi_nic_flag;
    int fastprof_rtt_delay_flag, fastprof_bandwidth_flag;
    // 	int probdata_src_file_flag = 0;
    // 	int probdata_dst_file_flag = 0;
    
    block_size = 0;
    server_flag = client_flag = 0;
    bind_flag = multi_nic_flag = 0;
    fastprof_rtt_delay_flag = fastprof_bandwidth_flag = 0;
    while ( (flag = getopt_long(argc, argv, "p:td:l:i:jqvsc:P:mb:axy:T:C:L:N:M:w:f:F:r:R:B:hun:k:S:V:z", longopts, NULL)) != -1)
    {
        switch (flag)
        {
            case 'p':
            {
                int port1 = -1, port2 = -1;
                int ret = -1;
                ret = sscanf(optarg, "%d:%d", &port1, &port2);
                if (ret == 1 || ret == 2) {
                    prof->ctrl_listen_port = (port1 > 0) ? port1 : T_DEFAULT_TCP_PORT;
                    prof->protocol_listen_port = (port2 > 0) ? port2 : T_DEFAULT_UDT_PORT;
                } else {
                    t_errno = T_ERR_UNKNOWN;
                    return -1;
                }
                break;
            }
            case 'd':
            {
                prof->prof_time_duration = atoi(optarg);
                if (prof->prof_time_duration < T_MIN_DURATION || prof->prof_time_duration > T_MAX_DURATION) {
                    t_errno = T_ERR_DURATION;
                    return -1;
                }
                client_flag = 1;
                break;
            }
            case 'i':
            {
                prof->report_interval = atof(optarg);
                if (prof->report_interval >= T_MAX_DURATION || prof->report_interval < T_MIN_DURATION) {
                    t_errno = T_ERR_INTERVAL;
                    return -1;
                }
                client_flag = 1;
                break;
            }
            case 'j':
            {
                prof->interval_report_flag = 1;
                client_flag = 1;
                break;
            }
            case 'q':
            {
                prof->server_report_flag = 1;
                server_flag = 1;
                break;
            }
            case 'v':
            {
                fprintf(stdout, "%s, please report bugs to %s\n", tpg_program_version, tpg_bug_report_contact);
                system("uname -a");
                exit(0);
            }
            case 's':
            {
                if (prof->role == T_CLIENT) {
                    t_errno = T_ERR_SERV_CLIENT;
                    return -1;
                }
                set_role(prof, T_SERVER);
                break;
            }
            case 'c':
            {
                if (prof->role == T_SERVER) {
                    t_errno = T_ERR_SERV_CLIENT;
                    return -1;
                }
                set_role(prof, T_CLIENT);
                set_hostname(prof, optarg);
                break;
            }
            case 'u':
            {
                set_protocol_id(prof, T_PUDP);
                client_flag = 1;
                break;
            }
            case 't':
            {
                set_protocol_id(prof, T_PUDT);
                client_flag = 1;
                break;
            }
            case 'm':
            {
                prof->is_multi_nic_enabled = 1;
                client_flag = 1;
                multi_nic_flag = 1;
                break;
            }
            case 'B':
            {
                prof->udt_maxbw = t_unit_atof(optarg);
                client_flag = 1;
                break;
            }
            case 'y':
            {
                prof->fastprof_bandwidth = t_unit_atof(optarg);
                fastprof_bandwidth_flag = 1;
                client_flag = 1;
                break;
            }
            case 'l': 
            {
                temp_value = t_unit_atof(optarg);
                if ((temp_value > INT_MAX)|| (temp_value < 0.0)) {
                    t_errno = T_ERR_BLOCK_SIZE;
                    return -1;
                } else {
                    block_size = (int)temp_value;
                }
                client_flag = 1;
                break;
            }
            case 'P':
            {
                prof->total_stream_num = atoi(optarg);
                if (prof->total_stream_num > T_MAX_STREAM_NUM) {
                    t_errno = T_ERR_NUM_STREAMS;
                    return -1;
                }
                client_flag = 1;
                break;
            }
            case 'w':
            {
                prof->tcp_sock_bufsize = t_unit_atoi(optarg);
                if (prof->tcp_sock_bufsize > T_MAX_TCP_BUFFER) {
                    t_errno = T_ERR_BUF_SIZE;
                    return -1;
                }
                client_flag = 1;
                break;
            }
            case 'f':
            {
                temp_value = t_unit_atof(optarg);
                if ((temp_value > INT_MAX) || (temp_value > T_MAX_UDT_BUFFER) || (temp_value < 0.0)) {
                    t_errno = T_ERR_BUF_SIZE;
                    return -1;
                } else {
                    prof->udt_send_bufsize = (int)temp_value;
                }
                client_flag = 1;
                break;
            }
            case 'F':
            {
                temp_value = t_unit_atof(optarg);
                if ((temp_value > INT_MAX) || (temp_value > T_MAX_UDP_BUFFER) || (temp_value < 0.0)) {
                    t_errno = T_ERR_BUF_SIZE;
                    return -1;
                } else {
                    prof->udp_send_bufsize = (int)temp_value;
                }
                client_flag = 1;
                break;
            }
            case 'r':
            {
                temp_value = t_unit_atof(optarg);
                if ((temp_value > INT_MAX) || (temp_value > T_MAX_UDT_BUFFER) || (temp_value < 0.0)) {
                    t_errno = T_ERR_BUF_SIZE;
                    return -1;
                } else {
                    prof->udt_recv_bufsize = (int)temp_value;
                }
                client_flag = 1;
                break;
            }
            case 'R':
            {
                temp_value = t_unit_atof(optarg);
                if ((temp_value > INT_MAX) || (temp_value > T_MAX_UDP_BUFFER) || (temp_value < 0.0)) {
                    t_errno = T_ERR_BUF_SIZE;
                    return -1;
                } else {
                    prof->udp_recv_bufsize = (int)temp_value;
                }
                client_flag = 1;
                break;
            }
            case 'b':
            {
                char* colon =  NULL;
                char ip[T_HOSTADDR_LEN];
                int port = -1;
                memset(ip, '\0', T_HOSTADDR_LEN);
                
                if ( (colon = strchr(optarg, ':')) == NULL ) {
                    t_errno = T_ERR_UNKNOWN;
                    return -1;
                }
                
                // printf("%s\n", colon);
                memcpy(ip, optarg, colon - optarg);
                ip[colon - optarg + 1] = '\0';
                port = atoi(colon+1);
                
                if (strlen(ip) > 0) {
                    if (set_bindaddr(prof, ip) < 0) {
                        t_errno = T_ERR_UNKNOWN;
                        return -1;
                    }
                } else {
                    memset(prof->local_ip, '\0', T_HOSTADDR_LEN);
                }
                if (port > 0) {
                    prof->local_port = port;
                } else if(port == 0) {
                    prof->local_port = port;
                } else {
                    prof->local_port = -1;
                }
                client_flag = 1;
                bind_flag = 1;
                break;
            }
            case 'T':
            {
                prof->fastprof_rtt_delay = t_unit_atof(optarg);
                fastprof_rtt_delay_flag = 1;
                client_flag = 1;
                break;
            }
            case 'C':
            {
                prof->fastprof_perf_gain_ratio = t_unit_atof(optarg);
                client_flag = 1;
                break;
            }
            case 'L':
            {
                prof->fastprof_no_imp_limit = t_unit_atoi(optarg);
                client_flag = 1;
                break;
            }
            case 'N':
            {
                prof->fastprof_max_prof_limit = t_unit_atoi(optarg);
                client_flag = 1;
                break;
            }
            case 'M':
            {
                prof->udt_mss = t_unit_atoi(optarg);
                if (prof->udt_mss > T_MAX_MSS) {
                    t_errno = T_ERR_MSS;
                    return -1;
                }
                prof->tcp_mss = t_unit_atoi(optarg);
                if (prof->tcp_mss > T_MAX_MSS) {
                    t_errno = T_ERR_MSS;
                    return -1;
                }
                client_flag = 1;
                break;
            }
            case 'a':
            {
                prof->cpu_affinity_flag = 1;
                break;
            }
            case 'x':
            {
                prof->fastprof_enable_flag = 1;
                set_role(prof, T_CLIENT);
                break;
            }
            case 'h':
            default:
            {
                t_print_long_usage();
                exit(1);
            }
        }
    }
    
    //
    if (prof->role == T_CLIENT && server_flag) {
        t_errno = T_ERR_SERVER_ONLY;
        return -1;
    }
    if (prof->role == T_SERVER && client_flag) {
        t_errno = T_ERR_CLIENT_ONLY;
        return -1;
    }
    if (block_size == 0) {
        if (prof->selected_protocol->protocol_id == T_PUDP) {
            block_size = T_DEFAULT_UDP_BLKSIZE;
        } else if (prof->selected_protocol->protocol_id == T_PUDT) {
            block_size = T_DEFAULT_UDT_BLKSIZE;
        } else {
            block_size = T_DEFAULT_TCP_BLKSIZE;
        }
    }
    if (block_size <= 3 || block_size > T_MAX_BLOCKSIZE) {
        t_errno = T_ERR_BLOCK_SIZE;
        return -1;
    }
    
    prof->blocksize = block_size;
    
    if ((prof->role != T_CLIENT) && (prof->role != T_SERVER)) {
        t_errno = T_ERR_NO_ROLE;
        return -1;
    }
    
    if ((prof->is_multi_nic_enabled > 0) && (prof->total_stream_num <= 1)) {
        t_errno = T_ERR_MULTI_NIC;
        return -1;
    }
    
    if (bind_flag > 0 && multi_nic_flag > 0) {
        t_errno = T_ERR_BIND_CONFLICT;
        return -1;
    }
    
    if (prof->fastprof_enable_flag > 0)	{
        if ( (fastprof_bandwidth_flag > 0) && (fastprof_rtt_delay_flag > 0) ){
            ;
        } else {
            fprintf(stdout, "Bandwidth (-y) and RTT (-T) may help speed up FastProf\n");
        }
        
        if (prof->udt_mss == 1500) {
            fprintf(stdout, "Jumbo frame may get better performance (current: 1500)\n");
        }
    }
    
    if (prof->report_interval > prof->prof_time_duration) {
        t_errno = T_ERR_INTERVAL;
        return -1;
    }
    return 0;
}

void print_sum_perf(tpg_profile *prof)
{
    int i = 0;
    long long int start_time_us;
    long long int end_time_us;
    double total_sent_bytes = 0.0;
    char total_sent_conv[11];
    double total_stream_sent_bytes = 0.0;
    char total_stream_sent_conv[11];
    double duration = 0.0;
    double goodput_byte_per_sec = 0.0;
    char goodput_conv[11];
    double goodput_stream_byte_per_sec = 0.0;
    char goodput_stream_conv[11];
    
    if (prof->is_multi_nic_enabled > 0) {
        T_LOG(T_PERF, "[ ID]  Elapsed(sec)  TotalSent  Goodput       NIC-to-NIC");
    } else {
        T_LOG(T_PERF, "[ ID]  Elapsed(sec)  TotalSent  Goodput");
    }
    
    start_time_us = prof->start_time_us - prof->start_time_us;
    end_time_us = prof->end_time_us - prof->start_time_us;
    duration = (end_time_us - start_time_us) * 1.0;
    
    for(i = 0; i < prof->accept_stream_num; i++) {
        if (prof->accept_stream_num > 1) {
            if (prof->is_multi_nic_enabled > 0) {
                total_stream_sent_bytes = (prof->stream_list)[i]->stream_result.total_sent_bytes;
                t_unit_format(total_stream_sent_conv, 11, total_stream_sent_bytes, 'A');
                goodput_stream_byte_per_sec = total_stream_sent_bytes / (duration / T_ONE_MILLION);
                t_unit_format(goodput_stream_conv, 11, goodput_stream_byte_per_sec, 'a');
                T_LOG(T_PERF, "[%3d]  %-4lld->%4lld    %s   %s/s    (%s,%s)",
                        (prof->stream_list)[i]->stream_id,
                        start_time_us / T_ONE_MILLION, 
                        end_time_us / T_ONE_MILLION,
                        total_stream_sent_conv,
                        goodput_stream_conv,
                        (prof->stream_list)[i]->stream_src_ip,
                        (prof->stream_list)[i]->stream_dst_ip);
            } else {
                total_stream_sent_bytes = (prof->stream_list)[i]->stream_result.total_sent_bytes;
                t_unit_format(total_stream_sent_conv, 11, total_stream_sent_bytes, 'A');
                goodput_stream_byte_per_sec = total_stream_sent_bytes / (duration / T_ONE_MILLION);
                t_unit_format(goodput_stream_conv, 11, goodput_stream_byte_per_sec, 'a');
                T_LOG(T_PERF, "[%3d]  %-4lld->%4lld    %s   %s/s",
                        (prof->stream_list)[i]->stream_id,
                        start_time_us / T_ONE_MILLION,
                        end_time_us / T_ONE_MILLION,
                        total_stream_sent_conv,
                        goodput_stream_conv);
            }
        }
        total_sent_bytes += (prof->stream_list)[i]->stream_result.total_sent_bytes;
    }
    t_unit_format(total_sent_conv, 11, total_sent_bytes, 'A');
    goodput_byte_per_sec = total_sent_bytes / (duration / T_ONE_MILLION);
    t_unit_format(goodput_conv, 11, goodput_byte_per_sec, 'a');
    
    T_LOG(T_PERF, "[SUM]  %-4lld->%4lld    %s   %s/s", start_time_us / T_ONE_MILLION, end_time_us / T_ONE_MILLION, total_sent_conv, goodput_conv);
}

void print_partial_perf(tpg_profile* prof)
{
    char* profile_id = prof->id_buf;
    char* remote_ip = prof->remote_ip;
    char* local_ip = prof->local_ip;
    int block_size = prof->blocksize;
    int stream_num = prof->accept_stream_num;
    int trans_duration = prof->prof_time_duration;
    double repot_intval = prof->report_interval;
    char trans_protocol[4];
    char multi_nic[4];
    int mss = prof->udt_mss;
    int udt_snd_buf = prof->udt_send_bufsize;
    int udt_rcv_buf = prof->udt_recv_bufsize;
    int udp_snd_buf = prof->udp_send_bufsize;
    int udp_rcv_buf = prof->udp_recv_bufsize;
    double bw = prof->udt_maxbw;
    if (prof->selected_protocol->protocol_id == T_PTCP) {
        strncpy(trans_protocol, "TCP", strlen("TCP"));
        trans_protocol[strlen("TCP")] = '\0';
    } else if(prof->selected_protocol->protocol_id == T_PUDT) {
        strncpy(trans_protocol, "UDT", strlen("UDT"));
        trans_protocol[strlen("UDT")] = '\0';
    } else if(prof->selected_protocol->protocol_id == T_PUDP) {
        strncpy(trans_protocol, "UDP", strlen("UDP"));
        trans_protocol[strlen("UDP")] = '\0';
    } else {
        strncpy(trans_protocol, "ERR", strlen("ERR"));
        trans_protocol[strlen("ERR")] = '\0';
    }
    if (prof->is_multi_nic_enabled > 0) {
        strncpy(multi_nic, "YES", strlen("YES"));
        multi_nic[strlen("YES")] = '\0';
    } else {
        strncpy(multi_nic, "NO", strlen("NO"));
        multi_nic[strlen("NO")] = '\0';
    }
    
    const char test_param_shortstr[] =
    "\nProfile ID:        [%s]\n"
    "Server IP Address: [%s]\n"
    "Client IP Address: [%s]\n"
    "Block Size:        [%d] Bytes\n"
    "Number of Streams: [%d]\n"
    "Transfer Duration: [%d] Seconds\n"
    "Report Interval:   [%0.2lf] Seconds\n"
    "Transport Protocol:[%s]\n"
    "Multi-NIC:         [%s]\n"
    "MSS:               [%d] Bytes\n"
    "UDT SndBufSize:    [%d] Bytes\n"
    "UDT RcvBufSize:    [%d] Bytes\n"
    "UDP SndBufSize:    [%d] Bytes\n"
    "UDP RcvBufSize:    [%d] Bytes\n"
    "BandWidth:         [%0.2lf] Bits/s\n";
    T_LOG(T_PERF,
            test_param_shortstr,
            profile_id,
            remote_ip,
            local_ip,
            block_size,
            stream_num,
            trans_duration,
            repot_intval,
            trans_protocol,
            multi_nic, mss,
            udt_snd_buf,
            udt_rcv_buf,
            udp_snd_buf,
            udp_rcv_buf,
            bw);
}

/*
int tpg_read_ip_pair(tpg_profile *prof)
{
    FILE* fp = NULL;
    char line_in_file[64];
    char* ip_cli = NULL;
    char* ip_srv = NULL;
    int nic_num = 0;
    int i = 0;
    
    if((fp = fopen(T_CONF_FILENAME, "r")) == NULL) {
        T_LOG(T_ERR, "Could not open file %s", T_CONF_FILENAME);
        t_errno = T_ERR_MULTI_NIC;
        return -1;
    }
    
    while (!feof(fp)) {
        memset(line_in_file, '\0', 64);
        fgets(line_in_file, 64, fp);
        
        if (line_in_file[0] == '#'  ||
            line_in_file[0] == '\n' ||
            line_in_file[0] == ' ') {
            continue;
        }
        
        if (strncmp(line_in_file, "CLIENT_NIC_IP <-> SERVER_NIC_IP",
                strlen("CLIENT_NIC_IP <-> SERVER_NIC_IP")) == 0) {
            if (fgets(line_in_file, 64, fp) == NULL) {
                T_LOG(T_ERR, "Could not read file %s", T_CONF_FILENAME);
                t_errno = T_ERR_MULTI_NIC;
                return -1;
            }
            if (sscanf(line_in_file, "%d", &nic_num) != 1) {
                T_LOG(T_ERR, "Format of configuration file is incorrect");
                t_errno = T_ERR_MULTI_NIC;
                return -1;
            }
            for (i = 0; i < nic_num; i++) {
                ip_cli = new char[T_HOSTADDR_LEN];
                ip_srv = new char[T_HOSTADDR_LEN];
                memset(ip_cli, '\0', T_HOSTADDR_LEN);
                memset(ip_srv, '\0', T_HOSTADDR_LEN);
                if (fgets(line_in_file, 64, fp) == NULL) {
                    T_LOG(T_ERR,
                            "Could not read file %s", T_CONF_FILENAME);
                    t_errno = T_ERR_MULTI_NIC;
                    return -1;
                }
                if (sscanf(line_in_file, "%s <-> %s", ip_cli, ip_srv) != 2) {
                    T_LOG(T_ERR, "Format of ip pairs is incorrect");
                    t_errno = T_ERR_MULTI_NIC;
                    return -1;
                } else {
                    T_LOG(T_HINT, "%s %s", ip_cli, ip_srv);
                }
                prof->client_nic_list.push_back(ip_cli);
                prof->server_nic_list.push_back(ip_srv);
            }
        }
    }
    
    fclose(fp);
    return nic_num;
}
*/


unsigned long long int record_curr_tm_usec()
{
#ifndef WIN32
    timeval t;
    gettimeofday(&t, 0);
    return t.tv_sec * 1000000ULL + t.tv_usec;
#else
    LARGE_INTEGER ccf;
    HANDLE hCurThread = ::GetCurrentThread(); 
    DWORD_PTR dwOldMask = ::SetThreadAffinityMask(hCurThread, 1);
    if (QueryPerformanceFrequency(&ccf)) {
        LARGE_INTEGER cc;
        if (QueryPerformanceCounter(&cc)) {
            SetThreadAffinityMask(hCurThread, dwOldMask); 
            return (cc.QuadPart * 1000000ULL / ccf.QuadPart);
        }
    }
    
    SetThreadAffinityMask(hCurThread, dwOldMask); 
    return GetTickCount() * 1000ULL;
#endif
}

double perturb()
{
    return ( ( (double)rand() / (double)RAND_MAX ) > 0.5 ? (1.0) : (-1.0) );
}

double rounding(double x)
{
    return (floor( ((double)(x)) + 0.5 ));
}

double rand_between(int x, int y)
{
    return ( (rand() % (y - x + 1)) + x );
}

int generate_profile_filename(char* filename, int no_imp_limit, int max_prof_limit, int perf_gain_ratio_int, const char* enable_disable)
{
    time_t t;
    struct tm currtime;
    struct stat st = {0};
    int processid;
    
    if (stat("./profiles", &st) == -1) {
        mkdir("./profiles", 0700);
    } else {
        T_LOG(T_HINT, "profiles folder exists");
    }
    
    t = time(NULL);
    currtime = *localtime(&t);
    processid = getpid();
    
    sprintf(filename, "./profiles/%d%02d%02d_%02d%02d%02d_%d_%d-%d-%d-%s.prof",
            currtime.tm_year + 1900,
            currtime.tm_mon + 1,
            currtime.tm_mday,
            currtime.tm_hour,
            currtime.tm_min,
            currtime.tm_sec,
            processid,
            no_imp_limit,
            max_prof_limit,
            perf_gain_ratio_int,
            enable_disable);
    return 0;
}