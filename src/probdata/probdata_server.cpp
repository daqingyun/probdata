/***************************************************************************
 * ProbData - PRofiling Optimization Based DAta Transfer Advisor
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
// #include "../tpg/tpg_profile.h"
// #include "../tpg/tpg_server.h"
#include <tpg_profile.h>
#include <tpg_server.h>
#include "../fastprof/fastprof_tcp.h"
#include "../fastprof/fastprof_udt.h"
#include "probdata.h"
#include "probdata_control.h"
#include "probdata_error.h"
#include "probdata_log.h"
#include "probdata_server.h"

int probdata_server(struct probdata* prob)
{
    int ret = 0;

    system("date");
    P_LOG(P_PERF, "*** PROBDATA SERVER IS STARTED ***");
    
    prob->status = P_PROGRAM_START;
    
    /* set jump signal - to be added */
    ;
    
    if (probdata_server_listen(prob) < 0) {
        return -1;
    }
    
    if (probdata_server_accept(prob) < 0) {
        return -1;
    }
    
    prob->status = P_CTRL_CHAN_EST;
    P_LOG(P_HINT, "state -> CTRLCHAN_ESTABLISHED");
    if (probdata_ctrl_chan_send_ctrl_packet(prob, P_CTRL_CHAN_EST) < 0) {
        p_errno = P_ERR_SEND_MESSAGE;
        return -1;
    }
    
    while (prob->status != P_PROGRAM_DONE && prob->status != P_SERVER_ERROR && prob->status != P_CLIENT_TERMINATE) {
        ret = probdata_server_poll_ctrl_channel(prob);
        if (ret < 0) {
            p_errno = P_ERR_UNKNOWN;
            return -1;
        } else if (ret == 0) {
            continue;
        } else {
            if (probdata_server_proc_ctrl_packet(prob) < 0) {
                p_errno = P_ERR_UNKNOWN;
                return -1;
            }
        }
    }
    
    P_LOG(P_HINT, "********** print something below 'cause it is done");
    
    if (probdata_server_clean_up(prob) < 0) {
        return -1;
    }

    system("date");
    
    return 0;
}

int probdata_server_listen(struct probdata* prob)
{
    if (probdata_ctrl_chan_listen(prob) < 0) {
        p_errno = P_ERR_UNKNOWN;
        return -1;
    }
    
    P_LOG(P_HINT, "probdata server is listening on port [%d]", prob->ctrl_port);
    
    return 0;
}

