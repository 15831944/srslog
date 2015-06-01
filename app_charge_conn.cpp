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
#include "../../BizCpLib/linux/charge_pack_type.h"

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

    std::string recvstr;
    while (true)
    {
        enum {BUFFER_LEN = 512};
        char buf[BUFFER_LEN] = {'\0'};

       ssize_t nactually = 0;
       if ((ret = skt->read(buf, BUFFER_LEN, &nactually) != ERROR_SUCCESS))
       {
           srs_error("recv client ,sock error[%d]", ret);
           return ret;
       }

       recvstr.append(buf, nactually);
       int pos = recvstr.find(CRLF);
       if ( pos != std::string::npos)
       {
           bool ret = handle_json_data(recvstr.substr(0, pos));
           if (!ret)
           {
               srs_error("handle_json_data ret==false.");
               return -1;
           }
           recvstr = recvstr.erase(0, pos + 2);
       }

       //in case dos attact, when data length has longer than 1m, close the connection.
       if (recvstr.length() > 1 * 512 * 1024)
       {
           return ret;
       }
    }

    return ret;
}

bool SrsChargeConn::handle_json_data(std::string data)
{
    srs_trace("recv client: \n%s", data.c_str());
    const nx_json *js_ = NULL;
    js_ = nx_json_parse_utf8(const_cast<char *>(data.c_str()));
    if (NULL == js_)
    {
        srs_error("handle_json_data:js==NULL");
        return false;
    }

    const nx_json * js_action = nx_json_get(js_, ACTION_STR);
    if (NULL == js_action)
    {
        srs_error("handle_json_data: js_action==NULL.");
        return false;
    }

    switch (js_action->int_value)
    {
    case JOPACKTYPE_STREAMMEDIA_HEARTBEAT:
    {
        type_ = StreamMediaConn;
    }
        break;
    case JOPACKTYPE_STREAMMEDIA_FLOWDATA: //流媒体服务器流量数据包
    {

    }
        break;
    case JOPACKTYPE_CLOUD_STORE_HEARTBEAT:
    {
        type_ = CloudStore;
    }
        break;
    case JOPACKTYPE_CLOUD_STORE_FLOWDATA://云存储流量数据包
    {

    }
        break;
    case JOPACKTYPE_UNKNOWN:
    default:
    {
        break;
    }
    }

    return true;
}
