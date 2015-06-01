#include "app_charge_conn.h"
#include "app_config.h"
#include "app_log.h"
#include "app_error_code.h"
#include <stdio.h>
#include <stdlib.h>
#include "app_json.h"
#include <dirent.h>
#include "execv_ffmpeg.h"
#include <sstream>
#include "jpg2base64.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>


SrsChargeConn::SrsChargeConn(SrsServer *srs_server, st_netfd_t client_stfd)
    : SrsConnection(srs_server, client_stfd)

{
    type_ = SrsConnUnknown;
    skt = new SrsStSocket(client_stfd);
    proxy = new SrsChargeServer(skt);
    _srs_config->subscribe(this);
}

SrsChargeConn::~SrsChargeConn()
{
    type_ = SrsConnUnknown;

    _srs_config->unsubscribe(this);
    srs_freep(skt);
    srs_freep(proxy);
}

void SrsChargeConn::kbps_resample()
{

}

int64_t SrsChargeConn::get_send_bytes_delta()
{
    return 0;
}

int64_t SrsChargeConn::get_recv_bytes_delta()
{
    return 0;
}

int SrsChargeConn::do_cycle()
{
    int ret = ERROR_SUCCESS;

    if (NULL == proxy) {
        ret = ERROR_POINT_NULL;
        srs_error("NULL == screenshot");
        return ret;
    }

    proxy->set_recv_timeout(SRS_CONSTS_ACCOUNT_RECV_TIMEOUT_US);
    proxy->set_send_timeout(SRS_CONSTS_ACCOUNT_SEND_TIMEOUT_US);

    //begin recv data and handle them.
    enum {HEAD_BUFFER_LEN = 512};
    char head_buffer[HEAD_BUFFER_LEN] = {'\0'};

    //recv head and type.
    ssize_t nsize = 0;
    if ((ret = skt->read(head_buffer, 5, &nsize)) != ERROR_SUCCESS) {
        srs_error("read screenshot head timeout. ret=%d", ret);
        return ret;
    }

    srs_trace("recv client[%s]", head_buffer);

    return ret;
}
