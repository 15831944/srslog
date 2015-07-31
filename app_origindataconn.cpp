#include "app_origindataconn.h"
#include "app_macros.h"

SrsOriginDataConn::SrsOriginDataConn(SrsServer *srs_server, st_netfd_t client_stfd)
    : SrsConnection(srs_server, client_stfd)
{
    type_ = SrsConnUnknown;
    skt = new SrsStSocket(client_stfd);
    dataserver = new SrsOriginDataServer(skt);
}

SrsOriginDataConn::~SrsOriginDataConn()
{
    type_ = SrsConnUnknown;

    srs_freep(skt);
    srs_freep(dataserver);
}

void SrsOriginDataConn::kbps_resample()
{

}

int64_t SrsOriginDataConn::get_send_bytes_delta()
{
    return 0;

}

int64_t SrsOriginDataConn::get_recv_bytes_delta()
{
    return 0;

}

int SrsOriginDataConn::do_cycle()
{
    int ret = ERROR_SUCCESS;

    if (NULL == dataserver) {
        ret = ERROR_POINT_NULL;
        srs_error("NULL == screenshot");
        return ret;
    }

    dataserver->set_recv_timeout(SRS_CONSTS_ORIGINDATA_RECV_TIMEOUT_US);
    dataserver->set_send_timeout(SRS_CONSTS_ORIGINDATA_SEND_TIMEOUT_US);

    enum {SIZE_TMP_BUFF = 1024};
    char recvtmp[SIZE_TMP_BUFF] = {'\0'};

    ssize_t nactually = 0;
    if ((ret = skt->read(recvtmp, SIZE_TMP_BUFF, &nactually) != ERROR_SUCCESS)) {
        srs_error("recv client ,sock error[%d]", ret);
        return ret;
    }

    std::string recv_str;
    recv_str.assign(recvtmp, nactually);
    std::string res;
    handle_client_data(recv_str, res);

    ssize_t nsize = 0;
    skt->write((char *)res.c_str(), res.length(), &nsize);

    return ret;
}

void SrsOriginDataConn::handle_client_data(std::string, std::string &)
{

}
