/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <list>
#include <sys/types.h>
#include <sys/timeb.h>
#include <cerrno>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <udt.h>
#include "tpg_profile.h"
#include "tpg_udt.h"
#include "tpg_tcp.h"
#include "tpg_client.h"
#include "tpg_log.h"

static jmp_buf sigend_jmp_buf;

void tpg_client_sig_reg(void (*handler)(int))
{
    signal(SIGINT, handler);
    
    signal(SIGTERM, handler);
    
    //signal(SIGHUP, handler);
    
    signal(SIGSEGV, handler);
}

void tpg_client_sig_handler(int sig)
{
    longjmp(sigend_jmp_buf, 1);
}

void tpg_client_sig_handler_end(tpg_profile *prof)
{
    prof->status = T_CLIENT_TERM;
    
    if(ctrl_chan_send_ctrl_packet(prof, T_CLIENT_TERM) < 0) {
        T_LOG(T_ERR, "Cannot send CLIENT_TERM");
    }
    
    for (int i=0; i < prof->accept_stream_num; i++) {
        if (prof->selected_protocol->protocol_close( &(prof->stream_list[i]->stream_sockfd) ) < 0) {
            T_LOG(T_ERR, "Cannot close [%s] socket on stream [%d]", prof->selected_protocol->protocol_name, i);
        }
    }
    
    T_LOG(T_PERF, "SIGINT signal is received");
    T_LOG(T_PERF, "Dump accumulated results");
    print_partial_perf(prof);
    T_LOG(T_PERF, "Partial performance measurements");
    print_sum_perf(prof);
    fflush(t_log_file);
    fsync(fileno(t_log_file));
    t_close_log();
    exit(EXIT_FAILURE);
}

int tpg_client_start_prof(tpg_profile *prof)
{
    int i = -1;
    int ret = -1;
    
    prof->send_workers = new pthread_t[prof->accept_stream_num];
    
    T_LOG(T_HINT, "Client is creating working threads...");
    
    T_LOG(T_PERF, "Client starts sending data on [%s] channel(s)", prof->selected_protocol->protocol_name);
    
    prof->start_time_us = t_get_currtime_us();
    
    for(i = 0; i < prof->accept_stream_num; i++) {
        ret = pthread_create(&((prof->send_workers)[i]), NULL, tpg_client_send_worker, ((prof->stream_list)[i]));
        
        if (ret != 0) {
            t_errno = T_ERR_CREATE_STREAM;
            return -1;
        }
    }
    
    if (prof->cpu_affinity_flag > 0) {
        if (tpg_client_load_balance(prof) < 0) {
            T_LOG(T_HINT, "Cannot set thread affinity");
            t_errno = T_ERR_AFFINITY;
            return -1;
        }
    }
    
#if T_AS_TOOLKIT == 1
    T_LOG(T_HINT, "Client is also creating a perf monitoring thread");
    if (pthread_create(&(prof->perf_mon), NULL, tpg_client_perfmon, prof)!=0) {
        t_errno = T_ERR_THREAD_ERROR;
        return -1;
    }
#endif
    return 0;
}

int tpg_client_load_balance(tpg_profile* prof)
{
    int i = 0;
    int cpu_number = 0;
    int cpu_id;
    
    cpu_set_t cpuset;
    cpu_set_t cpuget;
    cpu_number = sysconf(_SC_NPROCESSORS_CONF);

    T_LOG(T_HINT, "Total [%d] CPU cores", cpu_number);

    for(i = 0; i < prof->accept_stream_num; i++) {
        CPU_ZERO(&cpuset);
        cpu_id = i % cpu_number;
        CPU_SET(cpu_id, &cpuset);
        if (pthread_setaffinity_np((prof->send_workers)[i], sizeof(cpu_set_t), &cpuset) < 0) {
            return -1;
        }
        CPU_ZERO(&cpuget);
        if (pthread_getaffinity_np((prof->send_workers)[i], sizeof(cpu_set_t), &cpuget) < 0) {
            return -1;
        }
        
        if (CPU_ISSET(cpu_id, &cpuget) != 0) {
            T_LOG(T_HINT, "Send worker %d ([%llu]) is running on core [%d]", i, (unsigned long long int)(prof->send_workers)[i], cpu_id);
        }
    }
    
    return 0;
}

