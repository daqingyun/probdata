/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>
#include <sys/uio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <udt.h>
#include "tpg_ctrl.h"
#include "tpg_profile.h"
#include "tpg_server.h"
#include "tpg_log.h"

static jmp_buf sigend_jmp_buf;

void tpg_server_sig_handler(int sig)
{
    longjmp(sigend_jmp_buf, 1);
}

void tpg_server_sig_end(tpg_profile *prof)
{
    prof->status = T_SERVER_TERM;
    T_LOG(T_HINT, "Server state ---> T_SERVER_TERM");
    
    if (ctrl_chan_send_ctrl_packet(prof, T_SERVER_TERM) < 0) {
        T_LOG(T_ERR, "Server send T_SERVER_TERM fail");
    }
    
    for(int i = 0; i < prof->accept_stream_num; i++) {
        if (prof->selected_protocol->protocol_close(&(prof->stream_list[i]->stream_sockfd)) < 0) {
            T_LOG(T_ERR, "Stream socket close fail");
        }
    }
    
    T_LOG(T_HINT, "SIGINT signal is received");
    T_LOG(T_HINT, "Dump accumulated stats");
    
    fflush(t_log_file);
    fsync(fileno(t_log_file));
    t_close_log();
    exit(EXIT_FAILURE);
}

void tpg_server_sig_reg(void (*handler)(int))
{
    signal(SIGINT, handler);
    signal(SIGTERM, handler);
    //signal(SIGHUP, handler);
    signal(SIGSEGV, handler);
}

int tpg_server_start_prof(tpg_profile *prof)
{
    int i = -1;
    int ret = 0;
    
    prof->recv_workers = new pthread_t[prof->accept_stream_num];
    
    T_LOG(T_HINT, "Server is creating recv workers on [%s] channel(s)", prof->selected_protocol->protocol_name);
    T_LOG(T_PERF, "Server starts receiving data on [%s] channel(s)", prof->selected_protocol->protocol_name);
    
    prof->start_time_us = t_get_currtime_us();
    
    for (i = 0; i < prof->accept_stream_num; i++) {
        ret = pthread_create(&((prof->recv_workers)[i]), NULL, tpg_server_recv_worker, ((prof->stream_list)[i]));
        if (ret != 0) {
            T_LOG(T_HINT, "Cannot create threads");
            t_errno = T_ERR_CREATE_STREAM;
            return -1;
        }
    }
    
    if (prof->cpu_affinity_flag > 0) {
        if (tpg_server_load_balance(prof) < 0) {
            T_LOG(T_ERR, "Set thread affinity fail");
            t_errno = T_ERR_AFFINITY;
            return -1;
        }
    }
    
#if T_AS_TOOLKIT == 1
    if (prof->server_report_flag > 0) {
        if (pthread_create(&(prof->perf_mon), NULL, tpg_server_perfmon, prof) != 0) {
            T_LOG(T_HINT, "Server create perf_mon thread fail");
            t_errno = T_ERR_THREAD_ERROR;
            return -1;
        }
    }
#endif
    
    
    return 0;
}

int tpg_server_load_balance(tpg_profile *prof)
{
    int i = 0;
    int cpu_id = 0;
    int cpu_number = 0;
    
    cpu_set_t cpuset;
    cpu_set_t cpuget;
    cpu_number = sysconf(_SC_NPROCESSORS_CONF);
    
    T_LOG(T_HINT, "Total number of CPU cores [%d]", cpu_number);
    
    for(i = 0; i < prof->accept_stream_num; i++) {
        CPU_ZERO(&cpuset);
        cpu_id = (i % cpu_number);
        CPU_SET(cpu_id, &cpuset);
        T_LOG(T_HINT, "Set cpu core [%d] for %d thread id [%llu]", cpu_id, i, (unsigned long long int)(prof->recv_workers)[i]);
        if (pthread_setaffinity_np((prof->recv_workers)[i], sizeof(cpu_set_t), &cpuset) != 0) {
            T_LOG(T_HINT, "Cannot set thread affinity");
            t_errno = T_ERR_AFFINITY;
            return -1;
        }
        
        CPU_ZERO(&cpuget);
        if (pthread_getaffinity_np((prof->recv_workers)[i], sizeof(cpu_set_t), &cpuget) != 0) {
            T_LOG(T_HINT, "Cannot get thread affinity");
            t_errno = T_ERR_AFFINITY;
            return -1;
        }
        if (CPU_ISSET(cpu_id, &cpuget) != 0) {
            T_LOG(T_HINT, "Recv thread [%llu] is on core [%d]", (unsigned long long int)(prof->recv_workers)[i], cpu_id);
        }
    }
    
    return 0;
}


