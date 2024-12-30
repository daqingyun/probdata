/***************************************************************************
 * FastProf: A Stochastic Approximation-based Transport Profiler
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cmath>
#include <stdarg.h>
#include <getopt.h>
#include "fastprof_error.h"

int f_errno;

const char* f_errstr(int err_no)
{
    // static char errstr[ERROR_MSG_LEN];
    static std::string errstr("");
    // char intstr[32];
    // int perr = 0;
    
    switch (err_no)
    {
	case F_ERR_NONE:
        {
            errstr = "No error";
            break;
        }
	case F_ERR_UNKNOWN:
        {
            errstr = "Unknown error";
            break;
        }
	case F_ERR_ERROR:
        {
            errstr = "FastProf error";
            break;
        }
	case F_ERR_LOCK:
        {
            errstr = "mutex lock error";
            break;
        }
	case F_ERR_FILE:
        {
            errstr = "cannot open log file";
            break;
        }
    }
    errstr += ": ";
    if (t_errno) { // probdata error + tpg error
        errstr += t_errstr(t_errno);
    }
    if (i_errno) {
        errstr += iperf_strerror(i_errno);
    }
    
    return (errstr.c_str());
}