int tpg_client_end_prof(tpg_profile* prof)
{
    int i = 0;
    double total_sent_bytes = 0.0;
    long long int duration = 0;
    
    prof->end_time_us = t_get_currtime_us();
    
    T_LOG(T_HINT, "Time for sending data is up");
    T_LOG(T_HINT, "Goodput is being calculated...");
    
    for(i = 0; i < (prof->accept_stream_num); i++) {
        total_sent_bytes += ((prof->stream_list)[i])->stream_result.total_sent_bytes;
        T_LOG(T_HINT, "Total %lld bytes sent through stream [%d]", ((prof->stream_list)[i])->stream_result.total_sent_bytes, ((prof->stream_list)[i])->stream_id);
    }
    duration = prof->end_time_us - prof->start_time_us;
    prof->total_sent_bytes = total_sent_bytes;
    prof->client_perf_mbps = 8.0 * total_sent_bytes / duration;
    
    T_LOG(T_HINT, "Client average performance is %.3lf Mb/s", prof->client_perf_mbps);
    
    prof->status = T_PROFILING_END;
    T_LOG(T_HINT, "Client state ---> T_PROFILING_END");
    if (ctrl_chan_send_ctrl_packet(prof, T_PROFILING_END) < 0) {
        t_errno = T_ERR_CTRL_WRITE;
        return -1;
    } else {
        ;
    }
    
    return 0;
}


