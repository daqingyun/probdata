/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <fstream>
#include <ctime>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "tpg_ctrl.h"
#include "tpg_log.h"

int ctrl_chan_listen(tpg_profile* prof)
{
    int s = -1;
    int opt = -1;
    char portstr[6];
    struct addrinfo hints;
    struct addrinfo *localaddr = NULL;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    sprintf(portstr, "%d", prof->ctrl_listen_port);
    if(strlen(prof->local_ip) > 0) {
        if (getaddrinfo(prof->local_ip, portstr, &hints, &localaddr) != 0) {
            T_LOG(T_ERR, "error: getaddrinfo %s", strerror(errno));
            freeaddrinfo(localaddr);
            return -1;
        }
    } else {
        if (getaddrinfo(NULL, portstr, &hints, &localaddr) != 0) {
            T_LOG(T_ERR, "error: getaddrinfo %s", strerror(errno));
            freeaddrinfo(localaddr);
            return -1;
        }
    }
    
    if ((s = socket(localaddr->ai_family, localaddr->ai_socktype, 0)) < 0) {
        T_LOG(T_ERR, "error: socket create failure %s", strerror(errno));
        freeaddrinfo(localaddr);
        return -1;
    }
    
    T_LOG(T_HINT, "local address of control channel [%s|%d]",
            inet_ntoa( ((struct sockaddr_in*)localaddr->ai_addr)->sin_addr ),
            ntohs( ((struct sockaddr_in*)localaddr->ai_addr)->sin_port) );
    
    opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,	(char*)&opt, sizeof(opt)) < 0){
        close(s);
        freeaddrinfo(localaddr);
        T_LOG(T_ERR, "error: ctrlchan set option SO_REUSEADDR error");
        return -1;
    }
    
    if(bind(s,(struct sockaddr*)(localaddr->ai_addr),localaddr->ai_addrlen)<0){
        close(s);
        freeaddrinfo(localaddr);
        T_LOG(T_ERR, "error: control channel socket bind error %s", strerror(errno));
        return -1;
    } else {
        T_LOG(T_HINT, "debug: bind local address - [%s|%d]",
            inet_ntoa(((struct sockaddr_in*)localaddr->ai_addr)->sin_addr),
            ntohs(((struct sockaddr_in*)localaddr->ai_addr)->sin_port));
    }
    
    if (listen(s, T_MAX_BACKLOG) < 0) {
        close(s);
        freeaddrinfo(localaddr);
        T_LOG(T_HINT, "error: control channel socket listen error");
        return -1;
    } else {
        T_LOG(T_HINT, "debug: control channel listen on - [%s|%d]",
            inet_ntoa( ((struct sockaddr_in*)localaddr->ai_addr)->sin_addr ),
            ntohs(((struct sockaddr_in*)localaddr->ai_addr)->sin_port) );
    }
    prof->ctrl_listen_sock = s;
    freeaddrinfo(localaddr);
    return 0;
}

