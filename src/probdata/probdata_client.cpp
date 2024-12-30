/***************************************************************************
 * ProbData - PRofiling Optimization Based DAta Transfer Advisor
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
// #include "../tpg/tpg_profile.h"
#include <tpg_profile.h>
#include "../fastprof/fastprof.h"
#include "../fastprof/fastprof_udt.h"
#include "../fastprof/fastprof_tcp.h"
#include "probdata_control.h"
#include "probdata_error.h"
#include "probdata_log.h"
#include "probdata.h"
#include "probdata_client.h"

int probdata_client(struct probdata* prob)
{
    int ret = 0;

    P_LOG(P_HINT, "probdata client is starting...");
    
    P_LOG(P_PERF, "%s", p_program_version);
    system("uname -a");
    system("date");
    
    if (prob->rtt_ms < 0) {
        P_LOG(P_PERF, "+++++++++++++++++++++++++++++++++++++++++++++++++");
        P_LOG(P_PERF, "*** Estimating RTT...");
        if ((prob->rtt_ms = f_estimate_rtt(prob->remote_ip)) < 0) {
            P_LOG(P_HINT, "Cannot ping...");
        } else {
            P_LOG(P_PERF,
                "    Measured RTT: %.3lf ms", prob->rtt_ms);
            prob->prob_control->rtt_ms = prob->rtt_ms;
        }
    } else {
        ;
    }

    if (probdata_client_connect(prob) < 0) {
        return -1;
    }

    prob->status = P_PROGRAM_START;
    prob->start_time_us = prob->end_time_us = t_get_currtime_us();

    while (prob->status != P_PROGRAM_DONE) {
        ;

        ret = probdata_client_poll_ctrl_channel(prob);
        if (ret < 0) {
            p_errno = P_ERR_CTRL_CHAN;
            return -1;
        } else if (ret == 0) {
            continue;
        } else {
            if (probdata_client_proc_ctrl_packet(prob) < 0) {
                return -1;
            } else {
                P_LOG(P_HINT, "client proc %s", probdata_status_to_str(prob->status));
            }
        }
        usleep(100000);
    }
    
    P_LOG(P_PERF, "\n+++++++++++++++++++++++++++++++++++++++++++++++++");
    P_LOG(P_PERF, "*** RECOMMENDED CONTROL PARAMETER VALUES:");
    P_LOG(P_PERF, "    -----------------------------------------");
    if (prob->prob_udt_prof_max->perf_Mbps > prob->prob_tcp_prof_max->perf_Mbps && 
        prob->prob_udt_prof_max->perf_Mbps > prob->m2m_record_max->average_perf) {
        P_LOG(P_PERF, "       SrcIP: %s", prob->prob_control->local_ip);
        P_LOG(P_PERF, "       DstIP: %s", prob->prob_control->remote_ip);
        P_LOG(P_PERF, "    Protocol: %s", "UDT");
        P_LOG(P_PERF, "     PktSize: %d", prob->prob_udt_prof_max->udt_prof->udt_mss - 44);
        P_LOG(P_PERF, "     BlkSize: %d", prob->prob_udt_prof_max->udt_prof->blocksize);
        P_LOG(P_PERF, "     BufSize: %d", prob->prob_udt_prof_max->udt_prof->udt_send_bufsize);
        P_LOG(P_PERF, "     Stream#: %d", prob->prob_udt_prof_max->udt_prof->total_stream_num);
        P_LOG(P_PERF, "     MaxPerf: %.3lf Mbps", prob->prob_udt_prof_max->perf_Mbps);
    }
    if (prob->prob_tcp_prof_max->perf_Mbps > prob->prob_udt_prof_max->perf_Mbps &&
        prob->prob_tcp_prof_max->perf_Mbps > prob->m2m_record_max->average_perf) {
        P_LOG(P_PERF, "       SrcIP: %s", prob->prob_control->local_ip);
        P_LOG(P_PERF, "       DstIP: %s", prob->prob_control->remote_ip);
        P_LOG(P_PERF, "    Protocol: %s", "TCP");
        P_LOG(P_PERF, "     PktSize: %d", prob->prob_tcp_prof_max->tcp_prof->settings->mss);
        P_LOG(P_PERF, "     BlkSize: %d", prob->prob_tcp_prof_max->tcp_prof->settings->blksize);
        P_LOG(P_PERF, "     BufSize: %d", prob->prob_tcp_prof_max->tcp_prof->settings->socket_bufsize);
        P_LOG(P_PERF, "     Stream#: %d", prob->prob_tcp_prof_max->tcp_prof->num_streams);
        P_LOG(P_PERF, "     MaxPerf: %.3lf Mbps", prob->prob_tcp_prof_max->perf_Mbps);
    }
    if (prob->m2m_record_max->average_perf > prob->prob_tcp_prof_max->perf_Mbps && 
        prob->m2m_record_max->average_perf > prob->prob_udt_prof_max->perf_Mbps) {
        f_print_m2m_record(prob->m2m_record_max);
    }
    P_LOG(P_PERF, "    -----------------------------------------");
    P_LOG(P_PERF, "+++++++++++++++++++++++++++++++++++++++++++++++++");

    system("date");

    return 0;
}

int probdata_client_connect(struct probdata* prob)
{
	int len = 0;

    P_LOG(P_HINT, "probdata client is connecting to the server");
    
    if (prob->accept_sock < 0) {
        if (probdata_ctrl_chan_connect(prob) < 0) {
            P_LOG(P_ERR, "cannot connect to server [%d]", prob->accept_sock);
            p_errno = P_ERR_CTRL_CHAN;
            return -1;
        } else {
            P_LOG(P_HINT,
                "probdata client is connected to server TCP channel - [%s|%d]",
                prob->remote_ip, prob->listen_sock);
        }
    }
    
    if (probdata_client_check_id(prob) < 0) {
        P_LOG(P_HINT, "client ID check fail");
        return -1;
    } else {
        P_LOG(P_HINT, "client ID check success");
    }
    
    if (probdata_ctrl_chan_recv_Nbytes(prob->accept_sock, (char*)&len, sizeof(int)) < 0){
        p_errno = P_ERR_UNKNOWN;
        return -1;
    }
    if (probdata_ctrl_chan_recv_Nbytes(prob->accept_sock, prob->local_ip, len) < 0){
        p_errno = P_ERR_UNKNOWN;
        return -1;
    }
    
    memcpy(prob->prob_control->local_ip, prob->local_ip, strlen(prob->local_ip));

    return 0;
}

int probdata_client_poll_ctrl_channel(struct probdata* prob)
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

    ret_select = select(prob->max_sock + 1, &(prob->read_sock_set), NULL, NULL, &tv);

    if (ret_select < 0) {
        p_errno = P_ERR_CTRL_CHAN;
        return -1;
    } else if(ret_select == 0) {
        return 0;
    } else {
        if ((prob->accept_sock > 0) && FD_ISSET(prob->accept_sock, &(prob->read_sock_set))) {
            ret_size = probdata_ctrl_chan_recv_Nbytes(prob->accept_sock, (char*)&(prob->status), sizeof(signed char));
            if (ret_size <= 0) {
                P_LOG(P_HINT, "cannot read from control channel");
                return -1;
            } else {
                return ret_size;
            }
        } else {
            P_LOG(P_ERR, "either invalid TCP socket or not set in read socket set");
            p_errno = P_ERR_CTRL_CHAN;
            return -1;
        }
    }

    FD_CLR(prob->accept_sock, &(prob->read_sock_set));
    prob->max_sock = -1;

    P_LOG(P_HINT, "a control packet with type [%d|%s] is received", prob->status, probdata_status_to_str(prob->status));

    return ret_select;
}

int probdata_client_proc_ctrl_packet(struct probdata* prob)
{
    // int err = -1;
    char opt = 'x';

    switch (prob->status) {
    case P_CTRL_CHAN_EST:
        {
            prob->status = P_CTRL_CHAN_EST;
            P_LOG(P_HINT, "state -> PD_CTRL_CHAN_ESTABLISHED");
            
            if (probdata_ctrl_chan_send_ctrl_packet(prob, P_TWOWAY_HANDSHAKE) < 0) {
                p_errno = P_ERR_CTRL_CHAN;
                return -1;
            } else {
                P_LOG(P_HINT, "ctrl packet P_TWOWAY_HANDSHAKE is sent");
            }
            break;
        }
	case P_TWOWAY_HANDSHAKE:
        {
            prob->status = P_TWOWAY_HANDSHAKE;
            P_LOG(P_HINT, "state -> P_TWOWAY_HANDSHAKE");
            if (probdata_ctrl_chan_ctrlmsg_exchange(prob) < 0) {
                P_LOG(P_HINT, "ctrl info cannot be exchanged");
                return -1;
            } else {
                P_LOG(P_HINT, "ctrl info is exchanged");
            }
            if (strlen(prob->src_file) > 0) {
                if (probdata_ctrl_chan_send_ctrl_packet(prob, P_RUN_D2D_DB) < 0) {
                    p_errno = P_ERR_CTRL_CHAN;
                    return -1;
                } else {
                    ;
                }
            } else if (strlen(prob->dst_file) > 0) {
                if (probdata_ctrl_chan_send_ctrl_packet(prob, P_CLIENT_TERMINATE) < 0) {
                    p_errno = P_ERR_CTRL_CHAN;
                    return -1;
                } else {
                    ;
                }
                p_errno = P_ERR_DSTFILE_ONLY;
                return -1;
            } else {
                if (probdata_ctrl_chan_send_ctrl_packet(prob, P_RUN_M2M_DB) < 0) {
                    p_errno = P_ERR_CTRL_CHAN;
                    return -1;
                } else {
                    ;
                }
            }
            break;
        }
	case P_RUN_D2D_DB:
        {
            prob->status = P_RUN_D2D_DB;
            P_LOG(P_HINT, "state -> P_RUN_D2D_DB");
            if (probdata_client_search_d2d(prob) < 0) {
                P_LOG(P_PERF, "    Cannot find historical data will do online profiling");
                if (probdata_ctrl_chan_send_ctrl_packet(prob, P_RUN_D2D_TCP) < 0) {
                    p_errno = P_ERR_CTRL_CHAN;
                    return -1;
                } else {
                    ;
                }
            } else {
                P_LOG(P_PERF, "    Below is what we've found in the historical data:");
                P_LOG(P_PERF, "    -----------------------------------------");
                // fastprof_print_d2d_record(prob->m2m_record_max);
                P_LOG(P_PERF, "    -----------------------------------------");

                while (opt != 'y' && opt != 'N') {
                    printf("\n*** DO ONLINE PROFILING? [y/N]:");
                    fflush(stdout);
                    scanf(" %c", &opt);
                }
                if (opt == 'y') {
                    P_LOG(P_HINT, "do UDT only %d", prob->do_udt_only);
                    if (prob->do_udt_only > 0) {
                        if (probdata_ctrl_chan_send_ctrl_packet(prob, P_RUN_D2D_UDT) < 0) {
                            p_errno = P_ERR_CTRL_CHAN;
                            return -1;
                        } else {
                            ;
                        }
                    } else {
                        if (probdata_ctrl_chan_send_ctrl_packet(prob, P_RUN_D2D_TCP) < 0) {
                            p_errno = P_ERR_CTRL_CHAN;
                            return -1;
                        } else {
                            ;
                        }
                    }
                } else {
                    P_LOG(P_PERF, "Operation aborted.");
                    if (probdata_ctrl_chan_send_ctrl_packet(prob, P_PROGRAM_DONE) < 0) {
                        p_errno = P_ERR_CTRL_CHAN;
                        return -1;
                    } else {
                        ;
                    }
                }
            }
            break;
        }
	case P_RUN_D2D_TCP:
        {
            prob->status = P_RUN_D2D_TCP;

            P_LOG(P_HINT, "state -> P_RUN_D2D_TCP");

            if (probdata_client_dw_xdd_tcp(prob) < 0) {
                p_errno = P_ERR_UNKNOWN;
                return -1;
            } else {
                ;
            }

            if (probdata_ctrl_chan_send_ctrl_packet(prob, P_RUN_D2D_UDT) < 0) {
                p_errno = P_ERR_CTRL_CHAN;
                return -1;
            } else {
                ;
            }
            break;
        }
	case P_RUN_D2D_UDT:
        {
            prob->status = P_RUN_D2D_UDT;

            P_LOG(P_HINT, "state ---> P_RUN_D2D_UDT");

            if (probdata_client_dw_xdd_udt(prob) < 0) {
                p_errno = P_ERR_UNKNOWN;
                return -1;
            } else {
                ;
            }

            /* organizing results */
            ;
            if (probdata_ctrl_chan_send_ctrl_packet(prob, P_PROGRAM_DONE) < 0) {
                p_errno = P_ERR_CTRL_CHAN;
                return -1;
            } else {
                ;
            }
            break;
        }
	case P_RUN_M2M_DB:
        {
            prob->status = P_RUN_M2M_DB;

            P_LOG(P_HINT, "state -> P_RUN_M2M_DB");
            if (probdata_client_search_m2m(prob) < 0) {
                P_LOG(P_PERF, "    Cannot find historical data do online profiling");
                if (probdata_ctrl_chan_send_ctrl_packet(prob, P_RUN_M2M_TCP) < 0) {
                    p_errno = P_ERR_CTRL_CHAN;
                    return -1;
                } else {
                    ;
                }
            } else {
                P_LOG(P_PERF, "    Below is what we've found in the historical data:");
                P_LOG(P_PERF, "    -----------------------------------------");
                f_print_m2m_record(prob->m2m_record_max);
                P_LOG(P_PERF, "    -----------------------------------------");

#ifdef PROBDATA_DEBUG
                opt = 'y';
#endif

                while (opt != 'y' && opt != 'N') {
                    P_LOG(P_PERF, "\n+++++++++++++++++++++++++++++++++++++++++++++++++");
                    printf("*** DO ONLINE PROFILING? [y/N]:");
                    fflush(stdout);
                    scanf(" %c", &opt);
                }

                if (opt == 'y') {
                    P_LOG(P_HINT, "do UDT only %d", prob->do_udt_only);
                    if (prob->do_udt_only > 0) {
                        if (probdata_ctrl_chan_send_ctrl_packet(prob, P_RUN_M2M_UDT) < 0) {
                            p_errno = P_ERR_CTRL_CHAN;
                            return -1;
                        } else {
                            ;
                        }
                    } else {
                        if (probdata_ctrl_chan_send_ctrl_packet(prob, P_RUN_M2M_TCP) < 0) {
                            p_errno = P_ERR_CTRL_CHAN;
                            return -1;
                        } else {
                            ;
                        }
                    }
                } else {
                    P_LOG(P_PERF, "Operation aborted.");
                    if (probdata_ctrl_chan_send_ctrl_packet(prob, P_PROGRAM_DONE) < 0) {
                        p_errno = P_ERR_CTRL_CHAN;
                        return -1;
                    } else {
                        ;
                    }
                }
            }
            break;
        }
	case P_RUN_M2M_TCP:
        {
            prob->status = P_RUN_M2M_TCP;

            P_LOG(P_HINT, "state ---> P_RUN_M2M_TCP");

            if (probdata_client_tcp_fastprof_run(prob) < 0) {
                return -1;
            } else {
                ;
            }

            if (probdata_ctrl_chan_send_ctrl_packet(prob, P_RUN_M2M_UDT) < 0) {
                p_errno = P_ERR_CTRL_CHAN;
                return -1;
            } else {
                ;
            }
            break;
        }
	case P_RUN_M2M_UDT:
        {
            prob->status = P_RUN_M2M_UDT;

            P_LOG(P_HINT, "state -> P_RUN_M2M_UDT");

            if (probdata_client_udt_fastprof_run(prob) < 0) {
                return -1;
            } else {
                ;
            }

            /* organizing results */
            ;
            if (probdata_ctrl_chan_send_ctrl_packet(prob, P_PROGRAM_DONE) < 0) {
                p_errno = P_ERR_CTRL_CHAN;
                return -1;
            } else {
                ;
            }
            break;
        }
	case P_PROGRAM_DONE:
        {
            prob->status = P_PROGRAM_DONE;
            P_LOG(P_HINT, "state -> P_PROGRAM_DONE");
            P_LOG(P_HINT, "cleans up (Not yet implemented)");
            return 0;
        }
	case P_SERVER_ERROR:
        {
            ;
            break;
        }
	case P_ACCESS_DENIED:
        {
            ;
            break;
        }
	default:
        {
            p_errno = P_ERR_UNKNOWN;
            return -1;
        }
    }

    return 0;
}

