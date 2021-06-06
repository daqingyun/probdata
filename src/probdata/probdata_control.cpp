/***************************************************************************
 * ProbData - PRofiling Optimization Based DAta Transfer Advisor
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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include "probdata_control.h"


int probdata_ctrl_chan_listen(struct probdata* prob)
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
    sprintf(portstr, "%d", prob->ctrl_port);
    if(strlen(prob->local_ip) > 0) {
        if (getaddrinfo(prob->local_ip, portstr, &hints, &localaddr) != 0) {
            P_LOG(P_ERR, "getaddrinfo %s", strerror(errno));
            freeaddrinfo(localaddr);
            return -1;
        }
    } else {
        if (getaddrinfo(NULL, portstr, &hints, &localaddr) != 0) {
            P_LOG(P_ERR, "getaddrinfo %s", strerror(errno));
            freeaddrinfo(localaddr);
            return -1;
        }
    }
    
    if ((s = socket(localaddr->ai_family, localaddr->ai_socktype, 0)) < 0) {
        P_LOG(P_ERR, "socket create failure %s", strerror(errno));
        freeaddrinfo(localaddr);
        return -1;
    }
    
    P_LOG(P_HINT, "local address of control channel [%s|%d]",
            inet_ntoa( ((struct sockaddr_in*)localaddr->ai_addr)->sin_addr ),
            ntohs( ((struct sockaddr_in*)localaddr->ai_addr)->sin_port) );
    
    opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        close(s);
        freeaddrinfo(localaddr);
        P_LOG(P_ERR, "ctrl chan set option SO_REUSEADDR error");
        return -1;
    } 
    
    if(bind(s, (struct sockaddr*)(localaddr->ai_addr), localaddr->ai_addrlen) < 0) {
        close(s);
        freeaddrinfo(localaddr);
        P_LOG(P_ERR, "ctrl channel socket bind error");
        return -1;
    } else {
        P_LOG(P_HINT, "debug: bind local address - [%s|%d]",
            inet_ntoa( ((struct sockaddr_in*)localaddr->ai_addr)->sin_addr ),
            ntohs(((struct sockaddr_in*)localaddr->ai_addr)->sin_port) );
    }
    
    if (listen(s, P_MAX_BACKLOG) < 0) {
        close(s);
        freeaddrinfo(localaddr);
        P_LOG(P_ERR, "control channel socket listen error");
        return -1;
    } else {
        P_LOG(P_HINT, "control channel listen on - [%s|%d]",
            inet_ntoa( ((struct sockaddr_in*)localaddr->ai_addr)->sin_addr ),
            ntohs(((struct sockaddr_in*)localaddr->ai_addr)->sin_port) );
    }
    
    prob->listen_sock = s;
    freeaddrinfo(localaddr);
    
    return 0;
}

int probdata_ctrl_chan_connect(struct probdata* prob)
{
    int s = -1;
    char portstr[6];
    struct addrinfo hints;
    struct addrinfo *localaddr = NULL;
    struct addrinfo *serveraddr = NULL;
    
    P_LOG(P_HINT, "sending connection request - [%s|%s|%d]",
            prob->local_ip, prob->remote_ip, prob->ctrl_port);
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if(strlen(prob->local_ip) > 0) {
        if (getaddrinfo(prob->local_ip, "0", &hints, &localaddr) != 0) {
            P_LOG(P_ERR, "getaddrinfo %s", strerror(errno));
            freeaddrinfo(localaddr);
            return -1;
        }
    } else {
        if (getaddrinfo(NULL, "0", &hints, &localaddr) != 0) {
            P_LOG(P_ERR, "getaddrinfo %s", strerror(errno));
            freeaddrinfo(localaddr);
            return -1;
        }
    }
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    sprintf(portstr, "%d", prob->ctrl_port);
    if(strlen(prob->remote_ip) > 0) {
        if (getaddrinfo(prob->remote_ip, portstr, &hints, &serveraddr) != 0) {
            P_LOG(P_ERR, "getaddrinfo %s", strerror(errno));
            freeaddrinfo(localaddr);
            freeaddrinfo(serveraddr);
            return -1;
        }
    } else {
        P_LOG(P_PERF, "invalid server address");
        freeaddrinfo(localaddr);
        freeaddrinfo(serveraddr);
        return -1;
    }
    
    s = socket(serveraddr->ai_family, serveraddr->ai_socktype, 0);
    if (s < 0) {
        close(s);
        P_LOG(P_ERR, "socket create error");
        freeaddrinfo(localaddr);
        freeaddrinfo(serveraddr);
        return -1;
    }
    
    if (strlen(prob->local_ip) > 0) {
        if(bind(s, (struct sockaddr*)(localaddr->ai_addr), localaddr->ai_addrlen) < 0) {
            close(s);
            P_LOG(P_ERR, "socket bind error");
            freeaddrinfo(localaddr);
            freeaddrinfo(serveraddr);
            return -1;
        } else {
            P_LOG(P_HINT, "bind local address - [%s|%d]",
                inet_ntoa(((struct sockaddr_in*)localaddr->ai_addr)->sin_addr),
                ntohs(((struct sockaddr_in*)localaddr->ai_addr)->sin_port) );
        }
    }
    
    if (connect(s, (struct sockaddr *) serveraddr->ai_addr, serveraddr->ai_addrlen) < 0) {
        close(s);
        P_LOG(P_ERR, "socket connect error %s", strerror(errno));
        freeaddrinfo(localaddr);
        freeaddrinfo(serveraddr);
        return -1;
    } else {
        P_LOG(P_HINT, "client ctrl chan connected to - [%s|%d]",
                inet_ntoa(((struct sockaddr_in*)serveraddr->ai_addr)->sin_addr),
                ntohs(((struct sockaddr_in*)serveraddr->ai_addr)->sin_port));
    }
    
    prob->accept_sock = s;
    
    freeaddrinfo(localaddr);
    freeaddrinfo(serveraddr);
    
    return 0;
}

int probdata_ctrl_chan_accept(struct probdata* prob, struct sockaddr_in* addr, socklen_t* len)
{
    int s = -1;
    
    s = accept(prob->listen_sock, (struct sockaddr*) addr, len);
    
    if (s < 0) {
        close(s);
        P_LOG(P_ERR, "ctrl chan socket accept error");
        return -1;
    }
    
    return s;
}

int probdata_ctrl_chan_close_listen_sock(struct probdata *prob)
{
    if (prob->listen_sock > 0) {
        if (close(prob->listen_sock) != 0) {
            p_errno = P_ERR_UNKNOWN;
            return -1;
        } else {
            prob->listen_sock = -1;
        }
    }
    
    return 0;
}

int probdata_ctrl_chan_close_accept_sock(struct probdata *prob)
{
    P_LOG(P_HINT, "close control channel socket");
    if (prob->accept_sock > 0) {
        if (close(prob->accept_sock) != 0) {
            P_LOG(P_ERR, "cannot close control channel socket");
            f_errno = P_ERR_UNKNOWN;
            return -1;
        } else {
            P_LOG(P_HINT, "control channel socket is closed");
            prob->accept_sock = -1;
        }
    }
    return 0;
}

int probdata_ctrl_chan_send_ctrl_packet(struct probdata *prob, signed char sc_statevar)
{
    P_LOG(P_HINT, "send control packet - [%s]", probdata_status_to_str(sc_statevar));
    
    if(probdata_ctrl_chan_send_Nbytes(prob->accept_sock, (char*)&sc_statevar, sizeof(sc_statevar)) < 0) {
        P_LOG(P_ERR, "cannot send control packet - [%s]", probdata_status_to_str(sc_statevar));
        p_errno = P_ERR_UNKNOWN;
        return -1;
    } else {
        P_LOG(P_HINT, "control packet is sent - [%s]", probdata_status_to_str(sc_statevar));
        return 0;
    }
}

int probdata_ctrl_chan_recv_Nbytes(int fd, char *buf, int count)
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

int probdata_ctrl_chan_send_Nbytes(int fd, const char *buf, int count)
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

int probdata_ctrl_chan_ctrlmsg_exchange_cli(probdata *prob)
{
    int ret = -1;
    struct probdata_ctrlmsg data;
    
    P_LOG(P_HINT, "exchange ctrl parameter [%d] bytes", (int)sizeof(probdata_ctrlmsg));
    
    data.do_udt_only = prob->do_udt_only;
    data.host_max_buf = prob->prob_control->host_max_buf;
    
    ret = probdata_ctrl_chan_send_Nbytes(prob->accept_sock, (char*) &data, sizeof(data));
    if (ret < 0) {
        P_LOG(P_ERR, "cannot send ctrl parameter [%d|%s]", ret, strerror(errno));
        p_errno = P_ERR_UNKNOWN;
        return -1;
    } else {
        P_LOG(P_HINT, "ctrl parameter is sent [%d] is returned", ret);
    }
    
    ret = probdata_ctrl_chan_recv_Nbytes(prob->accept_sock, (char*) &data, sizeof(data));
    if (ret < 0) {
        P_LOG(P_ERR, "cannot receive ctrl info [%d|%s]", ret, strerror(errno));
        p_errno = P_ERR_UNKNOWN;
        return -1;
    } else {
        P_LOG(P_HINT, "client receive message: %d", ret);
        P_LOG(P_HINT, "host max buf: %d", data.host_max_buf);
        prob->host_max_buf = (data.host_max_buf > prob->host_max_buf) ? prob->host_max_buf: data.host_max_buf;
        prob->prob_control->host_max_buf = prob->host_max_buf;
    }
    
    return 0;
}

int probdata_ctrl_chan_ctrlmsg_exchange_srv(probdata *prob)
{
    int ret = -1;
    struct probdata_ctrlmsg data;
    
    P_LOG(P_HINT, "exchange ctrl parameters [%d] bytes", (int)sizeof(probdata_ctrlmsg));
    
    ret = probdata_ctrl_chan_recv_Nbytes(prob->accept_sock, (char*) &data, sizeof(data));
    if (ret < 0) {
        P_LOG(P_ERR, "cannot receive ctrl info [%d|%s]", ret, strerror(errno));
        p_errno = P_ERR_UNKNOWN;
        return -1;
    } else {
        P_LOG(P_HINT, "server receive message: %d", ret);
        P_LOG(P_HINT, "do udt only: %s", (data.do_udt_only > 0) ? "YES" : "NO");
        P_LOG(P_HINT, "host max buf: %d", data.host_max_buf);
        prob->do_udt_only = data.do_udt_only;
        prob->host_max_buf = (data.host_max_buf > prob->host_max_buf) ? prob->host_max_buf: data.host_max_buf;
        prob->prob_control->host_max_buf = prob->host_max_buf;
    }

    data.host_max_buf = prob->host_max_buf;

    ret = probdata_ctrl_chan_send_Nbytes(prob->accept_sock, (char*) &data, sizeof(data));
    if (ret < 0) {
        P_LOG(P_ERR, "cannot send ctrl info [%d|%s]", ret, strerror(errno));
        p_errno = P_ERR_UNKNOWN;
        return -1;
    } else {
        P_LOG(P_HINT, "ctrl parameter is sent [%d] is returned", ret);
    }
    
    return 0;
}

int probdata_ctrl_chan_ctrlmsg_exchange(probdata *prob)
{
    if (prob->role == P_CLIENT) {
        if (probdata_ctrl_chan_ctrlmsg_exchange_cli(prob) < 0) {
            return -1;
        }
    }
    
    if (prob->role == P_SERVER) {
        if (probdata_ctrl_chan_ctrlmsg_exchange_srv(prob) < 0) {
            return -1;
        }
    }
    
    return 0;
}

int probdata_ctrl_chan_perf_exchange_send(probdata *prob)
{
    if (prob->role == P_CLIENT) {
        if (probdata_ctrl_chan_send_Nbytes(prob->accept_sock, (char*)&(prob->max_perf_mbps), sizeof(double)) < 0) {
            p_errno = P_ERR_UNKNOWN;
            return -1;
        }
    } else {
        if (probdata_ctrl_chan_send_Nbytes(prob->accept_sock, (char*)&(prob->max_perf_mbps), sizeof(double)) < 0) {
            p_errno = P_ERR_UNKNOWN;
            return -1;
        }
    }
    
    return 0;
}

int probdata_ctrl_chan_perf_exchange_recv(probdata *prob)
{
    if (prob->role == P_CLIENT) {
        if (probdata_ctrl_chan_recv_Nbytes(prob->accept_sock, (char*)&(prob->max_perf_mbps), sizeof(double)) < 0) {
            p_errno = P_ERR_UNKNOWN;
            return -1;
        }
    } else {
        if (probdata_ctrl_chan_recv_Nbytes(prob->accept_sock, (char*)&(prob->max_perf_mbps), sizeof(double)) < 0) {
            p_errno = P_ERR_UNKNOWN;
            return -1;
        }
    }
    
    return 0;
}

int probdata_ctrl_chan_perf_exchange(probdata *prob)
{
    P_LOG(P_HINT, "exchange performance measurements");
    
    if (prob->role == P_CLIENT) {
        if (probdata_ctrl_chan_perf_exchange_send(prob) < 0) {
            return -1;
        }
        if (probdata_ctrl_chan_perf_exchange_recv(prob) < 0) {
            return -1;
        }
    } else if(prob->role == P_SERVER) {
        if (probdata_ctrl_chan_perf_exchange_recv(prob) < 0) {
            return -1;
        }
        if (probdata_ctrl_chan_perf_exchange_send(prob) < 0) {
            return -1;
        }
    } else {
        P_LOG(P_ERR, "program role error");
        return -1;
    }
    
    return 0;
}

const char* probdata_status_to_str(signed char prob_state)
{
    switch (prob_state)
    {
	case P_CTRL_CHAN_EST:
        {
            return "P_CTRL_CHAN_EST";
            break;
        }
	case P_PROGRAM_START:
        {
            return "P_PROGRAM_START";
            break;
        }
	case P_TWOWAY_HANDSHAKE:
        {
            return "P_TWOWAY_HANDSHAKE";
            break;
        }
	case P_RUN_M2M_DB:
        {
            return "P_RUN_M2M_DB";
            break;
        }
	case P_RUN_M2M_TCP:
        {
            return "P_RUN_M2M_TCP";
            break;
        }
	case P_RUN_M2M_UDT:
        {
            return "P_RUN_M2M_UDT";
            break;
        }
	case P_RUN_D2D_DB:
        {
            return "P_RUN_D2D_DB";
            break;
        }
	case P_RUN_D2D_TCP:
        {
            return "P_RUN_D2D_TCP";
            break;
        }
	case P_RUN_D2D_UDT:
        {
            return "P_RUN_D2D_UDT";
            break;
        }
	case P_PROGRAM_DONE:
        {
            return "P_PROGRAM_DONE";
            break;
        }
	case P_SERVER_ERROR:
        {
            return "P_SERVER_ERROR";
            break;
        }
	case P_CLIENT_TERMINATE:
        {
            return "P_CLIENT_TERMINATE";
            break;
        }
	case P_ACCESS_DENIED:
        {
            return "P_ACCESS_DENIED";
            break;
        }
	default:
        {
            return "BAD_STATE";
            break;
        }
    }
}

double probdata_unit_atof(const char *str)
{
    double var;
    char unit = '\0';

    assert(str != NULL);
    sscanf(str, "%lf%c", &var, &unit);
    switch (unit)
    {
    case 'g':
    case 'G':
        {
            var *= P_GB;
            break;
        }
    case 'm':
    case 'M':
        {
            var *= P_MB;
            break;
        }
    case 'k':
    case 'K':
        {
            var *= P_KB;
            break;
        }
    default:
        break;
    }

    return var;
}

int probdata_unit_atoi(const char *str)
{
    double var;
    char unit = '\0';

    assert(str != NULL);

    sscanf(str, "%lf%c", &var, &unit);
    
    switch (unit)
    {
    case 'g':
    case 'G':
        {
            var *= P_GB;
            break;
        }
    case 'm':
    case 'M':
        {
            var *= P_MB;
            break;
        }
    case 'k':
    case 'K':
        {
            var *= P_KB;
            break;
        }
    default:
        break;
    }
    return (int) var;
}