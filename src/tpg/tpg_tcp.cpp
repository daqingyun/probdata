/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "tpg_log.h"
#include "tpg_tcp.h"


int tcp_init(tpg_profile *prof)
{
    return 0;
}

/*
int tcp_getsockname(tpg_stream *pstream)
{
    unsigned int len = 0;
    
    if (getsockname(pstream->st_sockfd,
            (struct sockaddr *) &(pstream->stream_localaddr),
            (unsigned int*)&len) < 0) {
        t_errno = TE_INIT_STREAM;
        return -1;
    }
    return 0;
}
*/

/*
int tcp_getpeername(tpg_stream *pstream)
{
    unsigned int len = 0;
    if (getpeername(pstream->st_sockfd,
            (struct sockaddr *) &(pstream->stream_peeraddr),
            (unsigned int*)&len) < 0) {
        t_errno = TE_INIT_STREAM;
        return -1;
    }
    return 0;
}
*/

int tcp_listen(tpg_profile *prof)
{
    T_LOG(T_HINT, "TCP data channel is listening...");
    
    int s = -1;
    
    int set_tcp_bufsize_opt = -1;
    int get_tcp_bufsize = -1;
    socklen_t get_tcp_bufsize_len = 0;
    
    int set_tcp_mss_opt = -1;
    int get_tcp_mss = -1;
    socklen_t get_tcp_mss_len = 0;
    
    int set_sock_reuse_opt = -1;
    int get_sock_reuse = -1;
    socklen_t get_sock_reuse_len = 0;
    
    char portstr[8];
    struct addrinfo hints;
    struct addrinfo *localaddr;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    sprintf(portstr, "%d", prof->protocol_listen_port);
    
    /*
    if(strlen(prof->local_ip) > 0) {
        if (getaddrinfo(prof->local_ip, portstr, &hints, &localaddr) != 0) {
            T_LOG(T_ERR, "getaddrinfo() error");
            freeaddrinfo(localaddr);
            return -1;
        }
    } else {
        if (getaddrinfo(NULL, portstr, &hints, &localaddr) != 0) {
            T_LOG(T_ERR, "getaddrinfo() error");
            freeaddrinfo(localaddr);
            return -1;
        } 
    }*/
    
    if (getaddrinfo(NULL, portstr, &hints, &localaddr) != 0) {
        T_LOG(T_ERR, "getaddrinfo() error");
        freeaddrinfo(localaddr);
        return -1;
    }
    
    s = socket(localaddr->ai_family, localaddr->ai_socktype, 0);
    if (s < 0) {
        T_LOG(T_ERR, "Cannot create a socket");
        freeaddrinfo(localaddr);
        return -1;
    }
    
    T_LOG(T_HINT, "TCP data channel local address - [%s|%d]",
            inet_ntoa( ((struct sockaddr_in*)localaddr->ai_addr)->sin_addr ),
            ntohs(((struct sockaddr_in*)localaddr->ai_addr)->sin_port) );
    
    set_sock_reuse_opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)(&set_sock_reuse_opt), sizeof(int)) < 0) {
        T_LOG(T_ERR, "Cannot set TCP option SO_REUSEADDR");
        close(s);
        freeaddrinfo(localaddr);
        return -1;
    } else {
        get_sock_reuse = -1;
        get_sock_reuse_len = 0;
        if (getsockopt(s, SOL_SOCKET, SO_REUSEADDR, &get_sock_reuse, &get_sock_reuse_len) < 0) {
            T_LOG(T_ERR, "Can not get SO_REUSEADDR");
            close(s);
            freeaddrinfo(localaddr);
            return -1;
        } else {
            T_LOG(T_HINT, "Set SO_REUSEADDR to be %d", get_sock_reuse);
        }
    }
    
    if (prof->tcp_sock_bufsize > 0) {
        set_tcp_bufsize_opt = prof->tcp_sock_bufsize;
        if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)(&set_tcp_bufsize_opt), sizeof(int)) < 0) {
            T_LOG(T_ERR, "Cannot set TCP option SO_RCVBUF");
            close(s);
            freeaddrinfo(localaddr);
            t_errno = T_ERR_SET_BUF;
            return -1;
        } else {
            get_tcp_bufsize = -1;
            get_tcp_bufsize_len = 0;
            if (getsockopt(s, SOL_SOCKET, SO_RCVBUF, &get_tcp_bufsize, &get_tcp_bufsize_len) < 0) {
                T_LOG(T_ERR, "Can not get SO_RCVBUF");
                close(s);
                freeaddrinfo(localaddr);
                return -1;
            } else {
                T_LOG(T_HINT, "Set SO_RCVBUF to be %d", get_tcp_bufsize);
            }
        }
        
        set_tcp_bufsize_opt = prof->tcp_sock_bufsize;
        if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)(&set_tcp_bufsize_opt), sizeof(int)) < 0) {
            T_LOG(T_ERR, "Cannot set TCP option SO_SNDBUF");
            close(s);
            freeaddrinfo(localaddr);
            t_errno = T_ERR_SET_BUF;
            return -1;
        } else {
            get_tcp_bufsize = -1;
            get_tcp_bufsize_len = 0;
            if (getsockopt(s, SOL_SOCKET, SO_SNDBUF, &get_tcp_bufsize, &get_tcp_bufsize_len) < 0) {
                T_LOG(T_ERR, "Can not get SO_SNDBUF");
                close(s);
                freeaddrinfo(localaddr);
                return -1;
            } else {
                T_LOG(T_ERR, "Set SO_SNDBUF to be %d", get_tcp_bufsize);
            }
        }
    }
    
    if (prof->tcp_mss > 0) {
        set_tcp_mss_opt = prof->tcp_mss;
        if (setsockopt(s, IPPROTO_TCP, TCP_MAXSEG, (char*)&set_tcp_mss_opt, sizeof(set_tcp_mss_opt)) < 0) {
            T_LOG(T_ERR, "Cannot set TCP option TCP_MAXSEG");
            close(s);
            freeaddrinfo(localaddr);
            t_errno = T_ERR_MSS;
            return -1;
        } else {
            get_tcp_mss = -1;
            get_tcp_bufsize_len = 0;
            if (getsockopt(s, IPPROTO_TCP, TCP_MAXSEG, &get_tcp_mss, &get_tcp_mss_len) < 0) {
                T_LOG(T_ERR, "Can not get TCP_MAXSEG");
                close(s);
                freeaddrinfo(localaddr);
                return -1;
            } else {
                T_LOG(T_ERR, "Set TCP_MAXSEG to be %d", get_tcp_mss);
            }
        }
    }
    
    
    if(bind(s, (struct sockaddr*)localaddr->ai_addr, localaddr->ai_addrlen) < 0) {
        close(s);
        T_LOG(T_ERR, "Cannot bing TCP socket");
        freeaddrinfo(localaddr);
        return -1;
    } else {
        T_LOG(T_HINT, "TCP data channel bind local address - [%s|%d]",
            inet_ntoa( ((struct sockaddr_in*)localaddr->ai_addr)->sin_addr ),
            ntohs(((struct sockaddr_in*)localaddr->ai_addr)->sin_port) );
    }
    
    if (listen(s, T_MAX_BACKLOG) < 0) {
        close(s);
        freeaddrinfo(localaddr);
        T_LOG(T_ERR, "TCP socket cannot listen");
        return -1;
    } else {
        T_LOG(T_HINT, "TCP as data channel listen on - [%s|%d]",
            inet_ntoa( ((struct sockaddr_in*)localaddr->ai_addr)->sin_addr ),
            ntohs(((struct sockaddr_in*)localaddr->ai_addr)->sin_port) );
    }
    
    prof->protocol_listen_sock = s;
    
    freeaddrinfo(localaddr);
    return 0;
}