void probdata_client_id_gen(char *id)
{
    int currpid;
    char hostname[64];
    char temp[256];
    time_t t;
    struct tm currtime;

    currpid = getpid();
    srand(time(NULL));
    gethostname(hostname, sizeof(hostname));
    t = time(NULL);
    currtime = *localtime(&t);
    sprintf(temp, "%s.%ld.%06ld.%08lx%08lx.%s",
            hostname,
            (unsigned long int) (currtime.tm_min ^ rand()),
            (unsigned long int) (currtime.tm_sec ^ rand()),
            (unsigned long int) (rand() ^ currpid),
            (unsigned long int) (rand() ^ currpid),
            "1234567890123456789012345678901234567890");
    strncpy(id, temp, P_IDPKT_SIZE);
    id[P_IDPKT_SIZE - 1] = '\0';

}

int probdata_client_check_id(struct probdata* prob)
{
    probdata_client_id_gen(prob->id);
    if (probdata_ctrl_chan_send_Nbytes(prob->accept_sock, prob->id, P_IDPKT_SIZE) < 0) {
        P_LOG(P_HINT, "cannot send client ID to server");
        p_errno = P_ERR_UNKNOWN;
        return -1;
    } else {
        P_LOG(P_HINT, "client ID check ok");
        return 0;
    }

	return 0;
}

