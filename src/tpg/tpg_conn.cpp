/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include "tpg_conn.h"

int tpg_connection_init(tpg_nic2nic* pconn)
{
    if (pconn == NULL) {
        t_errno = T_ERR_PHY_CONN_INIT;
        return -1;
    }
    pconn->id = -1;
    memset(pconn->src_ip, '\0', T_HOSTADDR_LEN);
    memset(pconn->dst_ip, '\0', T_HOSTADDR_LEN);
    return 0;
}

int tpg_connection_alloc(tpg_nic2nic* pconn)
{
    /* do nothing */
    return 0;
}

int tpg_connection_free(tpg_nic2nic* pconn)
{
    if (pconn != NULL) {
        t_errno = T_ERR_PHY_CONN_INIT;
        return -1;
    }
    pconn->id = -1;
    memset(pconn->src_ip, '\0', T_HOSTADDR_LEN);
    memset(pconn->dst_ip, '\0', T_HOSTADDR_LEN);
    return 0;
}