int tcp_accept(tpg_profile *prof, int ms_timeout)
{
    T_LOG(T_HINT, "TCP data channel is accepting...");
    
    int s = -1;
    int rselect = -1;
    // signed char rbuf = ACCESS_DENIED;
    // char cookie[T_IDPKT_SIZE];
    socklen_t len;
    struct sockaddr_in addr;
    
    int maxfd = -1;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(prof->protocol_listen_sock, &readfds);
    if (prof->protocol_listen_sock > maxfd) {
        maxfd = prof->protocol_listen_sock;
    }
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = ms_timeout * T_THOUSAND;
    if ( (rselect = select(maxfd + 1, &readfds, NULL, NULL, &tv)) <= 0) {
        if (rselect == 0) {
            return 0;
        } else {
            t_errno = T_ERR_UDT_ERROR;
            return -1;
        }
    }
    
    len = sizeof(addr);
    if ((s = accept(prof->protocol_listen_sock, (struct sockaddr *) &addr, &len)) < 0) {
        t_errno = T_ERR_STREAM_CONNECT;
        return -1;
    } else {
        T_LOG(T_HINT, "Client [%s] is accepted on port [%d]", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    }
    
    FD_CLR(prof->protocol_listen_sock, &readfds);
    
    // do not do id check read cookie through data channel
    /* if (tcp_recvN(s, cookie, T_IDPKT_SIZE) < 0) {
        close(s);
        t_errno = T_ERR_RECV_COOKIE;
        return -1;
    }
    
    // check cookie
    if (strcmp(prof->id_buf, cookie) != 0) {
        if (tcp_sendN(s, (char*) &rbuf, sizeof(rbuf)) < 0) {
            close(s);
            t_errno = T_ERR_SEND_MESSAGE;
            return -1;
        }
        close(s);
    } */
    return s;
}

int tcp_connect(tpg_stream *pst)
{
    int s = -1;
    
    int set_tcp_bufsize_opt = -1;
    int get_tcp_bufsize = -1;
    socklen_t get_tcp_bufsize_len = 0;
    
    int set_tcp_mss_opt = -1;
    int get_tcp_mss = -1;
    socklen_t get_tcp_mss_len = 0;
    
    char portstr[6];
    struct addrinfo hints;
    struct addrinfo *localaddr = NULL;
    struct addrinfo *serveraddr = NULL;
    
    T_LOG(T_HINT, "Connecting - [from: %s to: %s at port: %d (dst)]", pst->stream_src_ip, pst->stream_dst_ip, pst->stream_dst_port);
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (pst->stream_src_port > 0) {
        sprintf(portstr, "%d", pst->stream_src_port);
    } else {
        sprintf(portstr, "%d", 0);
    }
    if(strlen(pst->stream_src_ip) > 0) {
        if (getaddrinfo(pst->stream_src_ip,portstr,&hints,&localaddr) != 0) {
            T_LOG(T_ERR, "getaddrinfo() error");
            freeaddrinfo(localaddr);
            return -1;
        } 
    } else {
        if (getaddrinfo(NULL, portstr, &hints, &localaddr) != 0) {
            T_LOG(T_ERR, "getaddrinfo() error %s", strerror(errno));
            freeaddrinfo(localaddr);
            return -1;
        } 
    }
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    // sprintf(portstr, "%d", T_DEFAULT_TCP_PORT);
    sprintf(portstr, "%d", pst->stream_dst_port);
    if(strlen(pst->stream_dst_ip) > 0) {
        if (getaddrinfo(pst->stream_dst_ip,portstr,&hints,&serveraddr) != 0) {
            T_LOG(T_ERR, "getaddrinfo() error");
            freeaddrinfo(localaddr);
            freeaddrinfo(serveraddr);
            return -1;
        } 
    } else {
        T_LOG(T_ERR, "error - server ip and port are both required");
        freeaddrinfo(localaddr);
        freeaddrinfo(serveraddr);
        return -1;
    }
    
    s = socket(serveraddr->ai_family, serveraddr->ai_socktype, 0);
    if (s < 0) {
        close(s);
        T_LOG(T_ERR, "Socket create error");
        freeaddrinfo(localaddr);
        freeaddrinfo(serveraddr);
        return -1;
    }
    
    if (pst->stream_to_profile->tcp_mss > 0) {
        set_tcp_mss_opt = pst->stream_to_profile->tcp_mss;
        if (setsockopt(s, IPPROTO_TCP, TCP_MAXSEG, (char*)&set_tcp_mss_opt, sizeof(set_tcp_mss_opt)) < 0) {
            T_LOG(T_ERR, "Cannot set TCP option TCP_MAXSEG");
            close(s);
            freeaddrinfo(localaddr);
            freeaddrinfo(serveraddr);
            t_errno = T_ERR_MSS;
            return -1;
        } else {
            get_tcp_mss = -1;
            get_tcp_bufsize_len = 0;
            if (getsockopt(s, IPPROTO_TCP, TCP_MAXSEG, &get_tcp_mss, &get_tcp_mss_len) < 0) {
                T_LOG(T_ERR, "Can not get TCP_MAXSEG");
                close(s);
                freeaddrinfo(localaddr);
                freeaddrinfo(serveraddr);
                return -1;
            } else {
                T_LOG(T_ERR, "Set TCP_MAXSEG to be %d", get_tcp_mss);
            }
        }
    }
    
    if (pst->stream_to_profile->tcp_sock_bufsize > 0) {
        set_tcp_bufsize_opt = pst->stream_to_profile->tcp_sock_bufsize;
        if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)(&set_tcp_bufsize_opt), sizeof(int)) < 0) {
            T_LOG(T_ERR, "Cannot set TCP SO_RCVBUF option");
            close(s);
            freeaddrinfo(localaddr);
            freeaddrinfo(serveraddr);
            t_errno = T_ERR_SET_BUF;
            return -1;
        } else {
            get_tcp_bufsize = -1;
            get_tcp_bufsize_len = 0;
            if (getsockopt(s, SOL_SOCKET, SO_RCVBUF, &get_tcp_bufsize, &get_tcp_bufsize_len) < 0) {
                T_LOG(T_ERR, "Cannot get TCP SO_RCVBUF option");
                close(s);
                freeaddrinfo(localaddr);
                freeaddrinfo(serveraddr);
                return -1;
            } else {
                T_LOG(T_HINT, "Set SO_RCVBUF to be %d", get_tcp_bufsize);
            }
        }
        
        set_tcp_bufsize_opt = pst->stream_to_profile->tcp_sock_bufsize;
        if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)(&set_tcp_bufsize_opt), sizeof(int)) < 0) {
            close(s);
            freeaddrinfo(localaddr);
            freeaddrinfo(serveraddr);
            t_errno = T_ERR_SET_BUF;
            return -1;
        } else {
            get_tcp_bufsize = -1;
            get_tcp_bufsize_len = 0;
            if (getsockopt(s, SOL_SOCKET, SO_SNDBUF, &get_tcp_bufsize, &get_tcp_bufsize_len) < 0) {
                T_LOG(T_ERR, "Cannot get TCP SO_SNDBUF option");
                close(s);
                freeaddrinfo(localaddr);
                freeaddrinfo(serveraddr);
                return -1;
            } else {
                T_LOG(T_HINT, "Set SO_SNDBUF to be %d", get_tcp_bufsize);
            }
        }
    }
    
    if (strlen(pst->stream_src_ip) > 0) {
        if(bind(s, (struct sockaddr*)localaddr->ai_addr, localaddr->ai_addrlen) < 0) {
            close(s);
            T_LOG(T_ERR, "Cannot bind [%s|%d] %s", inet_ntoa( ((struct sockaddr_in*)localaddr->ai_addr)->sin_addr ),
                ntohs(((struct sockaddr_in*)localaddr->ai_addr)->sin_port), strerror(errno));
            close(s);
            t_errno = T_ERR_STREAM_CONNECT;
            freeaddrinfo(localaddr);
            freeaddrinfo(serveraddr);
            return -1;
        } else {
            T_LOG(T_HINT, "bind local address - [%s|%d]", inet_ntoa(((struct sockaddr_in*)localaddr->ai_addr)->sin_addr),
                    ntohs(((struct sockaddr_in*)localaddr->ai_addr)->sin_port) );
        }
    }
    
    if (connect(s, (struct sockaddr *) serveraddr->ai_addr, serveraddr->ai_addrlen) < 0 && errno != EINPROGRESS) {
        T_LOG(T_ERR, "cannot connect [%s|%d] - %s", inet_ntoa(((struct sockaddr_in*)serveraddr->ai_addr)->sin_addr),
                ntohs(((struct sockaddr_in*)serveraddr->ai_addr)->sin_port), strerror(errno));
        close(s);
        t_errno = T_ERR_STREAM_CONNECT;
        freeaddrinfo(localaddr);
        freeaddrinfo(serveraddr);
        return -1;
    } else {
        T_LOG(T_HINT, "Protocol connected to - [name: %s | ip: %s | port: %d]",
            pst->stream_to_profile->selected_protocol->protocol_name,
            inet_ntoa(((struct sockaddr_in*)serveraddr->ai_addr)->sin_addr),
            ntohs(((struct sockaddr_in*)serveraddr->ai_addr)->sin_port));
    }
    
    /* send cookie through data channel for verification, not so sure about this */
    /* if (tcp_sendN(s, pst->stream_to_profile->id_buf, T_IDPKT_SIZE) < 0) {
        close(s);
        t_errno = T_ERR_SEND_COOKIE;
        freeaddrinfo(localaddr);
        freeaddrinfo(serveraddr);
        return -1;
    }
    T_LOG(T_HINT, "Cookie %s is sent", pst->stream_to_profile->id_buf);
    freeaddrinfo(localaddr);
    freeaddrinfo(serveraddr);
     */
    return s;
}

