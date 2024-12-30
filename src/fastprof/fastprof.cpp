/***************************************************************************
 * FastProf: A Stochastic Approximation-based Transport Profiler
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <iostream>
#include "fastprof.h"

int f_ctrl_default(struct fastprof_ctrl * ctrl)
{
    ctrl->role = F_UNKNOWN;
    memset(ctrl->local_ip, 0,  F_HOSTADDR_LEN);
    memset(ctrl->remote_ip, 0, F_HOSTADDR_LEN);
    ctrl->tcp_port = -1;
    ctrl->udt_port = -1;
    ctrl->bw_bps = -1.0;
    ctrl->rtt_ms = -1.0;
    ctrl->mss = -1;
    ctrl->A_val = -1.0;
    ctrl->a_val = -1.0;
    ctrl->a_scale = 1.0;
    ctrl->c_val = -1.0;
    ctrl->c_scale = 1.0;
    ctrl->alpha = -1.0;
    ctrl->gamma = -1.0;
    ctrl->pgr = -1.0;
    ctrl->impeded_limit = -1;
    ctrl->max_limit = -1;
    ctrl->duration = F_DEFAULT_DURATION;
    return 0;
}

int f_ctrl_set_addr(struct fastprof_ctrl* ctrl, char* addr)
{
    memcpy(ctrl->remote_ip, addr, strlen(addr));
    ctrl->remote_ip[strlen(addr)] = '\0';
    return 0;
}

double f_perturb()
{
    return ( ( (double)rand() / (double)RAND_MAX ) > 0.5 ? (1.0) : (-1.0) );
}

double f_round(double x)
{
    return (floor( ((double)(x)) + 0.5 ));
}

double f_rand(int x, int y)
{
    return (double)( (rand() % (y - x + 1)) + x );
}

int f_get_currtime_str(char* tm_str)
{
    time_t t;
    struct tm currtime;

    t = time(NULL);
    currtime = *localtime(&t);
    

    sprintf(tm_str, "%02d:%02d:%d:%02d:%02d:%02d", 
        currtime.tm_mon + 1, currtime.tm_mday, currtime.tm_year + 1900,
        currtime.tm_hour, currtime.tm_min, currtime.tm_sec);
    
    return 0;
}

int f_m2m_db_search(FILE* f, struct mem_record* record_max, struct fastprof_ctrl* ctrl)
{
    struct stat st = {0};
    int ret = 0;
    char filename[256];
    char readline[512];
    struct mem_record record_tmp;

    if (stat(F_DEFAULT_M2M_FOLDER, &st) == -1) {
        mkdir(F_DEFAULT_M2M_FOLDER, 0700);
        F_LOG(F_HINT, "profile folder <%s> created", F_DEFAULT_M2M_FOLDER);
        return -1;
    } else {
        F_LOG(F_HINT, "profile folder <%s> exists", F_DEFAULT_M2M_FOLDER);
    }
    
	if (!f) {
		sprintf(filename, "%s/%s_%s.PROF", F_DEFAULT_M2M_FOLDER, ctrl->local_ip, ctrl->remote_ip);
        F_LOG(F_PERF, "    FILE: %s", filename);
		f = fopen(filename, "a+");
		if (f == NULL) {
			printf("file open error\n");
			return -1;
		}
	}
    
    record_max->average_perf = -1.0;
    
    while (!feof(f)) {
        memset(readline, '\0', 512);
        // memset(&record_tmp, '\0', sizeof(struct mem_record));
        fgets(readline, 512, f);
        if (readline[0] == '\n' || readline[0] == '\t' ||
			readline[0] == ' '  || readline[0] == '\0' ||
			readline[0] == '#') {
            continue;
        }
        ret = sscanf(readline, MEM_RECORD_FORMAT_LABEL,
            (record_tmp.src_ip),
			(record_tmp.dst_ip),
			(record_tmp.curr_time),
			(record_tmp.protocol_name),
			(record_tmp.toolkit_name),
			&(record_tmp.num_streams),
			&(record_tmp.mss_size),
			&(record_tmp.buffer_size),
			(record_tmp.buffer_size_unit),
			&(record_tmp.block_size),
			(record_tmp.block_size_unit),
			&(record_tmp.duration),
			(record_tmp.duration_unit),
			&(record_tmp.trans_size),
			(record_tmp.trans_size_unit),
			&(record_tmp.average_perf),
			(record_tmp.average_perf_unit));
        if (ret != MEM_RECORD_NUM_ELEMENTS) {
            F_LOG(F_HINT, "parsing error - %d elements are captured", ret);
            return -1;
        }
        if (record_tmp.average_perf > record_max->average_perf) {
            memcpy(record_max, &record_tmp, sizeof(struct mem_record));
        }
    }

    if (record_max->average_perf < 0) {
        return -1;
    }
    
    fclose(f);
    return 0;
}

int f_print_m2m_record(struct mem_record *record)
{
	F_LOG(F_PERF,
        "    SrcIP:        %s\n"
        "    DstIP:        %s\n"
		"    Time:         %s\n"
		"    Protocol:     %s\n"
		"    Toolkit:      %s\n"
        "    Stream#:      %d\n"
        "    MSS(PktSize): %d\n"
		"    BufferSize:   %d%s\n"
		"    BlockSize:    %d%s\n"
		"    Duration:     %d%s\n"
		"    TransferSize: %lld%s\n"
		"    AvgPerf:      %lf%s",
		record->src_ip,
		record->dst_ip,
		record->curr_time,
		record->protocol_name,
		record->toolkit_name,
		record->num_streams,
		record->mss_size,
		record->buffer_size,
		record->buffer_size_unit,
		record->block_size,
		record->block_size_unit,
		record->duration,
		record->duration_unit,
		record->trans_size,
		record->trans_size_unit,
		record->average_perf,
		record->average_perf_unit);
	return 0;
}

double f_estimate_rtt(char* hostaddr)
{
    unsigned int ret = -1;
    double min_rtt_ms = -1.0;
    double avg_rtt_ms = -1.0;
    double max_rtt_ms = -1.0;
    double mdev_rtt_ms = -1.0;
    char format_cmd[128];
    std::string output_str;
    FILE * stream;
    const int max_buffer = 512;
    char buffer[max_buffer];
    sprintf(format_cmd, "/bin/ping -w 6 -c 5 %s", hostaddr);
    std::string cmd(format_cmd);
    cmd.append(" 2>&1");

    stream = popen(cmd.c_str(), "r");

    if (stream) {
        while (!feof(stream)) {
            if (fgets(buffer, max_buffer, stream) != NULL) {
                output_str.append(buffer);
            }
        }
        pclose(stream);
    } else {
        return -1.0;
    }

    // now parsing the output
    if ((unsigned int)(ret = output_str.find(std::string("mdev"), 0)) == std::string::npos) {
        return -1;
    } else {
        // printf("%s\n", output_str.c_str());
        // printf("%s\n", output_str.substr(ret, std::string::npos).c_str());
        // mdev = 0.030/0.043/0.066/0.014 ms
        sscanf(output_str.substr(ret, std::string::npos).c_str(), "mdev = %lf/%lf/%lf/%lf ms",
            &min_rtt_ms, &avg_rtt_ms, &max_rtt_ms, &mdev_rtt_ms);
    }
    
    return avg_rtt_ms;
}
