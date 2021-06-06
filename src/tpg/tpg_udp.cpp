/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <cerrno>
#include "tpg_udp.h"

int udp_profile(tpg_profile* prof)
{
    return 0;
}

int udp_init(tpg_profile *prof)
{
    return 0;
}

int udp_listen(tpg_profile *prof)
{
    return 0;
}

int udp_accept(tpg_profile *prof, int ms_timeout)
{
    return 0;
}

int udp_connect(tpg_stream *prof)
{
    return 0;
}

int udp_send(tpg_stream *pstream)
{
    return 0;
}

int udp_recv(tpg_stream *pstream)
{
    return 0;
}

int udp_close(int* sockfd)
{
    return 0;
}

const char* udp_strerror()
{
    return (strerror(errno));
}