int ctrl_chan_connect(tpg_profile* prof)
{
    int s = -1;
    char portstr[6];
    struct addrinfo hints;
    struct addrinfo *localaddr = NULL;
    struct addrinfo *serveraddr = NULL;
    
    T_LOG(T_HINT, "sending connection request - [%s|%s|%d]", prof->local_ip, prof->remote_ip, prof->ctrl_listen_port);
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if(strlen(prof->local_ip) > 0) {
        if (getaddrinfo(prof->local_ip, "0", &hints, &localaddr) != 0) {
            T_LOG(T_ERR, "error: getaddrinfo %s", strerror(errno));
            freeaddrinfo(localaddr);
            return -1;
        }
    } else {
        if (getaddrinfo(NULL, "0", &hints, &localaddr) != 0) {
            T_LOG(T_ERR, "error: getaddrinfo %s", strerror(errno));
            freeaddrinfo(localaddr);
            return -1;
        }
    }
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    sprintf(portstr, "%d", prof->ctrl_listen_port);
    if(strlen(prof->remote_ip) > 0)	{
        if (getaddrinfo(prof->remote_ip, portstr, &hints, &serveraddr) != 0) {
            T_LOG(T_ERR, "error: getaddrinfo %s", strerror(errno));
            freeaddrinfo(localaddr);
            freeaddrinfo(serveraddr);
            return -1;
        }
    } else {
        T_LOG(T_PERF, "Invalid server address");
        freeaddrinfo(localaddr);
        freeaddrinfo(serveraddr);
        return -1;
    }
    
    s = socket(serveraddr->ai_family, serveraddr->ai_socktype, 0);
    if (s < 0) {
        close(s);
        T_LOG(T_ERR, "error: socket create error");
        freeaddrinfo(localaddr);
        freeaddrinfo(serveraddr);
        return -1;
    }
    
    if (strlen(prof->local_ip) > 0) {
        if(bind(s, (struct sockaddr*)(localaddr->ai_addr), localaddr->ai_addrlen) < 0) {
            close(s);
            T_LOG(T_ERR, "error: socket bind error");
            freeaddrinfo(localaddr);
            freeaddrinfo(serveraddr);
            return -1;
        } else {
            T_LOG(T_HINT, "debug: bind local address - [%s|%d]",
                inet_ntoa(((struct sockaddr_in*)localaddr->ai_addr)->sin_addr),
                ntohs(((struct sockaddr_in*)localaddr->ai_addr)->sin_port) );
        }
    }
    
    if (connect(s, (struct sockaddr *) serveraddr->ai_addr, serveraddr->ai_addrlen) < 0) {
        close(s);
        T_LOG(T_ERR, "error: socket connect error %s", strerror(errno));
        freeaddrinfo(localaddr);
        freeaddrinfo(serveraddr);
        return -1;
    } else {
        T_LOG(T_HINT, "debug: client ctrl chan connected to - [%s|%d]",
            inet_ntoa( ((struct sockaddr_in*)serveraddr->ai_addr)->sin_addr ),
            ntohs(((struct sockaddr_in*)serveraddr->ai_addr)->sin_port) );
    }
    
    prof->ctrl_accept_sock = s;
    
    freeaddrinfo(localaddr);
    freeaddrinfo(serveraddr);
    
    return 0;
}

int ctrl_chan_accept(tpg_profile* prof, struct sockaddr_in* addr, socklen_t* len, int sec, int usec)
{
    int ret_select = -1;
    int sock_accept = -1;
    struct timeval tv;
    
    T_LOG(T_HINT, "I am accepting %d %d", sec, usec);
    
    FD_ZERO(&(prof->ctrl_read_sockset));
    FD_SET(prof->ctrl_listen_sock, &(prof->ctrl_read_sockset));
    if (prof->ctrl_listen_sock > prof->ctrl_max_sock) {
        prof->ctrl_max_sock = prof->ctrl_listen_sock;
    }
    
    if (sec < 0 || usec < 0) {
        T_LOG(T_HINT, "blocking mode select %d %d %d", sec, usec, ret_select);
        ret_select = select(prof->ctrl_max_sock + 1, &(prof->ctrl_read_sockset), NULL, NULL, NULL);
    } else {
        T_LOG(T_HINT, "non-blocking mode select %d %d %d", sec, usec, ret_select);
        tv.tv_sec = sec;
        tv.tv_usec = usec;
        ret_select = select(prof->ctrl_max_sock + 1, &(prof->ctrl_read_sockset), NULL, NULL, &tv);
    }
    
    T_LOG(T_HINT, "select() returns %d", ret_select);
    
    if (ret_select > 0) {
        if ((prof->ctrl_listen_sock > 0) && FD_ISSET(prof->ctrl_listen_sock, &(prof->ctrl_read_sockset))) {
            sock_accept = accept(prof->ctrl_listen_sock,(struct sockaddr*) addr, len);
            T_LOG(T_HINT, "accept returns %d", sock_accept);
            if (sock_accept <= 0) {
                close(sock_accept);
                T_LOG(T_ERR, "error: ctrl chan socket accept error");
                FD_CLR(prof->ctrl_listen_sock, &(prof->ctrl_read_sockset));
                return -1;
            } else {
                FD_CLR(prof->ctrl_listen_sock, &(prof->ctrl_read_sockset));
                return sock_accept;
            }
        }
    } else if (ret_select == 0) {
        FD_CLR(prof->ctrl_listen_sock, &(prof->ctrl_read_sockset));
        return ret_select;
    } else {
        FD_CLR(prof->ctrl_listen_sock, &(prof->ctrl_read_sockset));
        return ret_select;
    }
    
    FD_CLR(prof->ctrl_listen_sock, &(prof->ctrl_read_sockset));
    prof->ctrl_max_sock = -1;
    return 0;
}