int tpg_client_proc_ctrl_packet(tpg_profile* prof)
{
    int err;
    
    switch (prof->status)
    {
        case T_CLIENT_TERM:
        {
            prof->status = T_CLIENT_TERM;
            T_LOG(T_HINT, "Client state ---> T_CLIENT_TERM");
            if (ctrl_chan_send_ctrl_packet(prof, T_CLIENT_TERM) < 0) {
                t_errno = T_ERR_CTRL_WRITE;
                return -1;
            } else {
                ;
            }
            break;
        }
        case T_CTRLCHAN_EST:
        {
            prof->status = T_CTRLCHAN_EST;
            T_LOG(T_HINT, "Client state ---> T_CTRLCHAN_EST");
            
            if (ctrl_chan_send_ctrl_packet(prof, T_TWOWAY_HANDSHAKE) < 0) {
                t_errno = T_ERR_CTRL_WRITE;
                return -1;
            } else {
                T_LOG(T_HINT, "Ctrl pkt \'T_TWOWAY_HANDSHAKE\' is sent");
            }
            break;
        }
        case T_TWOWAY_HANDSHAKE:
        {
            prof->status = T_TWOWAY_HANDSHAKE;
            T_LOG(T_HINT, "Client state ---> T_TWOWAY_HANDSHAKE");
            if (ctrl_chan_ctrlmsg_exchange(prof) < 0) {
                T_LOG(T_HINT, "Ctrl parameter exchang fail");
                return -1;
            } else {
                T_LOG(T_HINT, "Ctrl parameter exchanged successfully");
                if (prof->selected_protocol->protocol_id == T_PUDT) {
                    if (UDT::startup() == UDT::ERROR) {
                        T_LOG(T_HINT, "UDT cannot startup");
                        t_errno = T_ERR_UDT_ERROR;
                        return -1;
                    }
                }
            }
            
            if (ctrl_chan_send_ctrl_packet(prof, T_CREATE_DATA_STREAMS)) {
                t_errno = T_ERR_CREATE_STREAM;
                return -1;
            } else {
                T_LOG(T_HINT, "Ctrl pkt \'T_CREATE_DATA_STREAMS\' sent");
            }
            break;
        }
	case T_SERVER_ERROR:
        {
            prof->status = T_SERVER_ERROR;
            T_LOG(T_HINT, "Client state ---> T_SERVER_ERROR");
            
            if (ctrl_chan_recv_Nbytes(prof->ctrl_accept_sock, (char*) &err, sizeof(err)) < 0) {
                t_errno = T_ERR_CTRL_READ;
                return -1;
            } else {
                t_errno = ntohl(err);
                T_LOG(T_PERF, "Server error %s", t_errstr(t_errno));
            }
            
            if (ctrl_chan_recv_Nbytes(prof->ctrl_accept_sock, (char*) &err, sizeof(err)) < 0) {
                t_errno = T_ERR_CTRL_READ;
                return -1;
            } else {
                errno = ntohl(err);
                T_LOG(T_PERF, "Server error %s", strerror(errno));
            }
            
            if(tpg_client_clean_up(prof) < 0) {
                return -1;
            } else {
                T_LOG(T_HINT, "Client is cleaned up");
            }
            
            if (tpg_client_reset(prof) < 0) {
                return -1;
            } else {
                T_LOG(T_HINT, "Client is resett");
            }
            
            return -1;
            break;
        }
	case T_CREATE_DATA_STREAMS:
        {
            prof->status = T_CREATE_DATA_STREAMS;
            T_LOG(T_HINT, "Client state ---> T_CREATE_DATA_STREAMS");
            if(tpg_client_connect_streams(prof) < 0) {
                return -1;
            } else {
                T_LOG(T_HINT, "All streams are created at client");
            }
            if (ctrl_chan_send_ctrl_packet(prof, T_PROFILING_START) < 0) {
                t_errno = T_ERR_CTRL_WRITE;
                return -1;
            } else {
                T_LOG(T_HINT, "Ctrl pkt \'T_PROFILING_START\' is sent");
            }
            break;
        }
        case T_PROFILING_START:
	{
            prof->status = T_PROFILING_START;
            T_LOG(T_HINT, "Client state ---> T_PROFILING_START");
            
            if(tpg_prof_start_timers(prof) < 0) {
                t_errno = T_ERR_INIT_PROFILE;
                return -1;
            } else {
                T_LOG(T_HINT, "Timers are started");
            }
            
            prof->status = T_PROFILING_RUNNING;
            T_LOG(T_HINT, "Client state ---> T_PROFILING_RUNNING");
            
            if(tpg_client_start_prof(prof) < 0) {
                return -1;
            } else {
                T_LOG(T_HINT, "Data are being sent...");
            }
            break;
        }
        case T_PROFILING_END:
        {
            prof->status = T_PROFILING_END;
            T_LOG(T_HINT, "Client state ---> T_PROFILING_END");
            
#if (T_AS_TOOLKIT == 1 || T_FASTPROF == 1)
            print_sum_perf(prof);
#endif
            if (ctrl_chan_send_ctrl_packet(prof, T_EXCHANGE_PROFILE) < 0) {
                t_errno = T_ERR_CTRL_WRITE;
                return -1;
            } else {
                T_LOG(T_HINT, "Ctrl pkt \'T_EXCHANGE_PROFILE\' is sent");
            }
            break;
        }
        case T_EXCHANGE_PROFILE:
        {
            prof->status = T_EXCHANGE_PROFILE;
            T_LOG(T_HINT, "Client ---> T_EXCHANGE_PROFILE");
            
            if (ctrl_chan_perf_exchange(prof) < 0) {
                return -1;
            } else {
                T_LOG(T_HINT, "performance results are exchanged");
                T_LOG(T_HINT, "Client: %.3lf Mb/s", prof->client_perf_mbps);
                T_LOG(T_HINT, "Server: %.3lf Mb/s", prof->server_perf_mbps);
                
                T_LOG(T_PERF, "Client: %.3lf Mb/s", prof->client_perf_mbps);
                T_LOG(T_PERF, "Server: %.3lf Mb/s", prof->server_perf_mbps);
            }
            
            if (ctrl_chan_send_ctrl_packet(prof, T_PROGRAM_DONE) != 0) {
                t_errno = T_ERR_CTRL_WRITE;
                return -1;
            } else {
                T_LOG(T_HINT, "Ctrl pkt \'T_PROGRAM_DONE\' is sent");
                T_LOG(T_HINT, "A One Time Profile is Completed");
                T_LOG(T_HINT, "Convert the Profile into Json in HERE");
            }
            break;
        }
	case T_PROGRAM_DONE:
        {
            prof->status = T_PROGRAM_DONE;
            T_LOG(T_HINT, "Client ---> T_PROGRAM_DONE");
            
            if(tpg_client_clean_up(prof) < 0) {
                return -1;
            } else {
                T_LOG(T_HINT, "Client is cleaned up");
            }
            
            if (tpg_client_reset(prof) < 0) {
                return -1;
            } else {
                T_LOG(T_HINT, "Client is reset");
            }
            break;
        }
        case T_SERVER_TERM:
        {
            prof->status = T_SERVER_TERM;
            T_LOG(T_HINT, "Client ---> T_SERVER_TERM");
            
            t_errno = T_ERR_SERVER_TERM;
            if(tpg_client_clean_up(prof) < 0) {
                return -1;
            } else {
                T_LOG(T_HINT, "Client is cleaned up");
            }
            
            if (tpg_client_reset(prof) < 0) {
                return -1;
            } else {
                T_LOG(T_HINT, "Client is reset");
            }
            return -1;
            break;
        }
        case T_ACCESS_DENIED:
        {
            prof->status = T_ACCESS_DENIED;
            T_LOG(T_HINT, "Client ---> T_ACCESS_DENIED");
            
            t_errno = T_ERR_ACCESS_DENIED;
            T_LOG(T_HINT, "Server denied");
            return -1;
        }
        default:
        {
            t_errno = T_ERR_MESSAGE;
            return -1;
        }
    }
    return 0;
}

