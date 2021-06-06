/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
//#include <set>
#include <udt.h>
#include "tpg_udt.h"
#include "tpg_log.h"

int udt_init(tpg_profile* prof)
{
    return 0;
}

/*
int udt_getsockname(tpg_stream* pstream)
{
    int len = 0;
    T_LOG("[debug]: getsockname by - [%d]\n", pstream->st_sockfd);
    if (UDT::ERROR == UDT::getsockname(pstream->st_sockfd,
            (struct sockaddr *)&(pstream->stream_localaddr), &len)) {
        t_errno = T_ERR_UDT_ERROR;
        return -1;
    }
    return 0;
}
*/

/*
int udt_getpeername(tpg_stream* pstream)
{
    int len = 0;
    if (UDT::ERROR == UDT::getpeername(pstream->st_sockfd,
            (struct sockaddr *) &(pstream->stream_peeraddr), &len)) {
        t_errno = T_ERR_UDT_ERROR;
        return -1;
    }
    return 0;
}
*/
int udt_listen(tpg_profile* prof)
{
    int s = -1;
    char portstr[8];
    struct addrinfo hints;
    struct addrinfo *localaddr;
    
    int set_udt_fc_opt = -1;
    int get_udt_fc = -1;
    int get_udt_fc_len = 0;
    
    int set_udt_mss_opt = -1;
    int get_udt_mss = -1;
    int get_udt_mss_len = 0;
    
    int set_udt_recvbuf_opt = -1;
    int get_udt_recvbuf = -1;
    int get_udt_recvbuf_len = 0;
    
    int set_udp_recvbuf_opt = -1;
    int get_udp_recvbuf = -1;
    int get_udp_recvbuf_len = 0;
    
    long long int set_udt_maxbw_opt = -1;
    long long int get_udt_maxbw = -1;
    int get_udt_maxbw_len = 0;
    
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    sprintf(portstr, "%d", prof->protocol_listen_port);
    if (getaddrinfo(NULL, portstr, &hints, &localaddr) != 0) {
        T_LOG(T_ERR, "getaddrinfo() error");
        freeaddrinfo(localaddr);
        return -1;
    }
    
    if (UDT::ERROR == (s = UDT::socket(AF_INET, SOCK_STREAM, 0))) {
        T_LOG(T_ERR, "Can not create UDT socket");
        t_errno = T_ERR_UDT_ERROR;
        freeaddrinfo(localaddr);
        return -1;
    }    
    T_LOG(T_HINT, "Listen on data channel - [name: %s | ip: %s | port: %d]",
            prof->selected_protocol->protocol_name,
            inet_ntoa( ((struct sockaddr_in*)localaddr->ai_addr)->sin_addr ),
            ntohs(((struct sockaddr_in*)localaddr->ai_addr)->sin_port) );
    
    if(UDT::ERROR == UDT::setsockopt(s, 0, UDT_REUSEADDR, new bool(false), sizeof(bool))) {
        T_LOG(T_ERR, "Cannot set UDT_REUSEADDR to be FALSE");
        t_errno = T_ERR_UDT_OPT_ERROR;
        freeaddrinfo(localaddr);
        UDT::close(s);
        return -1;
    } else {
        T_LOG(T_HINT, "Set SO_REUSEADDR to be FALSE");
    }
    
    // set for recv time out
    /*
     * How to set this timeout value is tricky
     */
    /*
    if (UDT::ERROR ==
        UDT::setsockopt(s, 0, UDT_RCVTIMEO, new int(19000), sizeof(int))) {
        t_errno = T_ERR_UDT_OPT_ERROR;
        freeaddrinfo(localaddr);
        UDT::close(s);
        return -1;
    } else {
        T_LOG("UDT data chan set socket option UDT_RCVTIMEO - [%d] sec\n", 5);
    }*/
    /*
     * see http://udt.sourceforge.net/udt4/doc/t-config.htm for how to set UDT_FC
     */
    if (prof->udt_mss > 0) {
        get_udt_recvbuf = -1;
        get_udt_recvbuf_len = sizeof(int);
        if (UDT::ERROR == UDT::getsockopt(s, 0, UDT_RCVBUF, &get_udt_recvbuf, &get_udt_recvbuf_len)) {
            T_LOG(T_ERR, "Cannot get UDT_RCVBUF option");
            t_errno = T_ERR_UDT_OPT_ERROR;
            freeaddrinfo(localaddr);
            UDT::close(s);
            return -1;
        } else {
            T_LOG(T_HINT, "Get current UDT_RCVBUF %d", get_udt_recvbuf);
        }
        
        get_udt_fc = -1;
        get_udt_fc_len = sizeof(int);
        if (UDT::ERROR == UDT::getsockopt(s, 0, UDT_FC, &get_udt_fc, &get_udt_fc_len)) {
            T_LOG(T_ERR, "Cannot get UDT_FC option");
            t_errno = T_ERR_UDT_OPT_ERROR;
            freeaddrinfo(localaddr);
            UDT::close(s);
            return -1;
        } else {
            T_LOG(T_HINT, "Get current UDT_FC [%d]", get_udt_fc);
        }
        
        if (get_udt_fc < (get_udt_recvbuf / prof->udt_mss )) {
            set_udt_fc_opt = (int)((get_udt_recvbuf / prof->udt_mss) + 1);
            if (UDT::ERROR == UDT::setsockopt(s, 0, UDT_FC, &set_udt_fc_opt, sizeof(int))) {
                T_LOG(T_ERR, "Cannot set UDT_FC");
                t_errno = T_ERR_UDT_OPT_ERROR;
                freeaddrinfo(localaddr);
                UDT::close(s);
                return -1;
            } else {
                get_udt_fc = -1;
                get_udt_fc_len = sizeof(int);
                if (UDT::ERROR == UDT::getsockopt(s, 0, UDT_FC, &get_udt_fc, &get_udt_fc_len)) {
                    T_LOG(T_ERR, "Get UDT_FC failure");
                } else {
                    T_LOG(T_HINT, "UDT_FC is updated to be %d", get_udt_fc);
                }
            }
        } else {
            T_LOG(T_HINT, "UDT_FC's value is ok, no need to change");
        }
        
        set_udt_mss_opt = prof->udt_mss;
        if (UDT::ERROR == UDT::setsockopt(s, 0, UDT_MSS, &set_udt_mss_opt, sizeof(int))) {
            T_LOG(T_ERR, "Cannot set UDT_MSS %d", set_udt_mss_opt);
            t_errno = T_ERR_UDT_OPT_ERROR;
            freeaddrinfo(localaddr);
            UDT::close(s);
            return -1;
        } else {
            get_udt_mss = -1;
            get_udt_mss_len = sizeof(int);
            if (UDT::ERROR == UDT::getsockopt(s, 0, UDT_MSS, &get_udt_mss, &get_udt_mss_len)) {
                T_LOG(T_ERR, "Cannot get UDT_MSS");
            } else {
                T_LOG(T_HINT, "UDT_MSS is updated to be %d", get_udt_mss);
            }
        }
    }
    
	if (prof->udt_recv_bufsize > 0) {
            get_udt_mss = -1;
            get_udt_mss_len = sizeof(int);
            if (UDT::ERROR == UDT::getsockopt(s, 0, UDT_MSS, &get_udt_mss, &get_udt_mss_len)) {
                T_LOG(T_ERR, "Cannot get UDT_MSS");
                t_errno = T_ERR_UDT_OPT_ERROR;
                UDT::close(s);
                freeaddrinfo(localaddr);
                return -1;
            } else {
                T_LOG(T_HINT, "Get UDT_MSS %d", get_udt_mss);
            }
            
            get_udt_fc = -1;
            get_udt_fc_len = sizeof(int);
            if (UDT::ERROR == UDT::getsockopt(s, 0, UDT_FC, &get_udt_fc, &get_udt_fc_len)) {
                T_LOG(T_ERR, "Cannot get UDT_FC");
                t_errno = T_ERR_UDT_OPT_ERROR;
                UDT::close(s);
                freeaddrinfo(localaddr);
                return -1;
            } else {
                T_LOG(T_HINT, "Get UDT_FC %d", get_udt_fc);
            }
            
            if (get_udt_fc < (prof->udt_recv_bufsize / get_udt_mss)) {
                set_udt_fc_opt = (int)((prof->udt_recv_bufsize / get_udt_mss) + 1);
                if (UDT::ERROR == UDT::setsockopt(s, 0, UDT_FC, &set_udt_fc_opt, sizeof(int))) {
                    T_LOG(T_ERR, "Cannot set UDT_FC");
                    t_errno = T_ERR_UDT_OPT_ERROR;
                    UDT::close(s);
                    freeaddrinfo(localaddr);
                    return -1;
                } else {
                    get_udt_fc = -1;
                    get_udt_fc_len = sizeof(int);
                    if(UDT::ERROR == UDT::getsockopt(s, 0, UDT_FC, &get_udt_fc, &get_udt_fc_len)) {
                        T_LOG(T_ERR, "Cannot get UDT_FC failure");
                    } else {
                        T_LOG(T_HINT, "UDT_FC is updated to be %d", get_udt_fc);
                    }
                }
            } else {
                T_LOG(T_HINT, "UDT_FC's value is ok, no need to change");
            }
            
            set_udt_recvbuf_opt = prof->udt_recv_bufsize;
            if (UDT::ERROR == UDT::setsockopt(s, 0, UDT_RCVBUF, &set_udt_recvbuf_opt, sizeof(int))) {
                T_LOG(T_ERR, "Cannot set UDT_RCVBUF");
                t_errno = T_ERR_UDT_OPT_ERROR;
                UDT::close(s);
                freeaddrinfo(localaddr);
                return -1;
            } else {
                get_udt_recvbuf = -1;
                get_udt_recvbuf_len = sizeof(int);
                if (UDT::ERROR == UDT::getsockopt(s, 0, UDT_RCVBUF, &get_udt_recvbuf, &get_udt_recvbuf_len)) {
                    T_LOG(T_ERR, "Cannot set UDT_RCVBUF");
                } else {
                    T_LOG(T_HINT, "UDT_RCVBUF is updated to be %d", get_udt_recvbuf);
                }
            }
	}
    
    if (prof->udp_recv_bufsize > 0) {
        set_udp_recvbuf_opt = prof->udp_recv_bufsize;
        if (UDT::ERROR == UDT::setsockopt(s, 0, UDP_RCVBUF, &set_udp_recvbuf_opt, sizeof(int))) {
            T_LOG(T_ERR, "Cannot set UDP_RCVBUF");
            t_errno = T_ERR_UDT_OPT_ERROR;
            UDT::close(s);
            freeaddrinfo(localaddr);
            return -1;
        } else {
            get_udp_recvbuf = -1;
            get_udp_recvbuf_len = sizeof(int);
            if (UDT::ERROR == UDT::getsockopt(s, 0, UDP_RCVBUF, &get_udp_recvbuf, &get_udp_recvbuf_len)) {
                T_LOG(T_ERR, "Cannot get UDP_RCVBUF");
            } else {
                T_LOG(T_HINT,
                        "UDP_RCVBUF is updated to be %d", get_udp_recvbuf);
            }
        }
    }
    
    if (prof->udt_maxbw > 0.0) {
        set_udt_maxbw_opt = (long long int)( floor( (prof->udt_maxbw / 8.0) + 0.5 ) );
        if (UDT::ERROR == UDT::setsockopt(s, 0, UDT_MAXBW, &set_udt_maxbw_opt, sizeof(long long int))) {
            T_LOG(T_ERR, "Cannot set UDT_MAXBW");
            t_errno = T_ERR_UDT_OPT_ERROR;
            UDT::close(s);
            freeaddrinfo(localaddr);
            return -1;
        } else {
            get_udt_maxbw = -1;
            get_udt_maxbw_len = sizeof(long long int);
            if (UDT::ERROR == UDT::getsockopt(s, 0, UDT_MAXBW, &get_udt_maxbw, &get_udt_maxbw_len))	{
                T_LOG(T_ERR, "Cannot set UDT_MAXBW");
            } else {
                T_LOG(T_HINT, "UDT_MAXBW is updated to be %lld", get_udt_maxbw);
            }
        }
    }
    
    if (UDT::ERROR == UDT::bind(s, (struct sockaddr *) localaddr->ai_addr, localaddr->ai_addrlen)) {
        T_LOG(T_ERR, "UDT bind error - %s", UDT::getlasterror().getErrorMessage());
        UDT::close(s);
        t_errno = T_ERR_UDT_ERROR;
        freeaddrinfo(localaddr);
        return -1;
    } else {
        T_LOG(T_HINT, "UDT bind local address - [%s|%d]",
                inet_ntoa(((struct sockaddr_in*)localaddr->ai_addr)->sin_addr),
                ntohs(((struct sockaddr_in*)localaddr->ai_addr)->sin_port) );
    }
    
    if (UDT::ERROR == UDT::listen(s, T_MAX_BACKLOG)) {
        T_LOG(T_ERR, "UDT listen error - [%s]", UDT::getlasterror().getErrorMessage());
        UDT::close(s);
        freeaddrinfo(localaddr);
        t_errno = T_ERR_UDT_ERROR;
        return -1;
    } else {
        T_LOG(T_HINT, "Protocol listen on - [name: %s | ip: %s | port: %d]",
                prof->selected_protocol->protocol_name,
                inet_ntoa(((struct sockaddr_in*)localaddr->ai_addr)->sin_addr),
                ntohs(((struct sockaddr_in*)localaddr->ai_addr)->sin_port));
    }
    
    prof->protocol_listen_sock = s;
    
    freeaddrinfo(localaddr);
    return 0;
}