double probdata_client_search_d2d(struct probdata* prob)
{
    double perf_tcp = -1.0;
    double perf_udt = -1.0;

    P_LOG(P_PERF, "\n*** SEARCH IN DISK-DISK DATA ***\n");

    // search in tcp-based data
    perf_tcp = 0.1;

    // search in udt-based data
    perf_udt = 0.2;

    return -1;
    
    // return (perf_tcp > perf_udt) ? perf_tcp : perf_udt;
    if (perf_udt > perf_tcp) {
        P_LOG(P_PERF, "\n*** SEARCH IN DISK-DISK DATA IS DONE ***\n");
        return perf_udt;
    } else {
        P_LOG(P_PERF, "\n*** SEARCH IN DISK-DISK DATA IS DONE ***\n");
        return perf_tcp;
    }
}

double probdata_client_search_m2m(struct probdata* prob)
{
    P_LOG(P_PERF, "\n+++++++++++++++++++++++++++++++++++++++++++++++++");
    P_LOG(P_PERF, "*** SEARCH IN MEM-MEM HISTORICAL PROFILING DATA");

    if (f_m2m_db_search(prob->db_file_handler, prob->m2m_record_max, prob->prob_control) < 0) {
        return -1;
    } else {
        return 0;
    }

    /*
    // search in tcp-based data
    perf_tcp = 0.1;

    // search in udt-based data
    perf_udt = 0.2;

    return -1;

    // return (perf_tcp > perf_udt) ? perf_tcp : perf_udt;
    if (perf_udt > perf_tcp) {
        P_LOG(P_PERF,
                "\n***SEARCH IN MEM-MEM DATA IS DONE***\n");
        return perf_udt;
    } else {
        P_LOG(P_PERF,
                "\n***SEARCH IN MEM-MEM DATA IS DONE***\n");
        return perf_tcp;
    }
    */
}