int tpg_client_read_multi_nic_conf(tpg_profile *prof)
{
    FILE* fp = NULL;
    char readline_from_file[T_HOSTADDR_LEN];
    char src_ip[T_HOSTADDR_LEN];
    char dst_ip[T_HOSTADDR_LEN];
    int conn_num = 0;
    int i = 0;
    tpg_nic2nic* pconn = NULL;
    
    if((fp = fopen(T_CONF_FILENAME, "r")) == NULL) {
        T_LOG(T_ERR, "Open NIC2NIC conf file %s fail", T_CONF_FILENAME);
        t_errno = T_ERR_MULTI_NIC;
        return -1;
    }
    
    while (!feof(fp)) {
        memset(readline_from_file, '\0', T_HOSTADDR_LEN);
        fgets(readline_from_file, T_HOSTADDR_LEN, fp);
        if (readline_from_file[0] == '#' || readline_from_file[0] == '\n' || readline_from_file[0] == ' ') {
            continue;
        }
        if (strncmp(readline_from_file, "CLIENT_NIC_IP <-> SERVER_NIC_IP", strlen("CLIENT_NIC_IP <-> SERVER_NIC_IP")) == 0) {
            if (fgets(readline_from_file, T_HOSTADDR_LEN, fp) == NULL) {
                T_LOG(T_ERR, "Could not read file %s", T_CONF_FILENAME);
                t_errno = T_ERR_MULTI_NIC;
                return -1;
            }
            
            if (sscanf(readline_from_file, "%d", &conn_num) != 1) {
                T_LOG(T_ERR, "Format of configuration file is incorrect");
                t_errno = T_ERR_MULTI_NIC;
                return -1;
            } else {
                T_LOG(T_HINT, "Total %d NIC-to-NIC connections",conn_num);
            }
            
            for (i = 0; i < conn_num; i++) {
                memset(src_ip, '\0', T_HOSTADDR_LEN);
                memset(dst_ip, '\0', T_HOSTADDR_LEN);
                if (fgets(readline_from_file, T_HOSTADDR_LEN, fp) == NULL) {
                    T_LOG(T_ERR, "Cannot read file %s", T_CONF_FILENAME);
                    t_errno = T_ERR_MULTI_NIC;
                    return -1;
                }
                if (sscanf(readline_from_file,"%s <-> %s",src_ip,dst_ip) != 2) {
                    T_LOG(T_ERR, "Format of ip pairs is incorrect");
                    t_errno = T_ERR_MULTI_NIC;
                    return -1;
                } else {
                    T_LOG(T_HINT, "NIC-to-NIC: %s->%s", src_ip, dst_ip);
                }

                pconn = new tpg_nic2nic;
                pconn->id = i;
                memset(pconn->src_ip, '\0', T_HOSTADDR_LEN);
                memcpy(pconn->src_ip, src_ip, strlen(src_ip));
                (pconn->src_ip)[strlen(src_ip)] = '\0';
                memset(pconn->dst_ip, '\0', T_HOSTADDR_LEN);
                memcpy(pconn->dst_ip, dst_ip, strlen(dst_ip));
                (pconn->dst_ip)[strlen(dst_ip)] = '\0';
                
                prof->connection_list.push_back(pconn);
            }
        }
    }
    fclose(fp);
    return conn_num;
}

int tpg_client_create_connection(tpg_profile* prof)
{
    tpg_nic2nic* pconn;
    if (prof->is_multi_nic_enabled > 0) {
        if (tpg_client_read_multi_nic_conf(prof) <= 0)	{
            T_LOG(T_ERR, "Cannot read NIC-to-NIC configuration file");
            return -1;
        } else {
            return 0;
        }
    } else {
        pconn = new tpg_nic2nic;
        pconn->id = 0;
        memset(pconn->src_ip, '\0', T_HOSTADDR_LEN);
        memcpy(pconn->src_ip, prof->local_ip, strlen(prof->local_ip));
        (pconn->src_ip)[strlen(pconn->src_ip)] = '\0';
        memset(pconn->dst_ip, '\0', T_HOSTADDR_LEN);
        memcpy(pconn->dst_ip, prof->remote_ip, strlen(prof->remote_ip));
        (pconn->dst_ip)[strlen(pconn->dst_ip)] = '\0';
        prof->connection_list.push_back(pconn);
        return 0;
    }
}