int udt_connect(tpg_stream* pst)
{
    int s = -1;
    char portstr[6];
    struct addrinfo hints;
    struct addrinfo *localaddr = NULL;
    struct addrinfo *serveraddr = NULL;
    
    int set_udt_mss_opt = -1;
    int get_udt_mss = -1;
    int get_udt_mss_len = 0;
    
    int set_udt_sendbuf_opt = -1;
    int get_udt_sendbuf = -1;
    int get_udt_sendbuf_len = 0;
    
    int set_udp_sendbuf_opt = -1;
    int get_udp_sendbuf = -1;
    int get_udp_sendbuf_len = 0;
    
    long long int set_udt_maxbw_opt = -1;
    long long int get_udt_maxbw = -1;
    int get_udt_maxbw_len = 0;
    
    T_LOG(T_HINT, "Connecting - [from: %s to: %s at port: %d (dst)]",
            pst->stream_src_ip, 
            pst->stream_dst_ip, 
            pst->stream_dst_port);
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
#ifdef ANL_UCHICAGO
    pst->stream_src_port = T_DEFAULT_UDT_PORT;
#endif
    if (pst->stream_src_port > 0) {
        sprintf(portstr, "%d", pst->stream_src_port);
    } else {
        sprintf(portstr, "%d", 0);
    }
    if(strlen(pst->stream_src_ip) > 0) {
        if (getaddrinfo(pst->stream_src_ip, portstr, &hints, &localaddr) != 0) {
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
    sprintf(portstr, "%d", pst->stream_dst_port);
    if(strlen(pst->stream_dst_ip) > 0) {
        if (getaddrinfo(pst->stream_dst_ip, portstr, &hints, &serveraddr) != 0) {
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
    
    if (UDT::ERROR == (s = UDT::socket(AF_INET, SOCK_STREAM, 0))) {
        UDT::close(s);
        T_LOG(T_ERR, "UDT socket error");
        t_errno = T_ERR_UDT_ERROR;
        freeaddrinfo(localaddr);
        freeaddrinfo(serveraddr);
        return -1;
    }
    
    if (pst->stream_to_profile->total_stream_num > 1) {
        if (pst->stream_to_profile->is_multi_nic_enabled > 0 && pst->stream_src_port > 0 && strlen(pst->stream_to_profile->local_ip) > 0) {
            t_errno = T_ERR_BIND_CONFLICT;
            return -1;
        } else {
            if(UDT::ERROR == UDT::setsockopt(s, 0, UDT_REUSEADDR, new bool(true), sizeof(bool))) {
                T_LOG(T_ERR, "Cannot set UDT_REUSEADDR to be TRUE");
                t_errno = T_ERR_UDT_OPT_ERROR;
                freeaddrinfo(localaddr);
                UDT::close(s);
                return -1;
            } else {
                T_LOG(T_HINT, "Set SO_REUSEADDR to be TRUE");
            }
        }
    } else {
        if(UDT::ERROR == UDT::setsockopt(s, 0, UDT_REUSEADDR, new bool(false), sizeof(bool))) {
            T_LOG(T_ERR, "Cannot set UDT_REUSEADDR to be FALSE");
            t_errno = T_ERR_UDT_OPT_ERROR;
            freeaddrinfo(localaddr);
            UDT::close(s);
            return -1;
        } else {
            T_LOG(T_HINT, "Set SO_REUSEADDR to be FALSE");
        }
    }
    
    if (pst->stream_to_profile->udt_mss > 0) {
        set_udt_mss_opt = pst->stream_to_profile->udt_mss;
        if (UDT::ERROR == UDT::setsockopt(s, 0, UDT_MSS, &set_udt_mss_opt, sizeof(int))) {
            T_LOG(T_ERR, "Cannot set UDT_MSS %s", UDT::getlasterror().getErrorMessage());
            UDT::close(s);
            t_errno = T_ERR_UDT_OPT_ERROR;
            freeaddrinfo(localaddr);
            freeaddrinfo(serveraddr);
            return -1;
        } else {
            get_udt_mss = -1;
            get_udt_mss_len = sizeof(int);
            if (UDT::ERROR == UDT::getsockopt(s, 0, UDT_MSS, &get_udt_mss, &get_udt_mss_len)) {
                T_LOG(T_ERR, "Cannot get UDT_MSS");
            } else {
                T_LOG(T_HINT, "UDT_MSS is updated to be %d", get_udt_mss);
            }
        }
    }
    
    if (pst->stream_to_profile->udt_send_bufsize > 0) {
        set_udt_sendbuf_opt = pst->stream_to_profile->udt_send_bufsize;
        if (UDT::ERROR == UDT::setsockopt(s, 0, UDT_SNDBUF, &set_udt_sendbuf_opt, sizeof(int))) {
            T_LOG(T_ERR, "Cannot set UDT_SNDBUF %s", UDT::getlasterror().getErrorMessage());
            UDT::close(s);
            t_errno = T_ERR_UDT_OPT_ERROR;
            freeaddrinfo(localaddr);
            freeaddrinfo(serveraddr);
            return -1;
        } else {
            get_udt_sendbuf = -1;
            get_udt_sendbuf_len = sizeof(int);
            if (UDT::ERROR == UDT::getsockopt(s, 0, UDT_SNDBUF, &get_udt_sendbuf, &get_udt_sendbuf_len)) {
                T_LOG(T_ERR, "Cannot get UDT_SNDBUF");
            } else {
                T_LOG(T_HINT, "UDT_SNDBUF is updated to be %d", get_udt_sendbuf);
            }
        }
    }
    
    if (pst->stream_to_profile->udp_send_bufsize > 0) {
        set_udp_sendbuf_opt = pst->stream_to_profile->udp_send_bufsize;
        if (UDT::ERROR == UDT::setsockopt(s, 0, UDP_SNDBUF, &set_udp_sendbuf_opt, sizeof(int))) {
            T_LOG(T_ERR, "Cannot set UDP_SNDBUF %s", UDT::getlasterror().getErrorMessage());
            UDT::close(s);
            t_errno = T_ERR_UDT_OPT_ERROR;
            freeaddrinfo(localaddr);
            freeaddrinfo(serveraddr);
            return -1;
        } else {
            get_udp_sendbuf = -1;
            get_udp_sendbuf_len = sizeof(int);
            if (UDT::ERROR == UDT::getsockopt(s, 0, UDP_SNDBUF, &get_udp_sendbuf, &get_udp_sendbuf_len)) {
                T_LOG(T_ERR, "Cannot get UDP_SNDBUF");
            } else {
                T_LOG(T_HINT, "UDP_SNDBUF is updated to be %d", get_udp_sendbuf);
            }
        }
    }
    
    if (pst->stream_to_profile->udt_maxbw > 0.0) {
        set_udt_maxbw_opt = (long long int)(floor((pst->stream_to_profile->udt_maxbw / 8.0) + 0.5));
        if (UDT::ERROR == UDT::setsockopt(s, 0, UDT_MAXBW, &set_udt_maxbw_opt, sizeof(long long int))) {
            T_LOG(T_ERR, "Cannot set UDT_MAXBW %s", UDT::getlasterror().getErrorMessage());
            UDT::close(s);
            t_errno = T_ERR_UDT_OPT_ERROR;
            freeaddrinfo(localaddr);
            freeaddrinfo(serveraddr);
            return -1;
        } else {
            get_udt_maxbw = -1;
            get_udt_maxbw_len = sizeof(long long int);
            if (UDT::ERROR == UDT::getsockopt(s, 0, UDT_MAXBW, &get_udt_maxbw, &get_udt_maxbw_len)) {
                T_LOG(T_ERR, "Cannot get UDT_MAXBW");
            } else {
                T_LOG(T_HINT, "UDT_MAXBW is updated to be %lld", get_udt_maxbw);
            }
        }
    }

    if (strlen(pst->stream_src_ip) > 0) {
        if (UDT::ERROR == UDT::bind(s, (struct sockaddr *) localaddr->ai_addr, localaddr->ai_addrlen)) {
            T_LOG(T_ERR, "UDT cannot bind [%s|%d] %s", inet_ntoa(((struct sockaddr_in*)localaddr->ai_addr)->sin_addr),
                ntohs(((struct sockaddr_in*)localaddr->ai_addr)->sin_port), UDT::getlasterror().getErrorMessage());
            UDT::close(s);
            t_errno = T_ERR_UDT_ERROR;
            freeaddrinfo(localaddr);
            freeaddrinfo(serveraddr);
            return -1;
        } else {
            T_LOG(T_HINT, "UDT bind local address - [%s|%d]",
                inet_ntoa(((struct sockaddr_in*)localaddr->ai_addr)->sin_addr),
                ntohs(((struct sockaddr_in*)localaddr->ai_addr)->sin_port));
        }
    }
    
    T_LOG(T_HINT, "UDT I am connecting...");
    T_LOG(T_HINT, "Protocol from - [name: %s | ip: %s | port: %d]",
        pst->stream_to_profile->selected_protocol->protocol_name,
        inet_ntoa(((struct sockaddr_in*)localaddr->ai_addr)->sin_addr),
        ntohs(((struct sockaddr_in*)localaddr->ai_addr)->sin_port));
    T_LOG(T_HINT, "Protocol connecting to - [name: %s | ip: %s | port: %d]",
        pst->stream_to_profile->selected_protocol->protocol_name,
        inet_ntoa(((struct sockaddr_in*)serveraddr->ai_addr)->sin_addr),
        ntohs(((struct sockaddr_in*)serveraddr->ai_addr)->sin_port));

    if (UDT::ERROR == UDT::connect(s, (struct sockaddr*) serveraddr->ai_addr, serveraddr->ai_addrlen)) {
        T_LOG(T_ERR, "UDT cannot connect [%s|%d] - %s", inet_ntoa(((struct sockaddr_in*)serveraddr->ai_addr)->sin_addr),
                ntohs(((struct sockaddr_in*)serveraddr->ai_addr)->sin_port), UDT::getlasterror().getErrorMessage());
        UDT::close(s);
        t_errno = T_ERR_UDT_ERROR;
        freeaddrinfo(localaddr);
        freeaddrinfo(serveraddr);
        return -1;
    } else {
        T_LOG(T_HINT, "Protocol connected to - [name: %s | ip: %s | port: %d]",
                pst->stream_to_profile->selected_protocol->protocol_name,
                inet_ntoa(((struct sockaddr_in*)serveraddr->ai_addr)->sin_addr),
                ntohs(((struct sockaddr_in*)serveraddr->ai_addr)->sin_port));
    }
    
    freeaddrinfo(localaddr);
    freeaddrinfo(serveraddr);
    
    return s;
}

int udt_accept(tpg_profile* prof, int ms_timeout)
{
    int s = -1;
    int ret = -1;
    int len = -1;

    struct sockaddr_in addr;
    
    T_LOG(T_HINT, "UDT I am accepting %d", ms_timeout);

    int eid = -1;
    std::set<UDTSOCKET> readfds;
    readfds.clear();
    if ( (eid = UDT::epoll_create()) < 0) {
        T_LOG(T_ERR, "UDT cannot create epoll");
        t_errno = T_ERR_UDT_ERROR;
        return -1;
    }
    if (UDT::epoll_add_usock(eid, prof->protocol_listen_sock) < 0) {
        T_LOG(T_ERR, "UDT cannot add epoll usock");
        t_errno = T_ERR_UDT_ERROR;
        return -1;
    }
    
    if ( (ret = UDT::epoll_wait(eid, &readfds, NULL, ms_timeout, NULL, NULL)) == UDT::ERROR) {
        if (UDT::getlasterror_code() == CUDTException::ETIMEOUT) {
            T_LOG(T_ERR, "UDT epoll timeout"); 
            return 0;
        } else {
            T_LOG(T_ERR, "UDT epoll error");
            t_errno = T_ERR_UDT_ERROR;
            return -1;
        }
    }
    
    len = sizeof(addr);
    if (UDT::INVALID_SOCK == (s = UDT::accept(prof->protocol_listen_sock, (struct sockaddr *) &addr, &len))) {
        T_LOG(T_ERR, "UDT accept error [%d]", UDT::getlasterror_code());
        t_errno = T_ERR_UDT_ERROR;
        return -1;
    } else {
        T_LOG(T_HINT, "Accepted client [name: %s | ip: %s | port: %d]",
                prof->selected_protocol->protocol_name,
                inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    }
    
    if (UDT::epoll_remove_usock(eid, prof->protocol_listen_sock) < 0) {
        T_LOG(T_ERR, "epoll cannot release usock");
        t_errno = T_ERR_UDT_ERROR;
        return -1;
    }
    if (UDT::epoll_release(eid) < 0) {
        T_LOG(T_ERR, "epoll cannot release usock");
        t_errno = T_ERR_UDT_ERROR;
        return -1;
    }
    
    return s;
}


int udt_recvN(int fd, char* buf, int count)
{
    int ret;
    int nleft = count;
    
    while (nleft > 0) {
        ret = UDT::recv(fd, buf, nleft, 0);
        if (UDT::ERROR == ret) {
            T_LOG(T_ERR, "UDT receive N bytes error %d", UDT::getlasterror_code());
            t_errno = T_ERR_UDT_RECV_ERROR;
            return -1;
        }
        if (ret < 0) {
            T_LOG(T_ERR, "UDT receive N bytes error %d", UDT::getlasterror_code());
            t_errno = T_ERR_UDT_RECV_ERROR;
            return -1;
        } else if (ret == 0) {
            T_LOG(T_ERR, "UDT receive N bytes error %d", UDT::getlasterror_code());
            t_errno = T_ERR_UDT_ERROR;
            return 0;
        }
        nleft -= ret;
        buf += ret;
    }
    return (count - nleft);
}


int udt_sendN(int fd, char* buf, int count)
{
    int ret;
    int nleft = count;
    
    while (nleft > 0) {
        ret = UDT::send(fd, buf, nleft, 0);
        if (UDT::ERROR == ret) {
            T_LOG(T_ERR, "UDT send N bytes error [%d] on socket id [%d]", UDT::getlasterror_code(), fd);
            t_errno = T_ERR_UDT_ERROR;
            return -1;
        }
	if (ret < 0) {
            T_LOG(T_ERR, "UDT send N bytes error [%d] on socket id [%d]", UDT::getlasterror_code(), fd);
            t_errno = T_ERR_UDT_ERROR;
            return -1;
        } else if (ret == 0) {
            T_LOG(T_ERR, "UDT send N bytes error [%d] on socket id [%d]", UDT::getlasterror_code(), fd);
            t_errno = T_ERR_UDT_ERROR;
            return 0;
        }
        nleft -= ret;
        buf += ret;
    }
    
    return (count - nleft);
}


int udt_send(tpg_stream* pst)
{
    return (udt_sendN(pst->stream_sockfd, pst->stream_blkbuf, pst->stream_to_profile->blocksize));
}

int udt_recv(tpg_stream* pst)
{
    return udt_recvN(pst->stream_sockfd, pst->stream_blkbuf, pst->stream_to_profile->blocksize);
}

int udt_close(int* sockfd)
{
    if (*sockfd < 0) {
        return 0;
    }
    if (UDT::close(*sockfd) != 0) {
        T_LOG(T_ERR, "UDT cannot close socket with id %d", *sockfd);
        t_errno = T_ERR_UDT_ERROR;
        return -1;
    } else {
        T_LOG(T_HINT, "Socket %d is closed", *sockfd);
        *sockfd = -1;
    }
    return 0;
}

const char* udt_strerror()
{
    return (t_errstr(T_ERR_UDT_ERROR));
}

void udt_client_perf(tpg_profile* prof)
{
    bool flagfirsttime = true;
    bool profile_state = false;
    double intval = 0.0;
    intval = prof->report_interval;
    
    T_LOG(T_HINT, "udt_client_perf() thread is starting...");
    
    while (prof->status == T_PROFILING_RUNNING) {
        usleep((int)(intval * T_ONE_MILLION));
        if (prof->accept_stream_num > 1) {
            if (prof->interval_report_flag > 0) {
                if (prof->is_multi_nic_enabled > 0) {
                    udt_print_header_multi_nic();
                } else {
                    udt_print_header();
                }
                udt_print_interval(prof);
            } else if (flagfirsttime) {
                udt_print_shortheader();
                flagfirsttime = false;
            }
            udt_print_sum(prof);
        } else {
            if (flagfirsttime && prof->interval_report_flag > 0) {
                udt_print_header();
                flagfirsttime = false;
            }
            if (prof->interval_report_flag > 0) {
                udt_print_interval(prof);
            }
        }
        
        pthread_mutex_lock(&(prof->client_finish_lock));
        profile_state = prof->client_finish_mark.all();
        pthread_mutex_unlock(&(prof->client_finish_lock));
        
        if (profile_state) {
            break;
        }
        
    }
    
    T_LOG(T_HINT, "udt_client_perf() thread is exitting...");

    return ;
}

void udt_print_header()
{
    T_LOG(T_PERF, "[ ID]  Elapsed(sec)  TotalSent  Goodput     FlightSize  SndRate     Loss  RecvACK  RecvNAK  RTT(ms)  EstBandWidth");
}

void udt_print_header_multi_nic()
{
    T_LOG(T_PERF, "[ ID]  Elapsed(sec)  TotalSent  Goodput       NIC-to-NIC");
}

void udt_print_shortheader()
{
    T_LOG(T_PERF, "[ ID]  Elapsed(sec)  TotalSent  Goodput");
}

void udt_print_interval_stream(tpg_stream *pst)
{
    long long int start_t_us = 0;
    long long int end_t_us = 0;
    int flight_size = 0;
    char flight_size_conv[11];
    double mbps_send_rate = 0.0;
    char send_rate_conv[11];
    double total_sent_bytes = 0.0;
    char total_sent_conv[11];
    double duration_t_us = 0.0;
    double goodput_byte_per_sec = 0.0;
    char goodput_conv[11];
    int send_loss = 0;
    int recvack = 0;
    int recvnak = 0;
    double rtt_ms = 0.0;
    double estimateBW_mbps = 0.0;
    char estimateBW_conv[11];
    
    UDTSOCKET udtsock = 0;
    UDT::TRACEINFO info;
    
    pst->stream_to_profile->end_time_us = pst->stream_result.end_time_usec = t_get_currtime_us();
    start_t_us = pst->stream_result.interval_time_usec - pst->stream_result.start_time_usec;
    end_t_us =  pst->stream_result.end_time_usec - pst->stream_result.start_time_usec;
    pst->stream_result.interval_time_usec = pst->stream_result.end_time_usec;
    duration_t_us = (end_t_us - start_t_us) * 1.0;
    total_sent_bytes = (pst->stream_result.total_sent_bytes - pst->stream_result.interval_sent_bytes) * 1.0;
    pst->stream_result.interval_sent_bytes = pst->stream_result.total_sent_bytes;
    t_unit_format(total_sent_conv, 11, total_sent_bytes, 'A');
    goodput_byte_per_sec = total_sent_bytes / (duration_t_us / T_ONE_MILLION);
    t_unit_format(goodput_conv, 11, goodput_byte_per_sec, 'a');
    if (pst->stream_to_profile->is_multi_nic_enabled <= 0) {
        udtsock = pst->stream_sockfd;
        if (UDT::ERROR == UDT::perfmon(udtsock, &info)) {
            t_errno = T_ERR_UDT_PERF_ERROR;
            T_LOG(T_ERR, "UDT perfmon error %s", UDT::getlasterror().getErrorMessage());
            return ;
        }        
        flight_size = info.pktFlightSize * pst->stream_to_profile->udt_mss;
        t_unit_format(flight_size_conv, 11, (double) (flight_size*1.0), 'A');
        send_loss = info.pktSndLoss;
        recvack = info.pktRecvACK;
        recvnak = info.pktRecvNAK;
        mbps_send_rate = info.mbpsSendRate;
        t_unit_format(send_rate_conv, 11, mbps_send_rate*1024*1024 / 8.0, 'a');
        rtt_ms = info.msRTT;
        estimateBW_mbps = info.mbpsBandwidth;
        t_unit_format(estimateBW_conv, 11, estimateBW_mbps*1024*1024 / 8.0, 'a');
        T_LOG(T_PERF, "[%3d]  %-4lld->%4lld    %s   %s/s  %s    %s/s  %-4d  %-7d  %-7d  %-6.1lf   %-s/s",
                pst->stream_id,
                start_t_us / T_ONE_MILLION, end_t_us / T_ONE_MILLION,
                total_sent_conv,
                goodput_conv,
                flight_size_conv,
                send_rate_conv,
                send_loss,
                recvack,
                recvnak,
                rtt_ms,
                estimateBW_conv);
    } else {
        T_LOG(T_PERF, "[%3d]  %-4lld->%4lld    %s   %s/s    (%s,%s)",
                pst->stream_id,
                start_t_us / T_ONE_MILLION, end_t_us / T_ONE_MILLION,
                total_sent_conv,
                goodput_conv,
                pst->stream_src_ip,
                pst->stream_dst_ip);
    }
}

void udt_print_interval(tpg_profile* prof)
{
    int i;
    for (i = 0; i < (prof->accept_stream_num); i++) {
        udt_print_interval_stream((prof->stream_list)[i]);
    }
}

void udt_print_sum(tpg_profile* prof)
{
    int i = -1;
    long long int start_t_us;
    long long int end_t_us;
    double total_sent_byte = 0.0;
    char total_sent_conv[11];
    double duration_t_us = 0.0;
    double goodput_byte_per_sec = 0.0;
    char goodput_conv[11];
    
    start_t_us = 0;
    end_t_us = (t_get_currtime_us() - prof->start_time_us);
    duration_t_us = (end_t_us - start_t_us) * 1.0;
    for(i = 0; i < prof->accept_stream_num; i++) {
        total_sent_byte += (prof->stream_list)[i]->stream_result.total_sent_bytes;
    }
    t_unit_format(total_sent_conv, 11, total_sent_byte, 'A');
    goodput_byte_per_sec = total_sent_byte / (duration_t_us / T_ONE_MILLION);
    t_unit_format(goodput_conv, 11, goodput_byte_per_sec, 'a');
    T_LOG(T_PERF, "[SUM]  %-4lld->%4lld    %s   %s/s", start_t_us / T_ONE_MILLION, end_t_us / T_ONE_MILLION, total_sent_conv, goodput_conv);
}