int probdata_client_clean_up(struct probdata* prob)
{
    P_LOG(P_PERF, "\n*** CLIENT IS CLEANING UP ***\n   (NOT YET TOTALLY IMPLEMENTED)\n");

    if (f_tcp_prof_delete(prob->prob_tcp_prof) < 0) {
        return -1;
    }
    if (f_udt_prof_delete(prob->prob_udt_prof) < 0) {
        return -1;
    }

    /* clean up for fastprof */
    ;

    /* clean up for probdata */
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
    delete prob->prob_udt_worker;
    prob->prob_udt_worker = NULL;
    delete prob->prob_tcp_worker;
    prob->prob_udt_worker = NULL;
    delete prob->prob_control;
    prob->prob_control = NULL;

    return 0;
}

int probdata_client_dw_xdd_tcp(probdata* prob)
{
    P_LOG(P_PERF, "\n*** XDD TCP-BASED CLIENT IS STARTED ***\n");
    usleep(2500000);
    P_LOG(P_PERF, "\n*** XDD TCP-BASED CLIENT IS DONE ***\n");
    return 0;
}

int probdata_client_dw_xdd_udt(probdata* prob)
{
    P_LOG(P_PERF, "\n*** XDD UDT-BASED CLIENT IS STARTED ***\n");
    usleep(2500000);
    P_LOG(P_PERF, "\n*** XDD UDT-BASED CLIENT IS DONE ***\n");
    return 0;
}