int tpg_client_create_stream(tpg_profile* prof, int stream_id)
{
    T_LOG(T_HINT, "Alloc stream with id [%d]", stream_id);
    int i;
    int index;
    int conn_num = -1;
    tpg_stream *pst = NULL;
    char temp[] = "/tmp/tpg.XXXXXX";
    
    if ((pst = new tpg_stream) == NULL) {
        T_LOG(T_ERR, "Cannot create a stream");
        return -1;
    } else {
        T_LOG(T_HINT, "The stream is new()ed");
    }
    if (tpg_stream_init(pst) < 0) {
        t_errno = T_ERR_CREATE_STREAM;
        T_LOG(T_ERR, "Cannot initialize a stream");
        return -1;
    } else {
        T_LOG(T_HINT, "The stream is initialized");
    }
    memset(pst, 0, sizeof(tpg_stream));


    pst->stream_to_profile = prof;
    
    pst->stream_blkbuf_fd = mkstemp(temp);
    
    if (pst->stream_blkbuf_fd == -1) {
        T_LOG(T_ERR, "mkstemp() fail");
        return -1;
    } else {
        T_LOG(T_HINT, "mkstemp() with buf fd [%d]", pst->stream_blkbuf_fd);
    }
    if (unlink(temp) < 0) {
        T_LOG(T_HINT, "unlink() fail");
        return -1;
    }
    if (ftruncate(pst->stream_blkbuf_fd, prof->blocksize) < 0)
    {
        T_LOG(T_HINT, "ftruncate() fail");
        return -1;
    }
    pst->stream_blkbuf = (char *) mmap(NULL, pst->stream_to_profile->blocksize, PROT_READ | PROT_WRITE, MAP_PRIVATE, pst->stream_blkbuf_fd, 0);
    if (pst->stream_blkbuf == MAP_FAILED) {
        T_LOG(T_ERR, "mmap() fail");
        return -1;
    } else {
        T_LOG(T_HINT, "Allocate a [%d] bytes block", pst->stream_to_profile->blocksize);
    }
    
    srand(time(NULL));
    for (i = 0; i < prof->blocksize; ++i) {
        pst->stream_blkbuf[i] = rand();
    }
    
    T_LOG(T_HINT, "Load protocol send/recv callbacks");
    pst->stream_send = prof->selected_protocol->protocol_send;
    pst->stream_recv = prof->selected_protocol->protocol_recv;
    
    pst->stream_id = stream_id;
    pst->stream_blksize = pst->stream_to_profile->blocksize;
    
    conn_num = (int)(prof->connection_list.size());
    T_LOG(T_HINT, "Total connection number %d", conn_num);
    memset(pst->stream_src_ip, '\0', T_HOSTADDR_LEN);
    memcpy(pst->stream_src_ip, (prof->connection_list)[stream_id % conn_num]->src_ip, strlen((prof->connection_list)[stream_id % conn_num]->src_ip));
    index = stream_id % conn_num;
    pst->stream_src_ip[strlen((prof->connection_list)[index]->src_ip)] = '\0';
    
    if (pst->stream_to_profile->local_port == 0) {
        pst->stream_src_port = T_DEFAULT_UDT_PORT + stream_id + 1;
    } else {
        pst->stream_src_port = pst->stream_to_profile->local_port;
    }
    
    memset(pst->stream_dst_ip, '\0', T_HOSTADDR_LEN);
    memcpy(pst->stream_dst_ip, (prof->connection_list)[stream_id % conn_num]->dst_ip, strlen((prof->connection_list)[stream_id % conn_num]->dst_ip));
    index = stream_id % conn_num;
    pst->stream_dst_ip[strlen((prof->connection_list)[index]->dst_ip)] = '\0';
    
    pst->stream_dst_port = pst->stream_to_profile->protocol_listen_port;
    
    pst->stream_to_nic2nic = (prof->connection_list)[stream_id % conn_num];
    T_LOG(T_HINT, "Start timers of all the streams");
    pst->stream_result.start_time_usec = pst->stream_result.interval_time_usec = pst->stream_result.end_time_usec = t_get_currtime_us();
    T_LOG(T_HINT, "Clear up counter for total bytes sent");
    pst->stream_result.total_sent_bytes = pst->stream_result.interval_sent_bytes = pst->stream_result.total_recv_bytes = pst->stream_result.interval_recv_bytes = 0;
    T_LOG(T_HINT, "Stream is initialized - [%s]", pst->stream_to_profile->selected_protocol->protocol_name);
    
    prof->stream_list.push_back(pst);
    T_LOG(T_HINT, "Add stream [%d] into stream list", pst->stream_id);
    T_LOG(T_HINT, "Stream is created with stream id [%d]", pst->stream_id);
    return 0;
}

int tpg_client_connect_streams(tpg_profile* prof)
{
    int i = -1;
    int s = -1;
    T_LOG(T_HINT, "Client is creating data streams");
    if (tpg_client_create_connection(prof) < 0) {
        t_errno = T_ERR_CREATE_STREAM;
        return -1;
    }
    for (i = 0; i < prof->total_stream_num; i++) {
        if (tpg_client_create_stream(prof, i) < 0){
            t_errno = T_ERR_CREATE_STREAM;
            return -1;
        }
    }
    for (i = 0; i < prof->total_stream_num; ++i) {
        s = prof->selected_protocol->protocol_connect((prof->stream_list)[i]);
        if (s < 0) {
            T_LOG(T_HINT, "Stream [%d] connection error", i);
        } else {
            T_LOG(T_HINT, "Stream [%d] is connected with socket [%d]",i,s);
            T_LOG(T_HINT, "Stream [%d] is connected from [%s] to [%s]", i, (prof->stream_list)[i]->stream_src_ip, (prof->stream_list)[i]->stream_dst_ip);
            prof->accept_stream_num ++;
            
            (prof->stream_list)[i]->stream_sockfd = s;
            
            prof->client_finish_mark.set(i, false);
            
            usleep(10000);
        }
    }
    if (prof->accept_stream_num != prof->total_stream_num) {
        if (prof->accept_stream_num == 0) {
            t_errno = T_ERR_CREATE_STREAM;
            return -1;
        } else {
            T_LOG(T_HINT, "Only %d of %d data stream are connected", prof->accept_stream_num, prof->total_stream_num);
        }
    }
    T_LOG(T_HINT, "All streams are created");
    return 0;
}