int tpg_server_listen(tpg_profile *prof)
{
    if (ctrl_chan_listen(prof) < 0) {
        t_errno = T_ERR_LISTEN;
        return -1;
    }
    T_LOG(T_PERF, "Server is listening on port [%d]", prof->ctrl_listen_port);
    return 0;
}

int tpg_server_accept(tpg_profile *prof, int tmo_sec, int tmo_usec)
{
    int s = -1;
    char cookie[T_IDPKT_SIZE];
    socklen_t len = 0;
    struct sockaddr_in client_addr;
    
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    
    len = sizeof(client_addr);
    s = ctrl_chan_accept(prof, &client_addr, &len, tmo_sec, tmo_usec);
    if(s < 0) {
        t_errno = T_ERR_ACCEPT;
        return -1;
    } else if (s == 0) {
        return 0;
    } else {
        T_LOG(T_HINT, "ctrl chan accept returns %d", s);
        if (prof->ctrl_accept_sock == -1) {
            prof->ctrl_accept_sock = s;
            
            if (ctrl_chan_recv_Nbytes(prof->ctrl_accept_sock, prof->id_buf, T_IDPKT_SIZE) < 0) {
                t_errno = T_ERR_RECV_COOKIE;
                return -1;
            }
            
            T_LOG(T_PERF, "Client [%s] is accepted on port [%d]", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        } else {
            /* read the cookie to move on */
            T_LOG(T_HINT, "Server is busy request [%s|%d] is rejected",
                    inet_ntoa(client_addr.sin_addr),
                    ntohs(client_addr.sin_port));
            
            if (ctrl_chan_recv_Nbytes(s, cookie, T_IDPKT_SIZE) < 0) {
                t_errno = T_ERR_RECV_COOKIE;
                return -1;
            }
            
            if (ctrl_chan_send_ctrl_packet(prof, T_ACCESS_DENIED) < 0) {
                t_errno = T_ERR_SEND_MESSAGE;
                return -1;
            }
            
            close(s);
        }
    }
    
    return 1;
}

int tpg_server_poll_ctrl_chan(tpg_profile *prof)
{
    int rselect;
    int rsize;
    struct timeval tv;
    
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
    FD_ZERO(&(prof->ctrl_read_sockset));
    FD_SET(prof->ctrl_accept_sock, &(prof->ctrl_read_sockset));
    if (prof->ctrl_accept_sock > prof->ctrl_max_sock) {
        prof->ctrl_max_sock = prof->ctrl_accept_sock;
    }
    
    rselect = select(prof->ctrl_max_sock +1, &(prof->ctrl_read_sockset), NULL, NULL, &tv);
    
    if (rselect < 0) {
        t_errno = T_ERR_SELECT;
        return -1;
    } else if(rselect == 0) {
        return 0;
    } else if (rselect > 0) {
        if ( (prof->ctrl_accept_sock > 0) && FD_ISSET(prof->ctrl_accept_sock, &(prof->ctrl_read_sockset)) ) {
            if ( ( rsize = ctrl_chan_recv_Nbytes( prof->ctrl_accept_sock, (char*) &(prof->status), sizeof(signed char) ) ) <= 0) {
                if (rsize == 0) {
                    T_LOG(T_HINT, "An unexpected error happens");
                    t_errno = T_ERR_CTRL_CLOSE;
                } else {
                    T_LOG(T_HINT, "An unexpected error happens %s", t_errstr(t_errno));
                    t_errno = T_ERR_RECV_MESSAGE;
                }
                return -1;
            }
        } else {
            T_LOG(T_HINT, "Something is wrong with control socket");
            t_errno = T_ERR_SELECT;
            return -1;
        }
    }
    
    FD_CLR(prof->ctrl_accept_sock, &(prof->ctrl_read_sockset));
    prof->ctrl_max_sock = -1;
    return rselect;
}

int tpg_server_create_stream(tpg_profile* prof, int sockfd, int stream_id)
{
    T_LOG(T_HINT, "Create stream with sockfd [%d] stream id [%d]", sockfd, stream_id);
    
    int i;
    tpg_stream *pst = NULL;
    char temp[] = "/tmp/tpg.XXXXXX";
    
    if ((pst = new tpg_stream) == NULL) {
        t_errno = T_ERR_CREATE_STREAM;
        T_LOG(T_ERR, "Cannot create a stream");
        return -1;
    } else {
        T_LOG(T_HINT, "A new data stream is created");
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
        T_LOG(T_HINT, "mkstemp() fd [%d] with socket [%d]", pst->stream_blkbuf_fd, sockfd);
    }
    
    if (unlink(temp) < 0) {
        T_LOG(T_HINT, "unlink() fail");
        return -1;
    } else {
        T_LOG(T_HINT, "unlink()ed");
    }
    
    if (ftruncate(pst->stream_blkbuf_fd, prof->blocksize) < 0) {
        T_LOG(T_HINT, "ftruncate() fail");
        return -1;
    } else {
        T_LOG(T_HINT, "ftruncate()ed");
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
    
    pst->stream_sockfd = sockfd;
    
    T_LOG(T_HINT, "Load protocol-specific send/recv functions");
    pst->stream_send = prof->selected_protocol->protocol_send;
    pst->stream_recv = prof->selected_protocol->protocol_recv;
    
    //
    pst->stream_id = stream_id;
    pst->stream_blksize = pst->stream_to_profile->blocksize;
    
    T_LOG(T_HINT, "Start timers of all the streams");
    pst->stream_result.start_time_usec = pst->stream_result.interval_time_usec = pst->stream_result.end_time_usec = t_get_currtime_us();
    
    T_LOG(T_HINT, "Clear up counter for total bytes sent");
    pst->stream_result.total_sent_bytes = pst->stream_result.interval_sent_bytes = pst->stream_result.total_recv_bytes = pst->stream_result.interval_recv_bytes = 0;
    
    T_LOG(T_HINT, "Stream is initialized - [%s]", pst->stream_to_profile->selected_protocol->protocol_name);
    
    prof->stream_list.push_back(pst);
    T_LOG(T_HINT, "Add stream [%d] into stream_list", pst->stream_id);
    T_LOG(T_HINT, "Stream is created with socket [%d] stream id [%d]", sockfd, pst->stream_id);
    
    return 0;
}

int tpg_server_listen_streams(tpg_profile* prof)
{
    int s = -1;
    int err = -1;
    
    if ((s = prof->selected_protocol->protocol_listen(prof)) < 0) {
        prof->status = T_SERVER_ERROR;
        if(ctrl_chan_send_ctrl_packet(prof, T_SERVER_ERROR) < 0) {
            T_LOG(T_HINT, "Cannot send T_SERVER_ERROR");
            return -1;
        } else {
            T_LOG(T_HINT, "T_SERVER_ERROR is sent");
        }
        err = htonl(t_errno);
        if (ctrl_chan_send_Nbytes(prof->ctrl_accept_sock, (char*)&err, sizeof(err)) < 0) {
            T_LOG(T_HINT, "Error code sending error");
            t_errno = T_ERR_CTRL_WRITE;
        }
        err = htonl(errno);
        if (ctrl_chan_send_Nbytes(prof->ctrl_accept_sock, (char*)&err, sizeof(err)) < 0) {
            T_LOG(T_HINT, "Error code sending error");
            t_errno = T_ERR_CTRL_WRITE;
        }
        return -1;
    } else {
        T_LOG(T_HINT, "Server is listening on [%s] channel", prof->selected_protocol->protocol_name);
        return 0;
    }
}

int tpg_server_accept_streams(tpg_profile* prof)
{
    int s = -1;
    int acceptcounter = 0;
    while(true) {
        T_LOG(T_HINT, "[%s] connection accepted [%d] of [%d] counter [%d]", prof->selected_protocol->protocol_name, prof->accept_stream_num, prof->total_stream_num, acceptcounter);
        if (acceptcounter == prof->total_stream_num) {
            if (prof->accept_stream_num == 0) {
                T_LOG(T_HINT, "All requested connections fail with timeout 5 sec");
                t_errno = T_ERR_CREATE_STREAM;
                return -1;
            }
            break;
        }
        
        s = prof->selected_protocol->protocol_accept(prof, 10000);
        if (s < 0) {
            tpg_server_clean_up(prof);
            tpg_server_reset(prof);
            return -1;
        } else if (s == 0) {
            T_LOG(T_HINT, "[%s] connection on stream [%d] accept TIMEOUT", prof->selected_protocol->protocol_name, acceptcounter);
            acceptcounter += 1;
        } else {
            if (tpg_server_create_stream(prof, s, acceptcounter) < 0) {
                tpg_server_clean_up(prof);
                tpg_server_reset(prof);
                return -1;
            } else {
                T_LOG(T_HINT, "Stream is created successfully");
            }
            
            T_LOG(T_HINT, "Set stream %d is unfinished", acceptcounter);
            prof->client_finish_mark.set(acceptcounter, false);
            acceptcounter += 1;
            
            prof->accept_stream_num ++;
            T_LOG(T_HINT, "Total %d accepted data streams", prof->accept_stream_num);
            if (prof->accept_stream_num == prof->total_stream_num) {
                T_LOG(T_HINT, "Reached the total stream number %d", prof->total_stream_num);
                break;
            }
        }
    }
    return 0;
}

int tpg_server_run(tpg_profile *prof, int tmo_sec, int tmo_usec)
{
    int ret = -1;
    
    T_LOG(T_HINT, "Profiling starts running...");
    
    tpg_server_sig_reg(tpg_server_sig_handler);
    if (setjmp(sigend_jmp_buf)) {
        tpg_server_sig_end(prof);
    }
    
    if (tpg_server_listen(prof) < 0) {
        return -1;
    }
    
    ret = tpg_server_accept(prof, tmo_sec, tmo_usec);
    T_LOG(T_HINT, "server accept returns %d", ret);
    if (ret < 0) {
        return -1;
    } else if (ret == 0) {
        return 0;
    }
    
    prof->status = T_PROGRAM_START;
    prof->accept_stream_num = 0;
    
    //
    prof->status = T_CTRLCHAN_EST;
    T_LOG(T_HINT, "Server state ---> T_CTRLCHAN_EST");
    ctrl_chan_send_ctrl_packet(prof, T_CTRLCHAN_EST);
    
    //
    while (prof->status != T_PROGRAM_DONE &&
           prof->status != T_SERVER_ERROR &&
           prof->status != T_CLIENT_TERM  &&
           prof->status != T_SERVER_TERM) {
        
        ret = tpg_server_poll_ctrl_chan(prof);
        
        if (ret < 0) {
            t_errno = T_ERR_CTRL_READ;
            return -1;
        } else if (ret == 0) {
            continue;
        } else if (ret > 0) {
            if(tpg_server_proc_ctrl_packet(prof) < 0) {
                return -1;
            }
        }
    }
    
    T_LOG(T_HINT, "One-time profiling is done");
    
    return 0;
}

int tpg_server_proc_ctrl_packet(tpg_profile *prof)
{
    int i = -1;
    // int s = -1;
    // int err = -1;
    double total_recv_bytes = 0.0;
    long long int duration = 0;
    
    T_LOG(T_HINT, "Server process a control packet of type %s", t_status_to_str(prof->status));
    
    /* it has already been read in server_poll_ctrl_chan() and stored in "prof->status" */
    switch(prof->status)
    {
	case T_TWOWAY_HANDSHAKE:
        {
            prof->status = T_TWOWAY_HANDSHAKE;
            T_LOG(T_HINT, "Program state ---> T_TWOWAY_HANDSHAKE");
            if (ctrl_chan_send_ctrl_packet(prof, T_TWOWAY_HANDSHAKE) < 0) {
                t_errno = T_ERR_CTRL_WRITE;
                return -1;
            } else {
                T_LOG(T_HINT, "T_TWOWAY_HANDSHAKE is sent");
            }
            if (ctrl_chan_ctrlmsg_exchange(prof) < 0) {
                return -1;
            } else {
                T_LOG(T_HINT, "Control parameters are exchanged"); 
                if (prof->selected_protocol->protocol_id == T_PUDT) {
                    if (UDT::startup() == UDT::ERROR) {
                        T_LOG(T_HINT, "UDT cannot startup");
                        t_errno = T_ERR_UDT_ERROR;
                        return -1;
                    }
                }
            }
            break;
        }
	case T_CREATE_DATA_STREAMS:
        {
            if (tpg_server_listen_streams(prof) < 0) {
                t_errno = T_ERR_CREATE_STREAM;
                return -1;
            }
            
            prof->status = T_CREATE_DATA_STREAMS;
            T_LOG(T_HINT, "Program state ---> T_CREATE_DATA_STREAMS");
            if (ctrl_chan_send_ctrl_packet(prof, T_CREATE_DATA_STREAMS) < 0) {
                T_LOG(T_HINT, "Cannot send T_CREATE_DATA_STREAMS");
                t_errno = T_ERR_CTRL_WRITE;
                return -1;
            } else {
                T_LOG(T_HINT, "T_CREATE_DATA_STREAMS is sent");
            }
            if (tpg_server_accept_streams(prof) < 0) {
                t_errno = T_ERR_CREATE_STREAM;
                return -1;
            }
            
            if (prof->selected_protocol->protocol_close(&(prof->protocol_listen_sock) ) < 0) {
                T_LOG(T_HINT, "[%s] socket closing error", prof->selected_protocol->protocol_name);
                t_errno = T_ERR_SOCK_CLOSE_ERROR;
                return -1;
            } else {
                T_LOG(T_HINT, "Protocol listen socket is closed");
            }
            break;
        }
        case T_PROFILING_START:
        {
            if(tpg_prof_start_timers(prof) < 0) {
                T_LOG(T_HINT, "Start timers fail");
                t_errno = T_ERR_INIT_PROFILE;
                return -1;
            } else {
                T_LOG(T_HINT, "All timers are started");
            }
            
            prof->status = T_PROFILING_START;
            T_LOG(T_HINT, "Program state --> T_PROFILING_START");
            if(ctrl_chan_send_ctrl_packet(prof, T_PROFILING_START) < 0) {
                T_LOG(T_HINT, "Cannot send T_PROFILING_START");
                t_errno = T_ERR_CTRL_WRITE;
                return -1;
            } else {
                T_LOG(T_HINT, "T_PROFILING_START is sent");
            }
            
            prof->status = T_PROFILING_RUNNING;
            T_LOG(T_HINT, "Program state ---> T_PROFILING_RUNNING");
            
            if(tpg_server_start_prof(prof) < 0) {
                T_LOG(T_HINT, "Cannot start receiving workers");
                return -1;
            } else {
                T_LOG(T_HINT, "All receiving workers are started");
            }
            break;
        }
        case T_PROFILING_END:
        {
            T_LOG(T_HINT, "Program state ---> T_PROFILING_END");
            
            T_LOG(T_HINT, "Server is waiting for threads to finish");
            for(i = 0; i < prof->accept_stream_num; i++) {
                if(pthread_join((prof->recv_workers)[i], NULL) != 0) {
                    T_LOG(T_HINT, "pthread_join() fails on stream [%d] due to [%s]", i, strerror(errno));
                }
            }
            
            pthread_mutex_lock(&(prof->server_finish_lock));
            prof->server_finish_mark = 1;
            pthread_mutex_unlock(&(prof->server_finish_lock));
            
#if T_AS_TOOLKIT == 1
            if (prof->server_report_flag > 0) {
                if(pthread_join(prof->perf_mon, NULL) != 0) {
                    T_LOG(T_HINT, "pthread_join() fails on perf_mon due to [%s]", strerror(errno));
                }
            }
#endif
            prof->end_time_us = t_get_currtime_us();
            for(i = 0; i < (prof->accept_stream_num); i++) {
                total_recv_bytes += (prof->stream_list)[i]->stream_result.total_recv_bytes;
                T_LOG(T_HINT, "[%s] data stream [%d] totally received [%lld] bytes",
                    prof->selected_protocol->protocol_name,
                    ((prof->stream_list)[i])->stream_id,
                    ((prof->stream_list)[i])->stream_result.total_recv_bytes);
            }
            duration = (prof->end_time_us - prof->start_time_us);
            prof->server_perf_mbps = 8.0 * total_recv_bytes / duration;
            
            prof->status = T_PROFILING_END;
            T_LOG(T_HINT, "Program state ---> T_PROFILING_END");
            if (ctrl_chan_send_ctrl_packet(prof, T_PROFILING_END) < 0) {
                T_LOG(T_HINT, "Cannot send T_PROFILING_END");
                t_errno = T_ERR_CTRL_WRITE;
                return -1;
            } else {
                T_LOG(T_HINT, "T_PROFILING_END is sent");
            }
            break;
        }
        case T_EXCHANGE_PROFILE:
        {
            prof->status = T_EXCHANGE_PROFILE;
            T_LOG(T_HINT, "Program state ---> T_EXCHANGE_PROFILE");
            if (ctrl_chan_send_ctrl_packet(prof, T_EXCHANGE_PROFILE) < 0) {
                T_LOG(T_HINT, "Cannot send T_EXCHANGE_PROFILE");
                t_errno = T_ERR_CTRL_WRITE;
                return -1;
            } else {
                T_LOG(T_HINT, "T_EXCHANGE_PROFILE is sent");
            }
            
            if (ctrl_chan_perf_exchange(prof) < 0) {
                return -1;
            } else {
                T_LOG(T_HINT, "Performance results are exchanged");
            }
            
            T_LOG(T_PERF, "Client perf: %.3lf Gb/s", prof->client_perf_mbps / 1000.0);
            T_LOG(T_PERF, "Server perf: %.3lf Gb/s", prof->server_perf_mbps / 1000.0);
            break;
        }
        case T_PROGRAM_DONE:
        {
            prof->status = T_PROGRAM_DONE;
            T_LOG(T_HINT, "Program state ---> T_PROGRAM_DONE");
            if (ctrl_chan_send_ctrl_packet(prof, T_PROGRAM_DONE) < 0) {
                T_LOG(T_HINT, "Cannot send T_PROGRAM_DONE");
                t_errno = T_ERR_CTRL_WRITE;
                return -1;
            } else {
                T_LOG(T_HINT, "T_PROGRAM_DONE is sent");
            }
            
            if (tpg_server_clean_up(prof) < 0) {
                T_LOG(T_HINT, "Server cleanup error");
                return -1;
            } else {
                T_LOG(T_HINT, "Server is cleaned up successfully");
            }
            
            if (tpg_server_reset(prof) < 0) {
                T_LOG(T_HINT, "Server reset error");
                return -1;
            } else {
                T_LOG(T_HINT, "Server is re-set successfully");
            }
            break;
        }        
        case T_CLIENT_TERM:
        {
            T_LOG(T_HINT, "Program state ---> T_CLIENT_TERM");
            
            pthread_mutex_lock(&(prof->server_finish_lock));
            prof->server_finish_mark = 1;
            pthread_mutex_unlock(&(prof->server_finish_lock));
            
            /* client accidentally terminated, calculate perf. so far */
            prof->end_time_us = t_get_currtime_us();
            for(i = 0; i < (prof->accept_stream_num); i++) {
                total_recv_bytes += (prof->stream_list)[i]->stream_result.total_recv_bytes;
                T_LOG(T_HINT, "Cliend stopped [%s] on stream [%d] and recv [%lld] bytes",
                    prof->selected_protocol->protocol_name,
                    ((prof->stream_list)[i])->stream_id,
                    ((prof->stream_list)[i])->stream_result.total_recv_bytes);
            }
            duration = (prof->end_time_us - prof->start_time_us);
            prof->server_perf_mbps = 8.0 * total_recv_bytes / duration;
            
            T_LOG(T_PERF, "Server perf: %.3lf Gb/s", prof->server_perf_mbps / 1000.0);
            
            if (tpg_server_clean_up(prof) < 0) {
                T_LOG(T_HINT, "Server cannot cleanup");
                return -1;
            } else {
                T_LOG(T_HINT, "Server is cleaned up successfully");
            }
            
            if (tpg_server_reset(prof) < 0) {
                T_LOG(T_HINT, "Server cannot reset");
                return -1;
            } else {
                T_LOG(T_HINT, "Server reset successfully");
            }
            t_errno = T_ERR_CLIENT_TERM;
            return -1;
            break;
        }
        default:
        {
            T_LOG(T_HINT, "An unknown status is received");
            t_errno = T_ERR_MESSAGE;
            return -1;
            break;
        }
    }
    
    T_LOG(T_HINT, "Server process control pkt is done type [%s]", t_status_to_str(prof->status));
    
    return 0;
}

int tpg_server_clean_up(tpg_profile *prof)
{
    int i;
    
    T_LOG(T_HINT, "Server is cleaning...");
    
    if (prof->status != T_PROGRAM_DONE && prof->status != T_PROFILING_END) {
        T_LOG(T_HINT, "***Server is waiting for working threads");
        for(i = 0; i < prof->accept_stream_num; i++) {
            if(pthread_join((prof->recv_workers)[i], NULL) != 0) {
                T_LOG(T_HINT, "pthread_join() fails on stream [%d]", i);
                t_errno = T_ERR_THREAD_ERROR;
                return -1;
            }
        }
        
#if T_AS_TOOLKIT == 1
        if (prof->server_report_flag > 0) {
            if(pthread_join(prof->perf_mon, NULL) != 0) {
                T_LOG(T_HINT, "pthread_join() fails on perf_mon [%s]\n", strerror(errno));
                t_errno = T_ERR_THREAD_ERROR;
                return -1;
            }
        }
#endif
    }
    
    T_LOG(T_HINT, "Destroy mutex...");
    pthread_mutex_destroy(&(prof->client_finish_lock));
    pthread_mutex_destroy(&(prof->server_finish_lock));
    T_LOG(T_HINT, "Mutexes are destroyed...");
    
    /* close all sockets, these two are TCP */
    T_LOG(T_HINT, "Closing control channel sockets...");
    ctrl_chan_close_listen_sock(prof);
    ctrl_chan_close_accept_sock(prof);
    T_LOG(T_HINT, "Control channel sockets are closed");
    
    //
    std::vector<tpg_stream *>::iterator it;
    for (it = prof->stream_list.begin(); it != prof->stream_list.end(); ++it) {
        if (prof->selected_protocol->protocol_close(&((*it)->stream_sockfd) ) < 0) {
            T_LOG(T_HINT, "Protocol socket close failure [%s]", prof->selected_protocol->protocol_str_error());
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
    
    T_LOG(T_HINT, "Deleting all recv workers...");
    delete [] (prof->send_workers);
    prof->send_workers = NULL;
    T_LOG(T_HINT, "All recv workers are deleted");

    return 0;
}

int tpg_server_reset(tpg_profile *prof)
{
    T_LOG(T_HINT, "Server is resetting");
    
    prof->role = T_SERVER;
    prof->is_sender = 0;
    prof->prof_time_duration = 10;
    prof->report_interval = 1.0;
    prof->cpu_affinity_flag = -1;
    prof->is_multi_nic_enabled = -1;
    prof->interval_report_flag = -1;
    // prof->rept_server_flag = -1; /* this could be ignored or not */
    memset(prof->local_ip, '\0', T_HOSTADDR_LEN);
    prof->local_port = -1;
    memset(prof->remote_ip, '\0', T_HOSTADDR_LEN);
    
    prof->ctrl_accept_sock = -1;
    prof->protocol_listen_sock = -1;
    
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
    prof->udt_mss = 0;
    prof->tcp_mss = 0;
    prof->udt_send_bufsize = 0;
    prof->udt_recv_bufsize = 0;
    prof->udp_send_bufsize = 0;
    prof->udp_recv_bufsize = 0;
    prof->udt_maxbw = 0.0;
    
    set_protocol_id(prof, T_PTCP);
    prof->blocksize = T_DEFAULT_TCP_BLKSIZE;
    memset(prof->id_buf, 0, T_IDPKT_SIZE);
    
    FD_ZERO(&prof->ctrl_read_sockset);
    prof->ctrl_max_sock = -1;
    
    prof->send_workers = NULL;
    prof->recv_workers = NULL;
    
    prof->client_perf_mbps = 0.0;
    prof->server_perf_mbps = 0.0;
    
    return 0;
}

void* tpg_server_recv_worker(void *stream)
{
    int ret;
    int server_status = 0;
    tpg_stream* pst = NULL;
    
    pst = (tpg_stream *) stream;
    
    T_LOG(T_HINT, "Recv worker [%lu] on core [%d] is receiving on stream [%d]", (unsigned long int) pthread_self(), sched_getcpu(), pst->stream_id);
    
    while (pst->stream_to_profile->status != T_SERVER_TERM &&
            pst->stream_to_profile->status != T_SERVER_ERROR &&
            pst->stream_to_profile->status != T_CLIENT_TERM &&
            pst->stream_to_profile->status != T_PROGRAM_DONE &&
            pst->stream_to_profile->server_finish_mark <= 0) {
        pthread_mutex_lock(&(pst->stream_to_profile->server_finish_lock));
        server_status = pst->stream_to_profile->server_finish_mark;
        pthread_mutex_unlock(&(pst->stream_to_profile->server_finish_lock));
        if (server_status > 0) {
            /* server is set to be done before I receive a termination packet this means error happens */
            T_LOG(T_HINT, "Server stops before a termination packet is received and worker [%lu] also aborts on stream [%d]",
                    (unsigned long int) pthread_self(), pst->stream_id);
            break;
        }
        ret = pst->stream_recv(pst);
        if (ret == 0) {
            T_LOG(T_HINT, "Timeout happens in recv worker [%lu] on stream [%d]", (unsigned long int) pthread_self(), pst->stream_id);
            pst->stream_to_profile->selected_protocol->protocol_close(&(pst->stream_sockfd));
            break;
        } else if (ret < 0) {
            T_LOG(T_HINT, "Connection broken, worker [%lu] is killed on stream [%d]", (unsigned long int) pthread_self(), pst->stream_id);
            pst->stream_to_profile->selected_protocol->protocol_close(&(pst->stream_sockfd));
            break;
        } else {
            // the last terminate should not be counted
            // sp->st_result->bytes_recv += ret;
            if(tpg_server_proc_data_packet(pst) == 0) {
                pst->stream_result.total_recv_bytes += ret;
                pst->stream_to_profile->end_time_us = pst->stream_result.end_time_usec = t_get_currtime_us();
                T_LOG(T_HINT, "A terminate packet is received");
                break;
            } else if (tpg_server_proc_data_packet(pst) > 0) {
                pst->stream_result.total_recv_bytes += ret;
                pst->stream_to_profile->end_time_us = pst->stream_result.end_time_usec = t_get_currtime_us();
            } else {
                t_errno = T_ERR_UNKNOWN;
                T_LOG(T_HINT, "An error happens in worker [%lu] on stream [%d]", (unsigned long int) pthread_self(), pst->stream_id);
                T_LOG(T_HINT, "Data might be corrupted");
                pst->stream_to_profile->status = T_SERVER_TERM;
                if (ctrl_chan_send_ctrl_packet(pst->stream_to_profile, T_SERVER_TERM) < 0) {
                    T_LOG(T_HINT, "Cannot send T_SERVER_TERM");
                    t_errno = T_ERR_CTRL_WRITE;
                    return NULL;
                } else {
                    T_LOG(T_HINT, "T_SERVER_TERM is sent");
                }
                break;
            }
        }
    }
    T_LOG(T_HINT, "Recv worker [%lu] on core [%d] is cleaning on stream [%d]", (unsigned long int) pthread_self(), sched_getcpu(), pst->stream_id);
    
    return NULL;
}

int tpg_server_proc_data_packet(tpg_stream *pst)
{
    if (strncmp(pst->stream_blkbuf, "TMN", strlen("TMN")) == 0) {
        return 0;
    } else if (strncmp(pst->stream_blkbuf, "NML", strlen("NML")) == 0) {
        return 1;
    } else if (strncmp(pst->stream_blkbuf, "INI", strlen("INI")) == 0) {
        return 1;
    } else {
        return -1;
    }
}

void* tpg_server_perfmon(void *prof)
{
    tpg_profile* pp = (tpg_profile*) prof;
    int i = 0;
    int profile_state = 0;
    long long int start_t_us = 0;
    long long int end_t_us = 0;
    tpg_stream* pst = NULL;
    double mbps_recv_rate = 0;
    double total_recv_mb = 0.0;
    double duration_t_us = 0.0;
    double goodput_mbps = 0.0;
    double rtt_ms = 0.0;
    // double estimateBW_mbps = 0.0;
    
    int rcvbuf_byte = 0;
    int rcvpkt_loss = 0;
    int sent_nak_pkts = 0;
    
    UDTSOCKET udtsock = 0;
    UDT::TRACEINFO info;
    printf("------------------------------------------------------------\n");
    printf("[ ID]\t");
    printf("Time(sec)\t");
    printf("Recv(MB)\t");
    printf("Goodput(Mbps)\t");
    printf("RecvBufSize(B)\t");
    printf("RecvRate(Mbps)\t");
    printf("SentNAKPkts\t");
    printf("TotalRcvPktsLost\t");
    printf("RTT(ms)\n");
    // printf("BW(Mbps)\n");
    printf("------------------------------------------------------------\n");
    
    while (pp->status != T_CLIENT_TERM && 
            pp->status != T_SERVER_TERM &&
            pp->status != T_SERVER_ERROR &&
            pp->status != T_ACCESS_DENIED &&
            pp->status != T_PROGRAM_DONE &&
            pp->server_finish_mark <= 0) {
        
        // check how long it has passed
        ;
        
        pthread_mutex_lock(&(pp->server_finish_lock));
        profile_state = pp->server_finish_mark;
        pthread_mutex_unlock(&(pp->server_finish_lock));
        if (profile_state > 0) {
            break;
        }
        
        for (i = 0; i < (pp->accept_stream_num); i++) {
            pst = (tpg_stream *)((pp->stream_list)[i]);
            pst->stream_to_profile->end_time_us = pst->stream_result.end_time_usec = t_get_currtime_us();
            start_t_us = pst->stream_result.interval_time_usec - pst->stream_result.start_time_usec; // in us
            end_t_us = pst->stream_result.end_time_usec - pst->stream_result.start_time_usec; // in us (10^-6 sec)
            pst->stream_result.interval_time_usec =  pst->stream_result.end_time_usec;
            duration_t_us = (end_t_us - start_t_us) * 1.0;
            
            total_recv_mb = 1.0 * (pst->stream_result.total_recv_bytes - pst->stream_result.interval_recv_bytes);
            total_recv_mb = (1.0 * total_recv_mb) / T_MB;
            pst->stream_result.interval_recv_bytes = pst->stream_result.total_recv_bytes;
            
            goodput_mbps = 8.0 * T_MB * (total_recv_mb / duration_t_us);
            
            udtsock = pst->stream_sockfd;
            if (UDT::ERROR == UDT::perfmon(udtsock, &info)) {
                t_errno = T_ERR_UDT_PERF_ERROR;
                return NULL;
            }
            
            rcvbuf_byte = info.byteAvailRcvBuf;
            //send_duration = info.usSndDuration;
            sent_nak_pkts = info.pktSentNAK;
            mbps_recv_rate = info.mbpsRecvRate;
            rcvpkt_loss = info.pktRcvLossTotal;
            rtt_ms = info.msRTT;
            // estimateBW_mbps = info.mbpsBandwidth;
            
            // output results
            printf("[%3d]\t", pst->stream_id);
            printf("%-3lld->%3lld\t", start_t_us / T_ONE_MILLION, end_t_us / T_ONE_MILLION);
            printf("%-6.2lf\t\t", total_recv_mb);
            printf("%-7.2lf\t\t", goodput_mbps);
            printf("%-6d\t", rcvbuf_byte);
            printf("%-7.2lf\t\t", mbps_recv_rate);
            printf("%-5d\t\t", sent_nak_pkts);
            printf("%-5d\t\t", rcvpkt_loss);
            printf("%-5.2lf\n", rtt_ms);
            //printf("%-7.2lf\n", estimateBW_mbps);
        }
        usleep((int)(1.0 * T_ONE_MILLION));
    }
    T_LOG(T_HINT, "server_perfmon() thread is exitting...");
    return NULL;
}