int probdata_client_tcp_prof_new(probdata* prob)
{
    prob->prob_tcp_prof = f_tcp_prof_new();
    if (prob->prob_tcp_prof == NULL) {
        p_errno = P_ERR_FASTPROF;
        return -1;
    } else {
        return 0;
    }

    prob->prob_tcp_prof_max = f_tcp_prof_new();
    if (prob->prob_tcp_prof_max == NULL) {
        p_errno = P_ERR_FASTPROF;
        return -1;
    } else {
        return 0;
    }
}

int probdata_client_tcp_prof_default(probdata* prob)
{
    /*
    if(iperf_defaults(pdt->fp_iperf_test->test) < 0) {
        pd_errno = PDE_IPERF3;
        return -1;
    } else {
        return 0;
    } */

    return f_tcp_prof_default(prob->prob_tcp_prof);
}

int probdata_client_tcp_prof_init(probdata* prob)
{
    // iperf_set_verbose(pdt->fp_iperf_test->test, 0);
    prob->prob_control->role = (prob->role == P_CLIENT) ? (F_CLIENT) : (F_SERVER);
    memcpy(prob->prob_control->local_ip, prob->local_ip, strlen(prob->local_ip));
    (prob->prob_control->local_ip)[strlen(prob->local_ip)] = '\0';
    memcpy(prob->prob_control->remote_ip, prob->remote_ip, strlen(prob->remote_ip));
    (prob->prob_control->remote_ip)[strlen(prob->remote_ip)] = '\0';
    prob->prob_control->tcp_port = prob->tcp_port;
    // iperf_set_test_reverse(pdt->fp_iperf_test->test, 0);
    // iperf_set_test_omit(pdt->fp_iperf_test->test, 1);
    prob->prob_control->duration = prob->prof_duration;
    
    return f_tcp_prof_init(prob->prob_tcp_prof, prob->prob_control);
}