int tpg_client_connect(tpg_profile* prof)
{
    T_LOG(T_HINT, "Client is connecting to the server");
    
    if (prof->ctrl_accept_sock < 0) {
        if (ctrl_chan_connect(prof) < 0) {
            T_LOG(T_ERR, "Cannot connect to server [%d]", prof->ctrl_accept_sock);
            t_errno = T_ERR_CONNECT;
            return -1;
        } else {
            T_LOG(T_HINT, "Client is connected to server ctrl chan (TCP) - [%s|%d]", prof->remote_ip, prof->ctrl_listen_port);
        }
    }

    if (tpg_client_check_id(prof) < 0) {
        T_LOG(T_HINT, "Client's ID check fail");
        return -1;
    } else {
        T_LOG(T_HINT, "Client's ID check is successful");
        return 0;
    }
    return 0;
}

int tpg_client_poll_ctrl_chan(tpg_profile *prof)
{
    int rselect = -1;
    int rsize = -1;
    struct timeval tv;
    
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
    FD_ZERO(&(prof->ctrl_read_sockset));
    FD_SET(prof->ctrl_accept_sock, &(prof->ctrl_read_sockset));
    if (prof->ctrl_accept_sock > prof->ctrl_max_sock) {
        prof->ctrl_max_sock = prof->ctrl_accept_sock;
    }
    rselect = select(prof->ctrl_max_sock + 1, &(prof->ctrl_read_sockset), NULL, NULL, &tv);
    if (rselect < 0) {
        t_errno = T_ERR_SELECT;
        return -1;
    } else if(rselect == 0) {
        return 0;
    } else if (rselect > 0) {
        if ((prof->ctrl_accept_sock > 0) && FD_ISSET(prof->ctrl_accept_sock, &(prof->ctrl_read_sockset))) {
            rsize = ctrl_chan_recv_Nbytes(prof->ctrl_accept_sock, (char*) &(prof->status), sizeof(signed char));
            if (rsize <= 0) {
                T_LOG(T_HINT, "Cannot read from control channel");
                return -1;
            } else {
                return rsize;
            }
        } else {
            T_LOG(T_ERR, "Invalid TCP socket or not set in read socket set %d", prof->ctrl_accept_sock);
            t_errno = T_ERR_SELECT;
            return -1;
        }
    }
    FD_CLR(prof->ctrl_accept_sock, &(prof->ctrl_read_sockset));
    prof->ctrl_max_sock = -1;
    T_LOG(T_HINT, "A control packet with type [%d|%s] is received", prof->status, t_status_to_str(prof->status));
    return rselect;
}

int tpg_client_run(tpg_profile *prof)
{
    int ret = 0;
    bool profile_state = false;
    T_LOG(T_HINT, "Client starts running...");
    tpg_client_sig_reg(tpg_client_sig_handler);
    if (setjmp(sigend_jmp_buf)) {
        tpg_client_sig_handler_end(prof);
    }
    T_LOG(T_PERF, "%s", tpg_program_version);
#if (T_AS_TOOLKIT == 1 || T_FASTPROF == 1)
    system("uname -a");
#endif
    
    if (tpg_client_connect(prof) < 0) {
        return -1;
    }
    prof->status = T_PROGRAM_START;
    
    prof->start_time_us  = prof->interval_time_us = prof->end_time_us = t_get_currtime_us();
    while (prof->status != T_PROGRAM_DONE) {
        pthread_mutex_lock(&(prof->client_finish_lock));
        profile_state = prof->client_finish_mark.all();
        pthread_mutex_unlock(&(prof->client_finish_lock));
        if (profile_state && (prof->status == T_PROFILING_RUNNING)) {
            if(tpg_client_end_prof(prof) < 0) {
                return -1;
            }
        }
        
        ret = tpg_client_poll_ctrl_chan(prof);
        if (ret < 0) {
            t_errno = T_ERR_CTRL_READ;
            return -1;
        } else if (ret == 0) {
            ;
        } else if (ret > 0)	{
            if (tpg_client_proc_ctrl_packet(prof) < 0) {
                return -1;
            } else {
                T_LOG(T_HINT, "Client processed a control packet %s", t_status_to_str(prof->status));
            }
        }
        
        usleep(100000);
    }
    
    return 0;
}

