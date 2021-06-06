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
#include "probdata_error.h"

int p_errno;

const char* probdata_errstr(int err_no)
{
    // static char errstr[ERROR_MSG_LEN];
    static std::string errstr("");
    // char intstr[32];
    // int perr = 0;
    
    switch (err_no)
    {
	case P_ERR_NONE:
        {
            errstr = "No error";
            break;
        }
	case P_ERR_DSTFILE_ONLY:
        {
            errstr = "A source file is required";
            break;
        }
	case P_ERR_DISABLED_PROBDATA:
        {
            errstr = "Do not specify any file names";
            break;
        }
	case P_ERR_FILE:
        {
            errstr = "file-related error (most likely cannot create)";
            break;
        }
	case P_ERR_SERV_CLIENT:
        {
            errstr = "cannot be both client and server or none of them";
            break;
        }
	case P_ERR_SEND_MESSAGE:
        {
            errstr = "control channel cannot send message";
            break;
        }
	case P_ERR_THREAD:
        {
            errstr = "thread-related errors";
            break;
        }
	case P_ERR_IPERF3:
        {
            errstr = "iperf3-related errors";
            break;
        }
	case P_ERR_FASTPROF:
        {
            errstr = "fastprof-related errors";
            break;
        }
	case P_ERR_SELECT:
        {
            errstr = "control channel select() error";
            break;
        }
	case P_ERR_TPG:
        {
            errstr = "tpg-related errors";
            break;
        }
        case P_ERR_CTRL_CHAN:
        {
            errstr = "control channel-related error";
            break;
        }
    }
    
    errstr += ": ";
    
    if (t_errno) { // += tpg error
        errstr += t_errstr(t_errno);
    }
    
    if (f_errno) { // += fastprof error
        errstr += f_errstr(f_errno);
    }
    
    return (errstr.c_str());
}