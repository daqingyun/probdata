/***************************************************************************
 * ProbData - PRofiling Optimization Based DAta Transfer Advisor
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sys/time.h>
#include <cmath>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include "probdata.h"
#include "probdata_control.h"
#include "probdata_log.h"
#include "probdata_error.h"
#include "probdata_client.h"
#include "probdata_server.h"

int main(int argc, char** argv)
{
    struct probdata* prob;
    prob = new struct probdata;

    /* wall clock time */
    struct timeval start, end;
    long long int time_elapsed_sec = 0;
    long long int hours = 0, minutes = 0, seconds = 0;

    gettimeofday(&start, NULL);
    
    if (probdata_new_and_default(prob) < 0) {
        P_LOG(P_ERR, "cannot set default setting for probdata");
        exit(1);
    }
    
    if (probdata_load_conf(prob) < 0) {
        P_LOG(P_ERR, "cannot load configuration file");
    }
    
    if (probdata_get_max_buf(prob) < 0) {
        P_LOG(P_HINT, "cannot get max buffer: %d", prob->host_max_buf);

        // don't exit but let lower layer to check
        ;
    }
    if (probdata_cmd_parse(prob, argc, argv) < 0) {
        P_LOG(P_PERF, "parameter parsing error: %s", probdata_errstr(p_errno));
        probdata_print_long_usage();
        exit(1);
    }
    if (probdata_set_control(prob) < 0)	{
        exit(1);
    }
    
    /* run client or server */
    if (prob->instance(prob) < 0) {
        P_LOG(P_PERF, "%s", probdata_errstr(p_errno));
        return -1;
    }
    
    gettimeofday(&end, NULL);
    time_elapsed_sec = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) / 1000000;
    hours = (long long int) (time_elapsed_sec / 3600);
    minutes = (long long int)((time_elapsed_sec - hours * 3600) / 60);
    seconds = time_elapsed_sec - hours * 3600 - minutes * 60;
    P_LOG(P_PERF, "Time elapsed: %lld hours %lld minutes %lld seconds\n", hours, minutes, seconds);
    
    return 0;
}