int ctrl_chan_close_listen_sock(tpg_profile* prof)
{
    T_LOG(T_HINT, "Close control channel listen socket...");
    if (prof->ctrl_listen_sock > 0) {
        if (close(prof->ctrl_listen_sock) != 0) {
            T_LOG(T_ERR, "Can not close ctrl chan listen socket...");
            t_errno = T_ERR_SOCK_CLOSE_ERROR;
            return -1;
        } else {
            T_LOG(T_HINT, "socket is closed");
            prof->ctrl_listen_sock = -1;
        }
    }
    
    return 0;
}

int ctrl_chan_close_accept_sock(tpg_profile* prof)
{
    T_LOG(T_HINT, "Close control channel accept socket...");
    if (prof->ctrl_accept_sock > 0) {
        if (close(prof->ctrl_accept_sock) != 0) {
            T_LOG(T_ERR, "Can not close ctrl chan accept socket...");
            t_errno = T_ERR_SOCK_CLOSE_ERROR;
            return -1;
        } else {
            T_LOG(T_HINT, "socket is closed");
            prof->ctrl_accept_sock = -1;
        }
    }
    
    return 0;
}

int ctrl_chan_send_ctrl_packet(tpg_profile *prof, signed char sc_statevar)
{
    T_LOG(T_HINT, "Send control packet - [%s]", t_status_to_str(sc_statevar));
    
    if(ctrl_chan_send_Nbytes(prof->ctrl_accept_sock, (char*)&sc_statevar, sizeof(sc_statevar)) < 0) {
        T_LOG(T_ERR, "Can not send control packet - [%s]", t_status_to_str(sc_statevar));
        t_errno = T_ERR_SEND_MESSAGE;
        return -1;
    } else {
        T_LOG(T_HINT, "Control packet is sent - [%s]", t_status_to_str(sc_statevar));
        return 0;
    }
}