int tcp_recvN(int fd, char* buf, int count)
{
    int r = -1;
    int nleft = count;
    
    while (nleft > 0) {
        r = read(fd, buf, nleft);
        if (r < 0) {
            return -1;
        } else if (r == 0) {
            break;
        }
        nleft -= r;
        buf += r;
    }
    
    return count - nleft;
}

int tcp_sendN(int fd, char* buf, int count)
{
    int r;
    int nleft = count;
    
    while (nleft > 0) {
        r = write(fd, buf, nleft);
        if (r < 0) {
            return -1;
        } else if (r == 0) {
            break;
        }
        nleft -= r;
        buf += r;
    }
    
    return count;
}

int tcp_send(tpg_stream *pst)
{
    return (tcp_sendN(pst->stream_sockfd, pst->stream_blkbuf, pst->stream_to_profile->blocksize));
}

int tcp_recv(tpg_stream *pst)
{
    int r = 0, res;
    struct timeval tv;
    fd_set readfds;
    
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    
    FD_ZERO(&readfds);
    FD_SET(pst->stream_sockfd, &readfds);
    
    res = select(pst->stream_sockfd+1, &readfds, NULL, NULL, &tv);
    
    if (res > 0 && (FD_ISSET(pst->stream_sockfd, &readfds))) {
        if (errno == EINTR || errno == EBADF) {
            T_LOG(T_ERR, "TCP select EINTR or EBADF error");
            return -1;
        }
        r = (tcp_recvN(pst->stream_sockfd, pst->stream_blkbuf, pst->stream_to_profile->blocksize));
        if (r < 0) {
            return r;
        }
    }
    FD_CLR(pst->stream_sockfd, &readfds);
    return r;
}