int probdata_client_tcp_fastprof_run(probdata* prob)
{
    P_LOG(P_PERF, "\n+++++++++++++++++++++++++++++++++++++++++++++++++");
    P_LOG(P_PERF, "*** FASTPROF TCP-BASED CLIENT IS STARTED");

    if (probdata_client_tcp_prof_new(prob) < 0) {
        return -1;
    }
    if (probdata_client_tcp_prof_default(prob) < 0) {
        return -1;
    }
    if (probdata_client_tcp_prof_init(prob) < 0) {
        return -1;
    }

    if (f_tcp_client_run(prob->prob_tcp_prof, prob->prob_tcp_prof_max, prob->prob_control) < 0) {
        P_LOG(P_ERR, "fastprof client run failed");
        return -1;
    }

    P_LOG(P_PERF, "    FASTPROF TCP-BASED CLIENT IS DONE");
    P_LOG(P_PERF, "+++++++++++++++++++++++++++++++++++++++++++++++++");

    return 0;
}

int probdata_client_tcp_prof_reset(probdata *prob)
{
    return f_tcp_prof_reset(prob->prob_tcp_prof, prob->prob_control);
}


int probdata_client_udt_prof_new(probdata* prob)
{
    prob->prob_udt_prof = f_udt_prof_new();
    if (prob->prob_udt_prof == NULL) {
        p_errno = P_ERR_FASTPROF;
        return -1;
    } else {
        return 0;
    }

    prob->prob_udt_prof_max = f_udt_prof_new();
    if (prob->prob_udt_prof_max == NULL) {
        p_errno = P_ERR_FASTPROF;
        return -1;
    } else {
        return 0;
    }
}

