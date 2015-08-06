#include "app_base_conn.h"
#include "app_log.h"
#include "app_error_code.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

std::string g_alarm_ip;
std::string g_alarm_port;

//  20/1T. 1byte price.
static const long double price_flow_per_byte = 0.00000000001818989403545856475830078125;
static const long double test_price_flow_per_byte = 0.00000001818989403545856475830078125;

SrsBaseConn::SrsBaseConn(SrsServer *srs_server, st_netfd_t client_stfd)
    : SrsConnection(srs_server, client_stfd)

{
    skt = new SrsStSocket(client_stfd);
    proxy = new SrsBaseServer(skt);
}

SrsBaseConn::~SrsBaseConn()
{
    srs_freep(skt);
    srs_freep(proxy);
}

void SrsBaseConn::kbps_resample()
{

}

int64_t SrsBaseConn::get_send_bytes_delta()
{
    return 0;
}

int64_t SrsBaseConn::get_recv_bytes_delta()
{
    return 0;
}

int SrsBaseConn::do_cycle()
{
    int ret = ERROR_SUCCESS;

    if (NULL == proxy) {
        ret = ERROR_POINT_NULL;
        return ret;
    }

    proxy->set_recv_timeout(SRS_CONSTS_ACCOUNT_RECV_TIMEOUT_US);
    proxy->set_send_timeout(SRS_CONSTS_ACCOUNT_SEND_TIMEOUT_US);

//    int sessionidint = rand();
//    std::stringstream ss;
//    ss << sessionidint;
//    sessionid_ = ss.str();

//    //send sessionid firt.
//    ssize_t nsize = 0;
//    skt->write((char *)sessionid_.c_str(), sessionid_.length(), &nsize);

//    enum {SIZE_TMP_BUFF = 1024};
//    char recvtmp[SIZE_TMP_BUFF] = {'\0'};

//    ssize_t nactually = 0;
//    if ((ret = skt->read(recvtmp, SIZE_TMP_BUFF, &nactually) != ERROR_SUCCESS)) {
//        srs_error("recv client ,sock error[%d]", ret);
//        return ret;
//    }

//    std::string recv_str;
//    recv_str.assign(recvtmp, nactually);
//    std::string res;
//    handle_client_data(recv_str, res);

//    nsize = 0;
//    skt->write((char *)res.c_str(), res.length(), &nsize);

    return ret;
}
