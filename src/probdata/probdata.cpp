/***************************************************************************
 * ProbData - PRofiling Optimization Based DAta Transfer Advisor
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cmath>
#include <stdarg.h>
#include <getopt.h>
#include <udt.h>
#include "probdata.h"
#include "probdata_control.h"
#include "probdata_error.h"
#include "probdata_log.h"

int probdata_new_and_default(probdata* prob)
{
    prob->role = P_UNKNOWN;
    prob->listen_sock = -1;
    prob->accept_sock = -1;
    prob->max_sock = -1;
    FD_ZERO(&(prob->read_sock_set));
    prob->ctrl_port = P_DEFAULT_PORT;
    prob->tcp_port = P_DEFAULT_TCP_PORT;
    prob->udt_port = P_DEFAULT_UDT_PORT;
    memset(prob->local_ip, '\0', P_HOSTADDR_LEN);
    memset(prob->remote_ip, '\0', P_HOSTADDR_LEN);
    memset(prob->src_file, '\0', P_FILENAME_LEN);
    memset(prob->dst_file, '\0', P_FILENAME_LEN);
    memset(prob->id, '\0', P_IDPKT_SIZE);
    prob->instance = NULL;
    prob->max_perf_mbps = -1.0;
    prob->start_time_us = -1; 
    prob->end_time_us = -1;
    prob->mss = P_DEFAULT_PACKET;
    prob->rtt_ms = -1.0;
    prob->prof_duration = P_DEFAULT_DURATION;
    prob->max_prof_limit = P_DEFAULT_MAX_PROF_LIMIT;
    prob->impeded_limit = P_DEFAULT_IMPEDED_LIMIT;
    prob->pgr = P_DEFAULT_PGR;
    prob->do_udt_only = -1;
    prob->host_max_buf = 2 * P_GB;
    prob->bandwidth = -1.0;
    prob->spsa_A_val = P_DEFAULT_SPSA_BIG_A_VAL;
    prob->spsa_a_val = P_DEFAULT_SPSA_SMALL_A_VAL;
    prob->spsa_a_scale = P_DEFAULT_SCALE_FACTOR_A_VAL;
    prob->spsa_c_val = P_DEFAULT_SPSA_SMALL_C_VAL;
    prob->spsa_c_scale = P_DEFAULT_SCALE_FACTOR_C_VAL;
    prob->spsa_alpha_val = P_DEFAULT_SPSA_ALPHA;
    prob->spsa_gamma_val = P_DEFAULT_SPSA_GAMMA;
    prob->prob_tcp_prof = new fastprof_tcp_prof;
    prob->prob_tcp_prof_max = new fastprof_tcp_prof;
    prob->prob_udt_prof = new fastprof_udt_prof;
    prob->prob_udt_prof_max = new fastprof_udt_prof;
    prob->prob_control = new fastprof_ctrl;
    prob->prob_control_max = new fastprof_ctrl;
    prob->prob_udt_worker = NULL;
    prob->prob_tcp_worker = NULL;
    prob->m2m_record = new struct mem_record;
    prob->m2m_record_max = new struct mem_record;
    
    return 0;
}

int probdata_delete(struct probdata* prob)
{
    if (prob->prob_udt_prof != NULL) {
        delete prob->prob_udt_prof;
        prob->prob_udt_prof = NULL;
    }
    if (prob->prob_udt_prof_max != NULL) {
        delete prob->prob_udt_prof_max;
        prob->prob_udt_prof_max = NULL;
    }
    if (prob->prob_tcp_prof != NULL) {
        delete prob->prob_tcp_prof;
        prob->prob_tcp_prof = NULL;
    }
    if (prob->prob_tcp_prof_max != NULL) {
        delete prob->prob_tcp_prof_max;
        prob->prob_tcp_prof_max = NULL;
    }
    if (prob->m2m_record != NULL) {
        delete prob->m2m_record;
        prob->m2m_record = NULL;
    }
    if (prob->m2m_record_max != NULL) {
        delete prob->m2m_record_max;
        prob->m2m_record_max = NULL;
    }
    if (prob->prob_control != NULL) {
        delete prob->prob_control;
        prob->prob_control = NULL;
    }
    if (prob->prob_control_max != NULL) {
        delete prob->prob_control_max;
        prob->prob_control_max = NULL;
    }
    if (prob->prob_udt_worker != NULL) {
        delete prob->prob_udt_worker;
        prob->prob_udt_worker = NULL;
    }
    if (prob->prob_tcp_worker != NULL) {
        delete prob->prob_tcp_worker;
        prob->prob_tcp_worker = NULL;
    }
    return 0;
}

int probdata_load_conf(probdata* prob)
{
    P_LOG(P_HINT, "LOAD CONFIGURATION FILE");
    return 0;
}

int probdata_set_role(probdata* prob, p_program_role role)
{
    prob->role = role;
    
    if (role == P_CLIENT || role == P_SERVER) {
        return 0;
    } else {
        p_errno = P_ERR_SERV_CLIENT;
        return -1;
    }
}

int probdata_set_local_addr(probdata* prob, char* addr)
{
    memcpy(prob->local_ip, addr, strlen(addr));
    prob->local_ip[strlen(addr)] = '\0';
    return 0;
}

int probdata_set_peer_addr(probdata* prob, char* addr)
{
    memcpy(prob->remote_ip, addr, strlen(addr));
    prob->remote_ip[strlen(addr)] = '\0';
    return 0;
}

int probdata_set_src_file(probdata* prob, char* src_filename)
{
    memcpy(prob->src_file, src_filename, strlen(src_filename));
    prob->src_file[strlen(src_filename)] = '\0';
    return 0;
}

int probdata_set_dst_file(probdata* prob, char* dst_filename)
{
    memcpy(prob->dst_file, dst_filename, strlen(dst_filename));
    prob->dst_file[strlen(dst_filename)] = '\0';
    return 0;
}

int probdata_cmd_parse(probdata* prob, int argc, char** argv)
{
    static struct option longopts[] =
    {
        {"Port", required_argument, NULL, 'p'},
        {"Server", no_argument, NULL, 's'},
        {"Client", required_argument, NULL, 'c'},
        {"Help", no_argument, NULL, 'h'},
        {"SrcFile", required_argument, NULL, 'S'},
        {"DstFile", required_argument, NULL, 'V'},
        {"UDT", no_argument, NULL, 't'},
        {"Duration", required_argument, NULL, 'd'},
        {"MSS", required_argument, NULL, 'M'},
        {"Bandwidth", required_argument, NULL, 'y'},
        {"PGR", required_argument, NULL, 'C'},
        {"Delay", required_argument, NULL, 'T'},
        {"ImpededLimit", required_argument, NULL, 'L'},
        {"NaxLimit", required_argument, NULL, 'N'},
        {NULL, 0, NULL, 0}
    };
    
    int prob_src_file_flag = -1;
    int prob_dst_file_flag = -1;
    
    int flag;
    while ((flag = getopt_long(argc, argv, "vhp:sc:S:V:td:M:y:C:T:L:N:e:g:o:x:", longopts, NULL)) != -1) {

        switch (flag) {
            case 'p': 
            {
                prob->ctrl_port = atoi(optarg);
                break;
            }
            case 's':
            {
                if (prob->role == P_CLIENT) {
                    p_errno = P_ERR_SERV_CLIENT;
                    return -1;
                } else {
                    probdata_set_role(prob, P_SERVER);
                }
                break;
            }
            case 'c':
            {
                if (prob->role == P_SERVER) {
                    p_errno = P_ERR_SERV_CLIENT;
                    return -1;
                } else {
                    probdata_set_role(prob, P_CLIENT);
                    probdata_set_peer_addr(prob, optarg);
                }
                break;
            }
            case 'S':
            {
                probdata_set_src_file(prob, optarg);
                prob_src_file_flag = 1;
                break;
            }
            case 'V':
            {
                probdata_set_dst_file(prob, optarg);
                prob_dst_file_flag = 1;
                break;
            }
            case 't':
            {
                prob->do_udt_only = 1;
                break;
            }
            case 'd':
            {
                prob->prof_duration = atoi(optarg);
                break;
            }
            case 'M':
            {
                prob->mss = atoi(optarg);
                break;
            }
            case 'y':
            {
                prob->bandwidth = probdata_unit_atof(optarg);
                break;
            }
            case 'C':
            {
                prob->pgr = atof(optarg);
                break;
            }
            case 'T':
            {
                prob->rtt_ms = atof(optarg);
                break;
            }
            case 'L':
            {
                prob->impeded_limit = atoi(optarg);
                break;
            }
            case 'N':
            {
                prob->max_prof_limit = atoi(optarg);
                break;
            }
            case 'e':
            {
                prob->spsa_a_val = atof(optarg);
                break;
            }
            case 'g':
            {
                prob->spsa_c_val = atof(optarg);
                break;
            }
            case 'o':
            {
                prob->spsa_a_scale = atof(optarg);
                break;
            }
            case 'x':
            {
                prob->spsa_c_scale = atof(optarg);
                break;
            }
            case 'v':
            {
                fprintf(stdout, "%s, please report bugs to %s\n", p_program_version, p_bug_report_contact);
                system("uname -a");
                exit(EXIT_SUCCESS);
            }
            case 'h':
            default:
            {
                probdata_print_long_usage();
                exit(1);
            }
        }
    }
    
    if (prob->role != P_CLIENT && prob->role != P_SERVER) {
        p_errno = P_ERR_SERV_CLIENT;
        return -1;
    }
    
    if (prob_src_file_flag > 0) { // disk-to-disk profiling
        if (prob->role == P_CLIENT) {
            prob->instance = probdata_client;
        } else if (prob->role == P_SERVER) {
            prob->instance = probdata_server;
        } else {
            p_errno = P_ERR_SERV_CLIENT;
            return -1;
        }
    } else if (prob_dst_file_flag > 0) { // only dst file
        p_errno = P_ERR_DSTFILE_ONLY;
        return -1;
    } else { // memory-to-memory profiling
        if (prob->role == P_CLIENT) {
            prob->instance = probdata_client;
        } else if (prob->role == P_SERVER) {
            prob->instance = probdata_server;
        } else {
            p_errno = P_ERR_SERV_CLIENT;
            return -1;
        }
    }
    return 0;
}

static int get_max_mem()
{
    int max_mem = -1;
    FILE * fp = NULL;

    fp = fopen("/proc/sys/net/core/rmem_max", "r");
    if (fp == NULL) {
        printf("file open error\n");
        return -1;
    }

    fscanf(fp, "%d", &max_mem);

    fclose(fp);

    return max_mem;
}

int probdata_get_max_buf(struct probdata* prob)
{
    int max_buffer = -1;
    max_buffer = get_max_mem();
    if (max_buffer < 0) {
        return -1;
    } else {
        prob->host_max_buf = max_buffer;
    }
 
    return 0;
}

int probdata_set_control(struct probdata* prob)
{
    P_LOG(P_HINT, "set control parameters");
    
    prob->prob_control->role = (prob->role == P_CLIENT) ? (F_CLIENT) : (F_SERVER);
    prob->prob_control->host_max_buf = prob->host_max_buf;
    memcpy(prob->prob_control->local_ip, prob->local_ip, strlen(prob->local_ip));
    prob->prob_control->local_ip[strlen(prob->local_ip)] = '\0';
    memcpy(prob->prob_control->remote_ip, prob->remote_ip, strlen(prob->remote_ip));
    prob->prob_control->remote_ip[strlen(prob->remote_ip)] = '\0';
    prob->prob_control->tcp_port = prob->tcp_port;
    prob->prob_control->udt_port = prob->udt_port;
    prob->prob_control->mss = prob->mss;
    prob->prob_control->bw_bps = prob->bandwidth;
    prob->prob_control->rtt_ms = prob->rtt_ms;
    prob->prob_control->A_val = prob->spsa_A_val;
    prob->prob_control->a_val = prob->spsa_a_val;
    prob->prob_control->a_scale = prob->spsa_a_scale;
    prob->prob_control->c_val = prob->spsa_c_val;
    prob->prob_control->c_scale = prob->spsa_c_scale;
    prob->prob_control->alpha = prob->spsa_alpha_val;
    prob->prob_control->gamma = prob->spsa_gamma_val;
    prob->prob_control->duration = prob->prof_duration;
    prob->prob_control->pgr = prob->pgr;
    prob->prob_control->impeded_limit = prob->impeded_limit;
    prob->prob_control->max_limit = prob->max_prof_limit;
    
    return 0;
}

const char p_program_version[] = 
    "ProbData version "
    P_PROGRAM_VERSION
    " (" P_PROGRAM_VERSION_DATE ")";

const char p_bug_report_contact[] =
    "<" P_BUG_REPORT_EMAIL ">";

const char p_usage_shortstr[] = 
    "\nUsage: probdata [-s|-c host] [options]\n"
    "Try `probdata --help' for more information.\n";

const char p_usage_longstr[] =
    "Usage: probdata [-s|-c host] [options]\n"
    "       probdata [-h|--help] [-v|--version]\n\n"
    "Server/Client:\n"
    "  -p, --port           # port to listen on/connect to default %d\n"
    "  -v, --version        # show version information and quit\n"
    "\n"
    "Server specific:\n"
    "  -s, --server         # run in server mode\n"
    "\n"
    "Client specific:\n"
    "  -c, --client <host>  # run in client mode, connecting to <host>\n"
    "  -t, --UDT            # use UDT only (default both UDT and %s)\n"
    "  -d, --time duration  # duration of the test by default %d seconds\n"
    "  -M, --set-mss        # set TCP/UDT maximum segment size (Bytes)\n"
    "                         MTU for UDT, (MTU-40) for TCP\n"
    "\n"
    "FastProf specific:\n"
    "  -y  --capacity       # link capacity default %.2lfGb/s\n"
    "  -T  --RTT            # RTT in milliseconds\n"
    "  -C  --ratio          # PGR default %.2lf\n"
    "  -L  --limit          # max iteration# without improvement default %d\n"
    "  -N  --max limit      # max # of the total iterations default %d\n"
    "\n"
    "ProbData specific:\n"
    "  -S  --src file       # source file (including path)\n"
    "  -V  --dst file       # destination file (including path) default %s\n"
    "  -e  --a factor       # a value default %.3lf\n"
    "  -g  --c factor       # c value default %.3lf\n"
    "  -o  --a factor scale # scaling factor for a value default %.3lf\n"
    "  -x  --c factor scale # scaling factor for c value default %.3lf\n"
    "\n"
    "ProbData version %s.\n"
    "Please report bugs to %s.\n"
    "\n";

int probdata_print_short_usage()
{
    fputs(p_usage_shortstr, stderr);
    return 0;
}

int probdata_print_long_usage()
{
    fprintf(stderr, p_usage_longstr,
            P_DEFAULT_PORT, "TCP", P_DEFAULT_DURATION, P_DEFAULT_BW,
            P_DEFAULT_PGR, P_DEFAULT_IMPEDED_LIMIT, P_DEFAULT_MAX_PROF_LIMIT,
            P_DEFAULT_DST_FILE,
            P_DEFAULT_SPSA_SMALL_A_VAL, P_DEFAULT_SPSA_SMALL_C_VAL,
            P_DEFAULT_SCALE_FACTOR_A_VAL, P_DEFAULT_SCALE_FACTOR_C_VAL,
            P_PROGRAM_VERSION, p_bug_report_contact);
    return 0;
}