int tcp_close(int* sockfd)
{
    if (*sockfd < 0) {
        return 0;
    }
    
    if (close(*sockfd) != 0) {
        T_LOG(T_ERR, "TCP cannot close socket");
        t_errno = T_ERR_SOCK_CLOSE_ERROR;
        return -1;
    } else {
        T_LOG(T_HINT, "Socket %d is closed", *sockfd);
        *sockfd = -1;
    }
    return 0;
}

const char* tcp_strerror()
{
    return (strerror(errno));
}

void tcp_client_perf(tpg_profile* prof)
{
    bool flagfirsttime = true;
    bool profile_state = false;
    double interval = 0.0;
    
    interval = prof->report_interval;
    
    while (prof->status == T_PROFILING_RUNNING) {
        usleep((int)(interval * T_ONE_MILLION));
        if (prof->accept_stream_num > 1) {
            if (prof->interval_report_flag > 0) {
                if (prof->is_multi_nic_enabled > 0) {
                    tcp_print_header_multi_nic();
                } else {
                    tcp_print_header();
                }
                tcp_print_interval(prof);
            } else if (flagfirsttime) {
                tcp_print_header();
                flagfirsttime = false;
            }
            tcp_print_sum(prof);
        } else {
            if (flagfirsttime && prof->interval_report_flag > 0) {
                tcp_print_header();
                flagfirsttime = false;
            }
            if (prof->interval_report_flag > 0) {
                tcp_print_interval(prof);
            }
        }
        
        pthread_mutex_lock(&(prof->client_finish_lock));
        profile_state = prof->client_finish_mark.all();
        pthread_mutex_unlock(&(prof->client_finish_lock));
        
        if (profile_state) {
            break;
        }
    }
}

