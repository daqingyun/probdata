/***************************************************************************
 * ProbData - PRofiling Optimization Based DAta Transfer Advisor
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _PROBDATA_ERROR_H_
#define _PROBDATA_ERROR_H_

// #include "../tpg/tpg_common.h" // for tpg error handler
#include <tpg_common.h>
#include "../fastprof/fastprof_error.h"

extern int p_errno;

enum
{
    P_ERR_NONE = 0,
    P_ERR_UNKNOWN = 1024,
    P_ERR_DSTFILE_ONLY = 101,
    P_ERR_DISABLED_PROBDATA = 102,
    P_ERR_FILE = 103,
    P_ERR_SERV_CLIENT = 104,
    P_ERR_SEND_MESSAGE = 105,
    P_ERR_THREAD = 106,
    P_ERR_IPERF3 = 107,
    P_ERR_FASTPROF = 108,
    P_ERR_SELECT = 190,
    P_ERR_TPG = 120,
    P_ERR_CTRL_CHAN = 121,
};

const char *probdata_errstr(int);

#endif