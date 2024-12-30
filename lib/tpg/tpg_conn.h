/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#ifndef _TPG_CONN_H_
#define _TPG_CONN_H_

#include <cstdio>
#include <cstdlib>

#include "tpg_common.h"
#include "tpg_profile.h"

struct tpg_nic2nic 
{
    int id;
    char src_ip[T_HOSTADDR_LEN];
    char dst_ip[T_HOSTADDR_LEN];
};

int tpg_connection_init(tpg_nic2nic* pconn);
int tpg_connection_alloc(tpg_nic2nic* pconn);
int tpg_connection_free(tpg_nic2nic* pconn);

#endif