void tcp_print_header()
{
    // may need more parameters about TCP,
    // to be added
    // printf("[ ID]  Elapsed(sec)   TotalSent    Goodput\n");
    T_LOG(T_PERF, "[ ID]  Elapsed(sec)  TotalSent  Goodput");
    //     "[123]  XXXX->XXXX     XXXXXXXX     XXXXXXXX\n"
}

void tcp_print_header_multi_nic()
{
    // may need more parameters about TCP,
    // to be added
    // printf("[ ID]  Elapsed(sec)   TotalSent    Goodput\n");
    T_LOG(T_PERF, "[ ID]  Elapsed(sec)  TotalSent  Goodput       NIC-to-NIC");
    //     "[123]  XXXX->XXXX     XXXXXXXX     XXXXXXXX\n"
}


void tcp_print_interval_stream(tpg_stream* pst)
{
    long long int start_t_us = 0;
    long long int end_t_us = 0;
    double total_sent_byte = 0.0;
    char total_sent_conv[11];
    double duration_t_us = 0.0;
    double goodput_byte_per_sec = 0.0;
    char goodput_conv[11];
    
    pst->stream_to_profile->end_time_us = pst->stream_result.end_time_usec = t_get_currtime_us();
    start_t_us = pst->stream_result.interval_time_usec - pst->stream_result.start_time_usec;
    end_t_us = pst->stream_result.end_time_usec - pst->stream_result.start_time_usec;
    pst->stream_result.interval_time_usec = t_get_currtime_us();
    duration_t_us = (end_t_us - start_t_us) * 1.0;
    total_sent_byte = 1.0 * (pst->stream_result.total_sent_bytes - pst->stream_result.interval_sent_bytes);
    pst->stream_result.interval_sent_bytes = pst->stream_result.total_sent_bytes;
    t_unit_format(total_sent_conv, 11, total_sent_byte, 'A');
    
    goodput_byte_per_sec = total_sent_byte / (duration_t_us / T_ONE_MILLION); // byte/sec
    t_unit_format(goodput_conv, 11, goodput_byte_per_sec, 'a');
    
    if (pst->stream_to_profile->is_multi_nic_enabled > 0) {
        T_LOG(T_PERF, "[%3d]  %-4lld->%4lld    %s   %s/s    (%s,%s)",
                pst->stream_id, start_t_us / T_ONE_MILLION, end_t_us / T_ONE_MILLION,
                total_sent_conv, goodput_conv, pst->stream_src_ip, pst->stream_dst_ip);
    } else {
        T_LOG(T_PERF, "[%3d]  %-4lld->%4lld    %s   %s/s",
            pst->stream_id, start_t_us / T_ONE_MILLION,
            end_t_us / T_ONE_MILLION, total_sent_conv, goodput_conv);
    }
}