static void tpg_client_idpkt_gen(char *pkt)
{
    int currpid;
    char hostname[64];
    char temp[128];
    time_t t;
    struct tm currtime;
    
    currpid = getpid();
    srand(time(NULL));
    
    gethostname(hostname, sizeof(hostname));
    t = time(NULL);
    currtime = *localtime(&t);
    sprintf(temp, "%s.%ld.%06ld.%08lx%08lx.%s",
            hostname, (unsigned long int) (currtime.tm_min ^ rand()), 
            (unsigned long int) (currtime.tm_sec ^ rand()),
            (unsigned long int) (rand() ^ currpid), 
            (unsigned long int) (rand() ^ currpid),
            "1234567890123456789012345678901234567890");
    
    strncpy(pkt, temp, T_IDPKT_SIZE);
    pkt[T_IDPKT_SIZE - 1] = '\0';
}


int tpg_client_check_id(tpg_profile* prof)
{
    tpg_client_idpkt_gen(prof->id_buf);
    
    if (ctrl_chan_send_Nbytes(prof->ctrl_accept_sock, prof->id_buf, T_IDPKT_SIZE) < 0) {
        T_LOG(T_HINT, "Cannot sent client id to server");
        t_errno = T_ERR_SEND_COOKIE;
        return -1;
    } else {
        T_LOG(T_HINT, "Client ID check ok");
        return 0;
    }
}

int tpg_client_clean_up(tpg_profile *prof)
{
    int i = 0;
    T_LOG(T_HINT, "Client is cleaning up");
    if (prof->status != T_PROGRAM_DONE && prof->status != T_PROFILING_END){
        T_LOG(T_HINT, "Clean up because errors happen");
        for(i = 0; i < prof->accept_stream_num; i++) {
            if(pthread_join((prof->send_workers)[i], NULL) != 0) {
                T_LOG(T_ERR, "pthread_join() fails on stream [%d]", i);
            }
        }

#if T_AS_TOOLKIT == 1
        if (pthread_join(prof->perf_mon, NULL) != 0) {
            T_LOG(T_ERR, "pthread_join() fails on perf_mon thread");
        }
#endif
    } else {
        T_LOG(T_HINT, "Clean up because profiling is done");
    }
    
    T_LOG(T_HINT, "Destroy mutex...");
    pthread_mutex_destroy(&(prof->client_finish_lock));
    pthread_mutex_destroy(&(prof->server_finish_lock));
    
    T_LOG(T_HINT, "Closing control channel sockets");
    if (ctrl_chan_close_listen_sock(prof) < 0) {
        T_LOG(T_HINT, "Control listen socket close error");
    } else {
        T_LOG(T_HINT, "Control listen socket is closed");
    }
    if (ctrl_chan_close_accept_sock(prof) < 0) {
        T_LOG(T_HINT, "Control accept socket close error");
    } else {
        T_LOG(T_HINT, "Control accept socket is closed");
    }
    
    std::vector<tpg_stream *>::iterator it;
    for (it = prof->stream_list.begin(); it != prof->stream_list.end(); ++it) {
        if (prof->selected_protocol->protocol_close(&((*it)->stream_sockfd))<0){
            T_LOG(T_ERR, "Protocol socket close error %s", prof->selected_protocol->protocol_str_error());
        }
    }
    
    if (prof->selected_protocol->protocol_id == T_PUDT) {
        if (UDT::cleanup() == UDT::ERROR) {
            T_LOG(T_HINT, "UDT cleanup fail");
            t_errno = T_ERR_UDT_ERROR;
            return -1;
        } else {
            T_LOG(T_HINT, "UDT cleanup successfully");
        }
    }
    
    T_LOG(T_HINT, "Deleting data streams...");
    while (!(prof->stream_list).empty()) {
        tpg_stream_free((prof->stream_list).front());
        (prof->stream_list).erase(prof->stream_list.begin());
    }
    prof->stream_list.clear();
    T_LOG(T_HINT, "All data streams are deleted");
    
    T_LOG(T_HINT, "Deleting connection list...");
    while (!(prof->connection_list).empty()) {
        if ((prof->connection_list).front() != NULL) {
            delete [] ((prof->connection_list).front());
            (prof->connection_list).front() = NULL;
        }
        (prof->connection_list).erase((prof->connection_list).begin());
    }
    prof->connection_list.clear();
    T_LOG(T_HINT, "Connection list is deleted");
    
    T_LOG(T_HINT, "Deleting all send workers...");
    delete [] (prof->send_workers);
    prof->send_workers = NULL;
    T_LOG(T_HINT, "All send workers are deleted");
    return 0;
}