int probdata_server_accept(struct probdata* prob)
{
    int s = -1;
	int ip_len = 0;
    socklen_t len = 0;
    char cookie[P_IDPKT_SIZE];
    struct sockaddr_in client_addr;
    
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    len = sizeof(client_addr);
    
    if ( (s = probdata_ctrl_chan_accept(prob, &client_addr, &len)) < 0) {
        p_errno = P_ERR_UNKNOWN;
        return -1;
    }
    
    if (prob->accept_sock < 0) {
        prob->accept_sock = s;
        if (probdata_ctrl_chan_recv_Nbytes(prob->accept_sock, prob->id, P_IDPKT_SIZE) < 0){
            p_errno = P_ERR_UNKNOWN;
            return -1;
        }
        P_LOG(P_HINT, "client [%s] is accepted on port [%d]",
            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        ip_len = (int)strlen(inet_ntoa(client_addr.sin_addr));
        if (probdata_ctrl_chan_send_Nbytes(prob->accept_sock, (char*)&ip_len, sizeof(int)) < 0){
            p_errno = P_ERR_UNKNOWN;
            return -1;
		}
		if (probdata_ctrl_chan_send_Nbytes(prob->accept_sock, inet_ntoa(client_addr.sin_addr), ip_len) < 0){
            p_errno = P_ERR_UNKNOWN;
            return -1;
		}
    } else {
        P_LOG(P_HINT, "server is busy and request [%s|%d] is rejected",
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        if (probdata_ctrl_chan_recv_Nbytes(s, cookie, P_IDPKT_SIZE) < 0) {
            p_errno = P_ERR_UNKNOWN;
            return -1;
        }
        if (probdata_ctrl_chan_send_ctrl_packet(prob, P_ACCESS_DENIED) > 0) {
            p_errno = P_ERR_UNKNOWN;
            return -1;
        }
        
        close(s);
    }
    
    return 0;
}

int probdata_server_poll_ctrl_channel(struct probdata* prob)
{
    int ret_select = -1;
    int ret_size = -1;
    struct timeval tv;
    
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
    FD_ZERO(&(prob->read_sock_set));
    FD_SET(prob->accept_sock, &(prob->read_sock_set));
    if (prob->accept_sock > prob->max_sock) {
        prob->max_sock = prob->accept_sock;
    }
    
    ret_select = select(prob->max_sock +1, &(prob->read_sock_set), NULL, NULL, &tv);
    
    if (ret_select < 0) {
        p_errno = P_ERR_UNKNOWN;
        return -1;
    } else if(ret_select == 0) {
        return 0;
    } else {
        if ((prob->accept_sock > 0) && FD_ISSET(prob->accept_sock, &(prob->read_sock_set)) ) {
            ret_size = probdata_ctrl_chan_recv_Nbytes(prob->accept_sock, (char*)&(prob->status), sizeof(signed char));
            if (ret_size <= 0) {
                if (ret_size == 0) {
                    P_LOG(P_HINT, "an unexpected error happens");
                    p_errno = P_ERR_UNKNOWN;
                } else {
                    P_LOG(P_HINT, "an unexpected error happens");
                    p_errno = P_ERR_UNKNOWN;
                }
                return -1;
            } else {
                ;
            }
        } else {
            P_LOG(P_HINT, "something is wrong with control socket");
            p_errno = P_ERR_UNKNOWN;
            return -1;
        }
    }
    
    FD_CLR(prob->accept_sock, &(prob->read_sock_set));
    prob->max_sock = -1;
    
    return ret_select;
}

int probdata_server_proc_ctrl_packet(struct probdata* prob)
{
    P_LOG(P_HINT, "server process a control packet of type %s",
            probdata_status_to_str(prob->status));

    switch (prob->status) {
        case P_TWOWAY_HANDSHAKE:
        {
            prob->status = P_TWOWAY_HANDSHAKE;
            P_LOG(P_HINT, "probdata state -> P_TWOWAY_HANDSHAKE");
            if (probdata_ctrl_chan_send_ctrl_packet(prob, P_TWOWAY_HANDSHAKE) < 0) {
                p_errno = P_ERR_UNKNOWN;
                return -1;
            } else {
                P_LOG(P_HINT, "P_TWOWAY_HANDSHAKE is sent");
            }
            
            if (probdata_ctrl_chan_ctrlmsg_exchange(prob) < 0) {
                return -1;
            } else {
                P_LOG(P_HINT, "control info exchanged");
            }
            break;
        }
	case P_RUN_D2D_DB:
        {
            prob->status = P_RUN_D2D_DB;
            P_LOG(P_HINT, "probdata state -> P_RUN_D2D_DB");
            
            if (probdata_ctrl_chan_send_ctrl_packet(prob, P_RUN_D2D_DB) < 0) {
                p_errno = P_ERR_UNKNOWN;
                return -1;
            } else {
                ;
            }
            break;
        }
	case P_RUN_D2D_TCP:
        {
            prob->status = P_RUN_D2D_TCP;
            P_LOG(P_HINT, "probdata state -> P_RUN_D2D_TCP");
            
            /* run dw xdd tcp-based server */
            P_LOG(P_PERF, "probdata is starting xdd tcp-based server");
            if (probdata_server_dw_xdd_tcp(prob) < 0) {
                p_errno = P_ERR_UNKNOWN;
                return -1;
            }
            
            if (probdata_ctrl_chan_send_ctrl_packet(prob, P_RUN_D2D_TCP) < 0) {
                p_errno = P_ERR_UNKNOWN;
                return -1;
            } else {
                ;
            }
            break;
        }
	case P_RUN_D2D_UDT:
        {
            prob->status = P_RUN_D2D_UDT;
            P_LOG(P_HINT, "probdata state -> P_RUN_D2D_UDT");
            
            /* run dw xdd udt-based server */
            P_LOG(P_PERF, "probdata state starting xdd udt-based server");
            if (probdata_server_dw_xdd_udt(prob) < 0) {
                p_errno = P_ERR_UNKNOWN;
                return -1;
            }
            
            if (probdata_ctrl_chan_send_ctrl_packet(prob, P_RUN_D2D_UDT) < 0) {
                p_errno = P_ERR_UNKNOWN;
                return -1;
            } else {
                ;
            }
            break;
        }
	case P_RUN_M2M_DB:
        {
            prob->status = P_RUN_M2M_DB;
            P_LOG(P_HINT, "probdata state -> P_RUN_M2M_DB");
            
            if (probdata_ctrl_chan_send_ctrl_packet(prob, P_RUN_M2M_DB) < 0) {
                p_errno = P_ERR_UNKNOWN;
                return -1;
            } else {
                ;
            }
            break;
        }
	case P_RUN_M2M_TCP:
        {
            prob->status = P_RUN_M2M_TCP;
            P_LOG(P_HINT, "probdata state -> P_RUN_M2M_TCP");
            
            P_LOG(P_HINT, "probdata is starting fastprof tcp-based server");
            if (probdata_server_tcp_fastprof_run(prob) < 0) {
                p_errno = P_ERR_UNKNOWN;
                return -1;
            }
            
            if (probdata_ctrl_chan_send_ctrl_packet(prob, P_RUN_M2M_TCP) < 0) {
                p_errno = P_ERR_UNKNOWN;
                return -1;
            } else {
                ;
            }
            break;
        }
	case P_RUN_M2M_UDT:
        {
            prob->status = P_RUN_M2M_UDT;
            P_LOG(P_HINT, "probdata state -> P_RUN_M2M_UDT");

            if (prob->do_udt_only > 0) {
                // since we didn't do TCP
                ;
            } else {
                if (pthread_cancel(*(prob->prob_tcp_worker)) != 0) {
                    P_LOG(P_HINT, "cannot kill iperf server of fastprof %s", strerror(errno));
                } else {
                    P_LOG(P_HINT, "iperf server of fastprof is killed");
                }
                if(pthread_join(*(prob->prob_tcp_worker), NULL) != 0) {
                    P_LOG(P_HINT, "pthread_join() fails %s", strerror(errno));
                } else {
                    P_LOG(P_HINT, "probdata fastprof tcp working thread is stopped");
                }
                P_LOG(P_PERF, "\n***FASTPROF TCP-BASED SERVER IS DONE***\n");
            }
            
            if (probdata_ctrl_chan_send_ctrl_packet(prob, P_RUN_M2M_UDT) < 0) {
                p_errno = P_ERR_UNKNOWN;
                return -1;
            } else {
                ;
            }
            
            usleep(500);
            
            if (probdata_server_udt_fastprof_run(prob) < 0) {
                p_errno = P_ERR_UNKNOWN;
                return -1;
            }
            break;
        }
	case P_CLIENT_TERMINATE:
        {
            prob->status = P_CLIENT_TERMINATE;
            P_LOG(P_HINT, "probdata state -> P_CLIENT_TERMINATE");
            
            /* necessarily do something */
            ;
            break;
        }
	case P_PROGRAM_DONE:
        {
            prob->status = P_PROGRAM_DONE;
            P_LOG(P_HINT, "probdata state -> P_PROGRAM_DONE");
            
            // do some wrap up work and then exit
            P_LOG(P_PERF, "***FASTPROF UDT-BASED SERVER IS DONE***\n");
            P_LOG(P_HINT, "probdata server is waiting for threads to finish");
            if (prob->prob_udt_worker != NULL) {
                if(pthread_join(*(prob->prob_udt_worker), NULL) != 0) {
                    P_LOG(P_HINT, "pthread_join() fails %s", strerror(errno));
                } else {
                    P_LOG(P_HINT, "probdata fastprof udt working thread is exitted");
                }
            }
            if (prob->prob_tcp_worker != NULL) {
                if(pthread_join(*(prob->prob_tcp_worker), NULL) != 0) {
                    P_LOG(P_HINT, "pthread_join() fails %s", strerror(errno));
                } else {
                    P_LOG(P_HINT, "probdata fastprof udt working thread is exitted");
                }
            }
            
            if (probdata_ctrl_chan_send_ctrl_packet(prob, P_PROGRAM_DONE) < 0) {
                p_errno = P_ERR_UNKNOWN;
                return -1;
            } else {
                ;
            }
            break;
        }
        default :
        {
            break;
        }
    }
    
    return 0;
}

int probdata_server_clean_up(struct probdata* prob)
{
    P_LOG(P_PERF, "***SERVER IS CLEANING UP*** (NOT YET TOTALLY IMPLEMENTED)");
    
    /* clean up for tpg
    if (prob->prob_udt_prof != NULL) {
        // the tpg prof is cleaned up in its own client/server
        
        if (tpg_server_clean_up(prob->prob_udt_prof->udt_prof) < 0) {
            P_LOG(P_HINT, "tpg clean up error");
            return -1;
        }
        delete prob->prob_udt_prof->udt_prof;
        prob->prob_udt_prof->udt_prof = NULL;
    }*/
    
    /* clean up for fastprof */
    ;
    
    /* clean up for probdata */
    delete (prob->prob_udt_worker);
    prob->prob_udt_worker = NULL;
    delete (prob->prob_tcp_worker);
    prob->prob_tcp_worker = NULL;
    prob->instance = NULL;
    FD_CLR(prob->accept_sock, &(prob->read_sock_set));
    FD_CLR(prob->listen_sock, &(prob->read_sock_set));
    close(prob->listen_sock);
    close(prob->accept_sock);
    memset(prob->local_ip, '0', P_HOSTADDR_LEN);
    memset(prob->remote_ip, '0', P_HOSTADDR_LEN);
    memset(prob->src_file, '0', P_FILENAME_LEN);
    memset(prob->dst_file, '0', P_FILENAME_LEN);
    memset(prob->id, '0', P_IDPKT_SIZE);
    
    return 0;
}

int probdata_server_m2m_record_recv(probdata* prob, struct mem_record* rec)
{
    if (prob->role == P_SERVER) {
        if (probdata_ctrl_chan_recv_Nbytes(prob->accept_sock,
            (char*)rec, sizeof(struct mem_record)) < 0) {
                p_errno = P_ERR_UNKNOWN;
                return -1;
        }
    } else {
        p_errno = P_ERR_SERV_CLIENT;
        return -1;
    }

    return 0;
}

int probdata_server_tcp_prof_new(probdata* prob)
{
    prob->prob_tcp_prof = f_tcp_prof_new();
    if (prob->prob_tcp_prof == NULL) {
        p_errno = P_ERR_FASTPROF;
        return -1;
    } else {
        return 0;
    }
}

int probdata_server_tcp_prof_default(probdata* prob)
{
    return f_tcp_prof_default(prob->prob_tcp_prof);
}

int probdata_server_tcp_prof_init(probdata* prob)
{
    return f_tcp_prof_init(prob->prob_tcp_prof, prob->prob_control);
}

int probdata_server_tcp_prof_delete(struct probdata* prob)
{
    return f_tcp_prof_delete(prob->prob_tcp_prof);
}

int probdata_server_tcp_fastprof_run(probdata* prob)
{
    int ret = -1;
    
    P_LOG(P_PERF, "\n***FASTPROF TCP-BASED SERVER IS STARTED***\n");
    
    if(probdata_server_tcp_prof_new(prob) < 0) {
        return -1;
    }
    
    if(probdata_server_tcp_prof_default(prob) < 0) {
        return -1;
    }
    
    if(probdata_server_tcp_prof_init(prob) < 0) {
        return -1;
    }
    
    prob->prob_tcp_worker = new pthread_t;
    ret = pthread_create(prob->prob_tcp_worker, NULL, probdata_server_tcp_fastprof_worker, (void*)prob);
    if (ret != 0) {
        P_LOG(P_HINT, "cannot create threads");
        p_errno = P_ERR_THREAD;
        return -1;
    } else {
        return 0;
    }
}

int probdata_server_tcp_prof_reset(probdata *prob)
{
    return f_tcp_prof_reset(prob->prob_tcp_prof, prob->prob_control);
}

void* probdata_server_tcp_fastprof_worker(void *prob)
{
    int err_cnt = 0;
    
    struct probdata* prob_local = (struct probdata*) prob;
    
    P_LOG(P_HINT, "FASTPROF TCP-BASED SERVER WORKER IS STARTED");
    
    pthread_cleanup_push(probdata_server_tcp_fastprof_cleanup_handler, prob_local);
    
    for (;;) {
        pthread_testcancel();  // the cancellation point
        if (iperf_run_server(prob_local->prob_tcp_prof->tcp_prof) < 0) {
            fprintf(stderr, "iperf3 error %s\n\n", iperf_strerror(i_errno));
            err_cnt = err_cnt + 1;
            if (err_cnt >= P_MAX_ERROR_NUM) {
                fprintf(stderr, "%d: too many errors, exiting\n", err_cnt);
                break;
            }
        } else {
            err_cnt = 0;
        }
        if (probdata_server_tcp_prof_reset(prob_local) < 0) {
            break;
        }
    }
    
    probdata_server_tcp_prof_delete(prob_local);
    
    pthread_cleanup_pop(1); // always call cleanup handler
    
    return NULL;
}

void probdata_server_tcp_fastprof_cleanup_handler(void *prob)
{
    struct probdata* prob_local = (struct probdata*) prob;
    
    // cleanup iperf3 test, these are executed somewhere else actually
    // but...
    f_tcp_prof_reset(prob_local->prob_tcp_prof, prob_local->prob_control);
    f_tcp_prof_delete(prob_local->prob_tcp_prof);
    
    // iperf_reset_test(pdt_local->fp_iperf_test->test);
    // iperf_free_test(pdt_local->fp_iperf_test->test);
    
    return ;
}

int probdata_server_udt_prof_new(struct probdata* prob)
{
    prob->prob_udt_prof = f_udt_prof_new();
    if (prob->prob_udt_prof == NULL) {
        p_errno = P_ERR_FASTPROF;
        return -1;
    } else {
        return 0;
    }
}

int probdata_server_udt_prof_default(struct probdata* prob)
{
    return f_udt_prof_default(prob->prob_udt_prof);
}

int probdata_server_udt_prof_init(struct probdata* prob)
{
    // set_role(pdt->fp_tpg_test->prof, PROGRAM_SERVER);
    prob->prob_control->role = (prob->role == P_CLIENT) ? (F_CLIENT) : (F_SERVER);
    // set_protocol_id(pdt->fp_tpg_test->prof, TPG_UDT);
    // this is done in fastprof since we know it is UDT
    memcpy(prob->prob_control->remote_ip, prob->remote_ip, strlen(prob->remote_ip));
    (prob->prob_control->remote_ip)[strlen(prob->remote_ip)] = '\0';
    prob->prob_control->udt_port = prob->udt_port;
    prob->prob_control->duration = prob->prof_duration;
    prob->prob_control->max_limit = prob->max_prof_limit;
    prob->prob_control->impeded_limit = prob->impeded_limit;
    prob->prob_control->pgr = prob->pgr;
    
    return f_udt_prof_init(prob->prob_udt_prof, prob->prob_control);
}

int probdata_server_udt_fastprof_run(struct probdata* prob)
{
    int ret = -1;
    
    P_LOG(P_PERF, "\n***FASTPROF UDT-BASED SERVER IS STARTED***\n");
    
    if (probdata_server_udt_prof_new(prob) < 0) {
        return -1;
    }
    
    if (probdata_server_udt_prof_default(prob) < 0) {
        return -1;
    }
    
    if (probdata_server_udt_prof_init(prob) < 0) {
        return -1;
    }
    
    prob->prob_udt_worker = new pthread_t;
    ret = pthread_create(prob->prob_udt_worker, NULL, probdata_server_udt_fastprof_worker, (void *)prob);
    if (ret != 0) {
        P_LOG(P_HINT, "cannot create threads");
        p_errno = P_ERR_THREAD;
        return -1;
    } else {
        return 0;
    }
}

int probdata_server_udt_prof_reset(struct probdata* prob)
{
    return f_udt_prof_reset(prob->prob_udt_prof, prob->prob_control);
}

void* probdata_server_udt_fastprof_worker(void *prob)
{
    int err_cnt = 0;
    int ok_cnt = 0;
    int ret = -1;
    struct probdata* prob_local = (struct probdata*) prob;
    
    P_LOG(P_HINT, "FASTPROF UDT-BASED SERVER WORKER IS STARTED");
    
    while(prob_local->status == P_RUN_M2M_UDT) {
        if (ok_cnt > prob_local->max_prof_limit) {
            P_LOG(P_PERF, "one-time profiling limit is reached - %d", ok_cnt);
            break;
        }
        if (err_cnt >= P_MAX_ERROR_NUM) {
            P_LOG(P_PERF, "[abort]: too many errors - [%d]\n", err_cnt);
            break;
        }
        // after 15 seconds it will return 0
        ret = tpg_server_run(prob_local->prob_udt_prof->udt_prof, 15, 0);
        if (ret < 0) {
            if (probdata_server_udt_prof_reset(prob_local) < 0) {
                P_LOG(P_PERF, "probdata reset errors - [%s]\n", probdata_errstr(p_errno));
                return NULL;
            }
            P_LOG(P_PERF, "%s\n", probdata_errstr(p_errno));
            err_cnt++;
            
            if (err_cnt > P_MAX_ERROR_NUM){
                P_LOG(P_PERF, "[abort]: too many errors - [%d]\n", err_cnt);
                break;
            }
        } else if (ret == 0) {
            continue; 
        } else {
            err_cnt = 0;
            ok_cnt = ok_cnt + 1;
        }
    }
    
    return NULL;
}

void probdata_server_udt_fastprof_cleanup_handler(void* prob)
{
    return ;
}

int probdata_server_dw_xdd_udt(probdata* prob)
{
    P_LOG(P_PERF, "\n***XDD UDT-BASED SERVER IS STARTED***\n");
    usleep(2500000);
    P_LOG(P_PERF, "\n***XDD UDT-BASED PROFILING IS DONE***\n");

    return 0;
}

int probdata_server_dw_xdd_tcp(probdata* prob)
{
    P_LOG(P_PERF, "\n***XDD TCP-BASED SERVER IS STARTED***\n");
    usleep(2500000);
    P_LOG(P_PERF, "\n***XDD TCP-BASED PROFILING IS DONE***\n");
    return 0;
}
