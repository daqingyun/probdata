/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include <sys/mman.h>
#include "tpg_log.h"
#include "tpg_stream.h"

int tpg_stream_init(tpg_stream* pst)
{
    if (pst == NULL) {
        t_errno = T_ERR_STREAM_INIT;
        return -1;
    }
    pst->stream_to_profile = NULL;
    pst->stream_id = -1;
    pst->stream_sockfd = -1;
    memset(pst->stream_src_ip, '\0', T_HOSTADDR_LEN);
    pst->stream_src_port = -1;
    memset(pst->stream_dst_ip, '\0', T_HOSTADDR_LEN);
    pst->stream_dst_port = -1;
    pst->stream_blkbuf = NULL;
    pst->stream_blkbuf_fd = -1;
    pst->stream_blksize = -1;
    
    pst->stream_result.start_time_usec = 0;
    pst->stream_result.interval_time_usec = 0;
    pst->stream_result.end_time_usec = 0;
    pst->stream_result.total_sent_bytes = 0;
    pst->stream_result.interval_sent_bytes = 0;
    pst->stream_result.total_recv_bytes = 0;
    pst->stream_result.interval_recv_bytes = 0;
    
    pst->stream_send = NULL;
    pst->stream_recv = NULL;
    return 0;
}

int tpg_stream_free(tpg_stream* pst)
{
    if (pst != NULL) {
        t_errno = T_ERR_STREAM_INIT;
        return -1;
    }
    pst->stream_to_profile = NULL;
    pst->stream_id = -1;
    pst->stream_sockfd = -1;
    memset(pst->stream_src_ip, '\0', T_HOSTADDR_LEN);
    memset(pst->stream_dst_ip, '\0', T_HOSTADDR_LEN);
    
    T_LOG(T_HINT, "Delete and munmap() a stream...");
    munmap(pst->stream_blkbuf, pst->stream_blksize);
    T_LOG(T_HINT, "Stream is munmapped");
    T_LOG(T_HINT, "Close stream block buffer fd...");
    close(pst->stream_blkbuf_fd);
    T_LOG(T_HINT, "Stream block buffer fd is closed");
    pst->stream_blkbuf = NULL;
    
    pst->stream_blkbuf_fd = -1;
    pst->stream_blksize = -1;
    
    pst->stream_result.start_time_usec = 0;
    pst->stream_result.interval_time_usec = 0;
    pst->stream_result.end_time_usec = 0;
    pst->stream_result.total_sent_bytes = 0;
    pst->stream_result.interval_sent_bytes = 0;
    pst->stream_result.total_recv_bytes = 0;
    pst->stream_result.interval_recv_bytes = 0;
    
    pst->stream_send = NULL;
    pst->stream_recv = NULL;
    
    if (pst != NULL) {
        T_LOG(T_HINT, "Stream is not empty deleting...");
        delete pst;
        pst = NULL;
        T_LOG(T_HINT, "Stream is deleted");
    }
    return 0;
}