int tpg_client_reset(tpg_profile* prof)
{
    T_LOG(T_HINT, "Re-setting parameter values of a tpg_prof* prof");
    
    set_protocol_id(prof, prof->selected_protocol->protocol_id);
    
    T_LOG(T_HINT, "Re-Initializing mutex...");
    pthread_mutex_init(&(prof->client_finish_lock), NULL);
    pthread_mutex_init(&(prof->server_finish_lock), NULL);
    
    T_LOG(T_HINT, "Re-Setting finish markers...");
    pthread_mutex_lock(&(prof->client_finish_lock));
    prof->client_finish_mark.set();
    pthread_mutex_unlock(&(prof->client_finish_lock));
    
    T_LOG(T_HINT, "Clear all timers...");
    prof->start_time_us = 0;
    prof->interval_time_us = 0;
    prof->end_time_us = 0;
    
    prof->total_stream_num = 1;
    
    prof->accept_stream_num = 0;
    
    prof->fastprof_enable_flag = 1;
    prof->ctrl_listen_sock = -1;
    prof->ctrl_accept_sock = -1;
    prof->protocol_listen_sock  = -1;
#ifndef ANL_UCHICAGO
    memset(prof->local_ip, '\0', T_HOSTADDR_LEN);
    prof->local_port = -1;
#endif
    return 0;
}

int pack_data_block(tpg_stream *pst, const char* type)
{
    memcpy(pst->stream_blkbuf, type, strlen(type));
    return 0;
}

void* tpg_client_send_worker(void *pstream)
{
    int ret = -1;
    long long int us_start_time = 0;
    long long int us_end_time = 0;
    long long int sec_duration = 0;
    tpg_stream* pst = NULL;
    
    pst = (tpg_stream *) pstream;
    sec_duration = pst->stream_to_profile->prof_time_duration;
    us_end_time = us_start_time = t_get_currtime_us();
    T_LOG(T_HINT, "Send worker [%lu] on core [%d] is sending on stream [%d]...", (unsigned long int) pthread_self(), sched_getcpu(), pst->stream_id);
    do{
        us_end_time = t_get_currtime_us();
        if ((us_end_time - us_start_time) < (T_ONE_MILLION * sec_duration)) {
            if (pack_data_block(pst, "NML") < 0) {
                t_errno = T_ERR_CLIENT_ONLY;
                break;
            }
            ret = pst->stream_send(pst);
            if(ret <= 0) {
                T_LOG(T_PERF, "Stream [%d] is broken due to %s", pst->stream_id, pst->stream_to_profile->selected_protocol->protocol_str_error());
                
                pst->stream_result.end_time_usec = t_get_currtime_us();
                
                T_LOG(T_PERF, "Stream socket in stream [%d] is closed", pst->stream_id);
                pst->stream_to_profile->selected_protocol->protocol_close(&(pst->stream_sockfd));
                
                pthread_mutex_lock(&(pst->stream_to_profile->client_finish_lock));
                pst->stream_to_profile->client_finish_mark.set(pst->stream_id, true);
                pthread_mutex_unlock(&(pst->stream_to_profile->client_finish_lock));
                break;
            } else {
                pst->stream_to_profile->end_time_us = pst->stream_result.end_time_usec = t_get_currtime_us();
                pst->stream_result.total_sent_bytes += ret;
            }
        } else {
            if (pack_data_block(pst, "TMN") < 0) {
                t_errno = T_ERR_CLIENT_ONLY;
                break;
            }
            ret = pst->stream_send(pst);
            if (ret <= 0) {
               T_LOG(T_PERF, "Stream [%d] is broken due to [%s]", pst->stream_id, pst->stream_to_profile->selected_protocol->protocol_str_error());
                
                pst->stream_result.end_time_usec = t_get_currtime_us();
                
                pst->stream_to_profile->selected_protocol->protocol_close(&(pst->stream_sockfd));
                
                pthread_mutex_lock(&(pst->stream_to_profile->client_finish_lock));
                pst->stream_to_profile->client_finish_mark.set(pst->stream_id, true);
                pthread_mutex_unlock(&(pst->stream_to_profile->client_finish_lock));
                break;
            } else {
                pst->stream_to_profile->end_time_us = pst->stream_result.end_time_usec = t_get_currtime_us();
                pst->stream_result.total_sent_bytes += ret;
                break;
            }
        }
    }while(pst->stream_to_profile->status != T_SERVER_TERM &&
            pst->stream_to_profile->status != T_CLIENT_TERM &&
            pst->stream_to_profile->status != T_SERVER_ERROR &&
            pst->stream_to_profile->status != T_ACCESS_DENIED &&
            pst->stream_to_profile->status != T_PROGRAM_DONE);
    T_LOG(T_HINT, "Send worker [%lu] on core [%d] is cleaning on stream [%d]", (unsigned long int) pthread_self(),sched_getcpu(), pst->stream_id);
    
    pthread_mutex_lock(&(pst->stream_to_profile->client_finish_lock));
    pst->stream_to_profile->client_finish_mark.set(pst->stream_id, true);
    pthread_mutex_unlock(&(pst->stream_to_profile->client_finish_lock));
    return NULL;
}

void* tpg_client_perfmon(void *prof)
{
    tpg_profile* pp = (tpg_profile*) prof;
    pp->selected_protocol->protocol_client_perfmon(pp);
    return NULL;
}