void tcp_print_interval(tpg_profile* prof)
{
    int i;
    for (i = 0; i < (prof->accept_stream_num); i++) {
        tcp_print_interval_stream((prof->stream_list)[i]);
    }
}

void tcp_print_sum(tpg_profile* prof)
{
    int i = -1;
    long long int start_t_us;
    long long int end_t_us;
    double total_sent_byte = 0.0;
    char total_sent_conv[11];
    double duration_t_us = 0.0;
    double goodput_byte_per_sec = 0.0;
    char goodput_conv[11];
    
    prof->end_time_us = t_get_currtime_us();
    start_t_us = prof->start_time_us - prof->start_time_us;
    end_t_us = prof->end_time_us - prof->start_time_us;
    duration_t_us = (end_t_us - start_t_us) * 1.0;
    
    for(i = 0; i < prof->accept_stream_num; i++) {
        total_sent_byte += (prof->stream_list)[i]->stream_result.total_sent_bytes;
    }
    t_unit_format(total_sent_conv, 11, total_sent_byte, 'A');
    goodput_byte_per_sec = total_sent_byte / (duration_t_us / T_ONE_MILLION);
    t_unit_format(goodput_conv, 11, goodput_byte_per_sec, 'a');
    T_LOG(T_PERF, "[SUM]  %-4lld->%4lld    %s   %s/s",
        start_t_us / T_ONE_MILLION, end_t_us / T_ONE_MILLION,
        total_sent_conv, goodput_conv);
}