int probdata_client_udt_prof_default(struct probdata* prob)
{
    return f_udt_prof_default(prob->prob_udt_prof);
}

int probdata_client_udt_prof_init(struct probdata* prob)
{
    prob->prob_control->role = (prob->role == P_CLIENT) ? (F_CLIENT) : (F_SERVER);

    // set protocol id
    // this is done in fastprof since we know it is UDT

    memcpy(prob->prob_control->local_ip, prob->local_ip, strlen(prob->local_ip));
    (prob->prob_control->local_ip)[strlen(prob->local_ip)] = '\0';
    memcpy(prob->prob_control->remote_ip, prob->remote_ip, strlen(prob->remote_ip));
    (prob->prob_control->remote_ip)[strlen(prob->remote_ip)] = '\0';
    prob->prob_control->duration = prob->prof_duration;
    prob->prob_control->max_limit = prob->max_prof_limit;
    prob->prob_control->impeded_limit = prob->impeded_limit;
    prob->prob_control->pgr = prob->pgr;
    prob->prob_control->mss = prob->mss;

    // set number of streams
    ;

    return f_udt_prof_init(prob->prob_udt_prof, prob->prob_control);
}

int probdata_client_udt_fastprof_run(probdata* prob)
{
    P_LOG(P_PERF, "\n+++++++++++++++++++++++++++++++++++++++++++++++++");
    P_LOG(P_PERF, "*** FASTPROF UDT-BASED CLIENT IS STARTED");

    if (probdata_client_udt_prof_new(prob) < 0) {
        return -1;
    }

    if (probdata_client_udt_prof_default(prob) < 0) {
        return -1;
    }

    if (probdata_client_udt_prof_init(prob) < 0) {
        return -1;
    }

    if (f_udt_client_run(prob->prob_udt_prof, prob->prob_udt_prof_max, prob->prob_control) < 0) {
        P_LOG(P_ERR, "fastprof failed");
        return -1;
    }

    P_LOG(P_PERF, "    FASTPROF UDT-BASED CLIENT IS DONE");
    P_LOG(P_PERF, "+++++++++++++++++++++++++++++++++++++++++++++++++");

    return 0;
}

int probdata_client_m2m_record_send(probdata* prob, struct mem_record *rec)
{
    if (prob->role == P_CLIENT) {
        if (probdata_ctrl_chan_send_Nbytes(prob->accept_sock, (char*)rec, sizeof(struct mem_record)) < 0) {
            p_errno = P_ERR_UNKNOWN;
            return -1;
        }
    } else {
        p_errno = P_ERR_SERV_CLIENT;
        return -1;
    }

    return 0;
}
