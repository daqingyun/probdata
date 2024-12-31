/***************************************************************************
 * FastProf: A Stochastic Approximation-based Transport Profiler
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _FASTPROF_ERROR_
#define _FASTPROF_ERROR_

#include <sysexits.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C"{
#endif
    #include <iperf_api.h>
#ifdef __cplusplus
}
#endif

#include <tpg.h>

enum
{
    F_ERR_NONE = 0,
    F_ERR_UNKNOWN = 1024,
    F_ERR_ERROR = 108,
    F_ERR_LOCK = 109,
    F_ERR_FILE = 110,
    F_ERR_IPERF3 = 111,
    F_ERR_TPG = 112,
};


const char *f_errstr(int);
extern int  f_errno;

#endif