int ctrl_chan_recv_Nbytes(int fd, char *buf, int count)
{
    int r;
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

int ctrl_chan_send_Nbytes(int fd, const char *buf, int count)
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

int ctrl_chan_ctrlmsg_exchange_cli(tpg_profile *prof)
{
    int ret = -1;
    ctrl_message data;
    
    T_LOG(T_HINT, "Exchange control parameter [%d] bytes", (int)sizeof(ctrl_message));
    data.prot_id = prof->selected_protocol->protocol_id;
    data.blk_size = prof->blocksize;
    data.stream_num = prof->total_stream_num;
    data.affinity = prof->cpu_affinity_flag;
    data.udt_mss = prof->udt_mss;
    data.is_multi_nic_enabled = prof->is_multi_nic_enabled;
    // data.cli_num_nics         = prof->client_nic_num;
    data.udt_sock_snd_bufsize = prof->udt_send_bufsize;
    data.udp_sock_snd_bufsize = prof->udp_send_bufsize;
    data.udt_sock_rcv_bufsize = prof->udt_recv_bufsize;
    data.udp_sock_rcv_bufsize = prof->udp_recv_bufsize;
    data.udt_maxbw = prof->udt_maxbw;
    
    ret = ctrl_chan_send_Nbytes(prof->ctrl_accept_sock, (char*) &data, sizeof(data));
    if (ret < 0) {
        T_LOG(T_ERR, "Can not send control parameter [%d|%s]", ret, strerror(errno));
        t_errno = T_ERR_SEND_PARAMS;
        return -1;
    } else {
        T_LOG(T_HINT, "Control parameter is sent [%d] is returned", ret);
    }
    
    // remove the handling of multiple NIC-NIC test for now
    /*
    if (prof->is_multi_nic_enabled_enabled > 0) {
    	ret = ctrl_chan_recv_Nbytes(prof->ctrl_accept_sock,
                (char*)&(prof->server_nic_num), 
                sizeof(int));
    	if (ret < 0) {
            T_LOG(T_ERR, "Can not receive server NIC number - [%d|%s]",
                    ret, strerror(errno));
            t_errno = T_ERR_RECV_PARAMS;
            return -1;
    	} else {
            T_LOG(T_HINT, "Server has [%d] private NIC cards",
                    prof->server_nic_num);
    	}

    	if (prof->server_nic_num >= 1) {
            tempbuffer = new char[T_HOSTADDR_LEN * prof->server_nic_num];
            ret = ctrl_chan_recv_Nbytes(prof->ctrl_accept_sock, 
                    tempbuffer, 
                    T_HOSTADDR_LEN * prof->server_nic_num);
            if (ret < 0) {
                T_LOG(T_ERR, "Can not receive server NICs info - [%d|%s]", 
                        ret, 
                        strerror(errno));
                t_errno = T_ERR_SEND_PARAMS;
                return -1;
            }
            
            for (i = 0; i < prof->server_nic_num; ++i) {
                ip = new char[T_HOSTADDR_LEN];
                memset(ip, 0, T_HOSTADDR_LEN);
                memcpy(ip, tempbuffer + (T_HOSTADDR_LEN * i), 
                        strlen(tempbuffer + (T_HOSTADDR_LEN * i)));
                prof->server_nic_list.push_back(ip);
                T_LOG(T_HINT,
                        "Client NIC info added to recv buffer - [%d|%s|%s]",
                        i, ip,
                        tempbuffer + T_HOSTADDR_LEN * i);
            }
            
            delete [] tempbuffer;
        } else {
            T_LOG(T_ERR,
                    "Number of NIC cards at server site is %d",
                    prof->server_nic_num);
            prof->server_nic_list.clear();
            return -1;
        }
    } else {
        ;
    }
    */
    
    T_LOG(T_HINT, "Control parameter [%d] bytes are exchanged", (int)sizeof(ctrl_message));
    return 0;
}

int ctrl_chan_ctrlmsg_exchange_srv(tpg_profile *prof)
{
    int ret = -1;
    ctrl_message data;
    
    T_LOG(T_HINT, "Exchange control parameters [%d] bytes", (int)sizeof(ctrl_message));
    
    ret = ctrl_chan_recv_Nbytes(prof->ctrl_accept_sock, (char*) &data, sizeof(data));
    if (ret < 0) {
        T_LOG(T_ERR, "Can not receive control info [%d|%s]", ret, strerror(errno));
        t_errno = T_ERR_RECV_PARAMS;
        return -1;
    } else {
        set_protocol_id(prof, data.prot_id);
        T_LOG(T_HINT,"Server receive message - [%d]", ret);
        T_LOG(T_HINT,"protocol id - [%d]\n"
                "         protocol name - [%s]\n"
                "         block size - [%d]\n"
                "         stream number - [%d]\n"
                "         CPU affinity - [%d]\n"
                "         MSS - [%d]\n"
                "         IsMultiNIC - [%d]\n"
                "         UDT send buffer size - [%d]\n"
                "         UDP send buffer size - [%d]\n"
                "         UDT recv buffer size - [%d]\n"
                "         UDP recv buffer size - [%d]\n"
                "         Max BW - [%.2lf]",
                prof->selected_protocol->protocol_id,
                prof->selected_protocol->protocol_name,
                data.blk_size,
                data.stream_num,
                data.affinity,
                data.udt_mss,
                data.is_multi_nic_enabled,
                data.udt_sock_snd_bufsize,
                data.udp_sock_snd_bufsize,
                data.udt_sock_rcv_bufsize,
                data.udp_sock_rcv_bufsize,
                data.udt_maxbw);
        prof->blocksize = data.blk_size;
        prof->total_stream_num = data.stream_num;
        prof->cpu_affinity_flag  = data.affinity;
        prof->udt_mss = data.udt_mss;
        prof->is_multi_nic_enabled = data.is_multi_nic_enabled;
        prof->udt_send_bufsize = data.udt_sock_snd_bufsize;
        prof->udp_send_bufsize = data.udp_sock_snd_bufsize;
        prof->udt_recv_bufsize = data.udt_sock_rcv_bufsize;
        prof->udp_recv_bufsize = data.udp_sock_rcv_bufsize;
        prof->udt_maxbw = data.udt_maxbw;
    }
    
    // check to see if we need to do the host profiling
    /* if (prof->is_multi_nic_enabled_enabled > 0) {
        prof->server_nic_num = get_host_nic_ip(prof);
        
        ret = ctrl_chan_send_Nbytes(prof->ctrl_accept_sock,
                (char *)&(prof->server_nic_num),
                sizeof(int));
        if (ret < 0) {
            T_LOG(T_ERR, "Can not send number of NIC cards [%d|%s]",
                    ret,
                    strerror(errno));
            t_errno = T_ERR_SEND_PARAMS;
            return -1;
        }

        if (prof->server_nic_num >= 1) {
            tempbuffer = new char[T_HOSTADDR_LEN * prof->server_nic_num];
            memset(tempbuffer, 0, T_HOSTADDR_LEN * prof->server_nic_num);
            for (i = 0; i < prof->server_nic_num; ++i) {
                memcpy(tempbuffer + (T_HOSTADDR_LEN * i),
                        prof->server_nic_list[i],
                        strlen(prof->server_nic_list[i]));
                T_LOG(T_HINT, "Server NIC added to buffer - [%d|%s]",
                        i, tempbuffer + T_HOSTADDR_LEN * i);
            }
            ret = ctrl_chan_send_Nbytes(prof->ctrl_accept_sock,
                    tempbuffer,
                    T_HOSTADDR_LEN * prof->server_nic_num);
            if (ret < 0) {
                T_LOG(T_ERR, "Can not send NIC IPs error [%d|%s]", ret,
                        strerror(errno));
                t_errno = T_ERR_SEND_PARAMS;
                return -1;
            }
            delete [] tempbuffer;
        } else {
            T_LOG(T_ERR,
                "Number of NIC cards at server is %d", prof->server_nic_num);
        }
    } else {
        T_LOG(T_HINT, "Multi-NIC is not enabled");
    }*/
    return 0;
}

int ctrl_chan_ctrlmsg_exchange(tpg_profile *prof)
{
    if (prof->role == T_CLIENT) {
        if (ctrl_chan_ctrlmsg_exchange_cli(prof) < 0) {
            return -1;
        }
    }
    if (prof->role == T_SERVER) {
        if (ctrl_chan_ctrlmsg_exchange_srv(prof) < 0) {
            return -1;
        }
    }
    return 0;
}

int ctrl_chan_perf_exchange_send(tpg_profile *prof)
{
    if (prof->role == T_CLIENT) {
        if (ctrl_chan_send_Nbytes(prof->ctrl_accept_sock, (char*)&(prof->client_perf_mbps), sizeof(double)) < 0) {
            t_errno = T_ERR_SEND_MESSAGE;
            return -1;
        }
    } else {
        if (ctrl_chan_send_Nbytes(prof->ctrl_accept_sock, (char*)&(prof->server_perf_mbps), sizeof(double)) < 0) {
            t_errno = T_ERR_SEND_MESSAGE;
            return -1;
        }
    }
    return 0;
}

int ctrl_chan_perf_exchange_recv(tpg_profile *prof)
{
    if (prof->role == T_CLIENT) {
        if (ctrl_chan_recv_Nbytes(prof->ctrl_accept_sock, (char*)&(prof->server_perf_mbps), sizeof(double)) < 0) {
            t_errno = T_ERR_SEND_MESSAGE;
            return -1;
        }
    } else {
        if (ctrl_chan_recv_Nbytes(prof->ctrl_accept_sock, (char*)&(prof->client_perf_mbps), sizeof(double)) < 0) {
            t_errno = T_ERR_SEND_MESSAGE;
            return -1;
        }
    }
    return 0;
}

int ctrl_chan_perf_exchange(tpg_profile *prof)
{
    T_LOG(T_HINT, "Exchange performance measurements");
    
    if (prof->role == T_CLIENT) {
        if (ctrl_chan_perf_exchange_send(prof) < 0) {
            return -1;
        }
        if (ctrl_chan_perf_exchange_recv(prof) < 0) {
            return -1;
        }
    } else if(prof->role == T_SERVER) {
        if (ctrl_chan_perf_exchange_recv(prof) < 0) {
            return -1;
        }
        if (ctrl_chan_perf_exchange_send(prof) < 0) {
            return -1;
        }
    } else {
        T_LOG(T_ERR, "Program role error");
        return -1;
    }
    return 0;
}