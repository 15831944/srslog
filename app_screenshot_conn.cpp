#include "app_screenshot_conn.h"
#include "app_config.h"
#include "app_log.h"
#include "app_error_code.h"
#include <stdlib.h>
#include "app_json.h"

SrsScreenShotConn::SrsScreenShotConn(SrsServer *srs_server, st_netfd_t client_stfd)
    : SrsConnection(srs_server, client_stfd)

{
    type_ = SrsConnUnknown;
    skt = new SrsStSocket(client_stfd);
    screenshot = new SrsScreenShotServer(skt);
    _srs_config->subscribe(this);
}

SrsScreenShotConn::~SrsScreenShotConn()
{
    type_ = SrsConnUnknown;

    _srs_config->unsubscribe(this);
    srs_freep(skt);
    srs_freep(screenshot);
}

void SrsScreenShotConn::kbps_resample()
{

}

int64_t SrsScreenShotConn::get_send_bytes_delta()
{
    return 0;
}

int64_t SrsScreenShotConn::get_recv_bytes_delta()
{
    return 0;
}

int SrsScreenShotConn::do_cycle()
{
    int ret = ERROR_SUCCESS;

    enum {HEAD_BUFFER_LEN = 17};
    char head_buffer[HEAD_BUFFER_LEN] = {'\0'};

    if (NULL == screenshot) {
        ret = ERROR_POINT_NULL;
        return ret;
    }

    screenshot->set_recv_timeout(SRS_CONSTS_ACCOUNT_RECV_TIMEOUT_US);
    screenshot->set_send_timeout(SRS_CONSTS_ACCOUNT_SEND_TIMEOUT_US);

    //recv head.
    ssize_t nsize = 0;
    if ((ret = skt->read(head_buffer, HEAD_BUFFER_LEN - 1, &nsize)) != ERROR_SUCCESS) {
        srs_error("read screenshot head timeout. ret=%d", ret);
        return ret;
    }

    int json_len = atoi(head_buffer);
    enum {JOSN_BODY_LEN = 4096};
    char json_buff[JOSN_BODY_LEN] = {'\0'};
    //recv json body.
    nsize = 0;
    if ((ret = skt->read(json_buff, json_len, &nsize)) != ERROR_SUCCESS) {
        srs_error("read screen json body timeout. ret=%d", ret);
        return ret;
    }

    do_screen_shot_job(json_buff, json_len);

    return ret;
}

void SrsScreenShotConn::do_screen_shot_job(char *json_data, int len)
{
    //parse json.
    ScreenShotData res;
    int ret = parse_json(json_data, len, res);
    if (!ret) {
        srs_error("do_screen_shot_job:parse json failed.");
        return ;
    }

}

bool SrsScreenShotConn::parse_json(char *json_data, int len, ScreenShotData &res)
{
    const nx_json* js = nx_json_parse_utf8(json_data);
    if (NULL == js){
        return false;
    }

    const nx_json* js_params = nx_json_get(js, "params");
    if (NULL == js_params) {
        return false;
    }

    const nx_json* js_item = nx_json_item(js_params, 0);
    if (NULL == js_item) {
        return false;
    }

    const nx_json* js_app = nx_json_get(js_item, "app");
    if (NULL == js_app) {
        return false;
    }

    res.app = js_app->text_value;

    const nx_json* js_stream = nx_json_get(js_item, "stream");
    if (NULL == js_stream) {
        return false;
    }

    res.stream = js_stream->text_value;

    if (NULL != js) {
        nx_json_free(js);
    }

    return true;
}
