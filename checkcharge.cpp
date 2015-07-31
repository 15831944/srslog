#include "checkcharge.h"
#include "app_macros.h"
#include "app_error_code.h"
#include "app_log.h"
#include "redis_process.h"
#include "app_json.h"
#include <sstream>
#include "charge_pack_type.h"
#include "aescry.h"
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define CHECK_CHARGE_FLOW_TIME_INTERVAL 6

CheckCharge::CheckCharge()
{
    pthread = new SrsThread(this, CHECK_CHARGE_FLOW_TIME_INTERVAL, true);
}

CheckCharge::~CheckCharge()
{
    srs_freep(pthread);
}

int CheckCharge::start()
{
    int ret = ERROR_SUCCESS;

    if ((ret = pthread->start()) != ERROR_SUCCESS) {
        srs_error("CheckCharge: st_thread_create failed. ret=%d", ret);
        return ret;
    }

    srs_trace("CheckCharge thread cid=%d, current_cid=%d", pthread->cid(), _srs_context->get_id());

    return ret;
}

void CheckCharge::stop()
{
    pthread->stop();
}

int CheckCharge::cycle()
{
    int ret = ERROR_SUCCESS;

    if (!g_aes.SetKey()) {
        return -1;
    }

    while (true) {
        std::string jsondata;
        get_users_need_upload(jsondata);
        if (jsondata.length() > 0) {
            //encry data.
            while (jsondata.length() % 16 != 0) {
                jsondata += '0';
            }

            int len = jsondata.length();
            char *pbuf = new char[len];
            if (NULL == pbuf) {
                st_sleep(60);
                continue;
            }

            g_aes.Encode((uint8_t*)pbuf, (uint8_t*)jsondata.c_str(), len);

            //send to alarm.
            int fd = -1;
            int ret = jcc_connect_server(g_alarm_ip, atoi(g_alarm_port.c_str()), fd);
            if (ret != JCC_SUCCESS) {

                st_sleep(60);
                continue;
            }

            send(fd, pbuf, len, 0);
        }
        st_sleep(60);
    }

    return ret;
}

void CheckCharge::get_users_need_upload(std::string &res)
{
    //get all user info.
    RedisProcess *ob = RedisProcess::get_instance();
    if (NULL == ob) {
        srs_error("get_users_info_upload: get ReidsProcess NULL.");
        return ;
    }

    std::vector<std::string> usersinfo;
    std::map<std::string, T_UserDataInfo> all_users_data;
    RedisProcess::mu_.Lock();
    ob->get_all_keys(usersinfo);

    for (std::vector<std::string>::iterator iter = usersinfo.begin();
         iter != usersinfo.end();
         ++iter) {
        T_UserDataInfo data;
        get_single_user_info(*iter, data);
        all_users_data.insert(std::make_pair(*iter, data));
    }
    RedisProcess::mu_.UnLock();

    //judge if need to upload.
    tell_me_result(all_users_data, res);

    //encry data.

}

void CheckCharge::get_single_user_info(const std::string &user, T_UserDataInfo &data)
{
    RedisProcess *ob = RedisProcess::get_instance();
    if (NULL == ob) {
        srs_error("get_single_user_info: get ReidsProcess NULL.");
        return ;
    }

    ob->get_single_userinfo(user, data);
}

void CheckCharge::tell_me_result(const std::map<std::string, T_UserDataInfo> &data,
                                 std::string &res)
{
    std::stringstream ss;
    ss << __SRS_JOBJECT_START
       << __SRS_JFIELD_STR(JS_CHARGE_TYPE, __SRS_TYPE_MONEYWARN) << __SRS_JFIELD_CONT
       << "\"users\":"
       << __SRS_JARRAY_START;

    int nums = 0;
    for (std::map<std::string, T_UserDataInfo>::const_iterator iter = data.begin();
         iter != data.end();
         ++iter) {
         if (atof(iter->second.money_left_.c_str()) < 5.00) {
            ss << __SRS_JOBJECT_START <<
                  __SRS_JFIELD_STR("name", iter->first)
               << __SRS_JOBJECT_END
               << __SRS_JFIELD_CONT;
            ++nums;
         }
    }

    if (0 == nums) {
        return;
    }

    std::string temp_str;
    temp_str = ss.str();
    int npos = temp_str.find_last_of(",");
    temp_str = temp_str.substr(0, npos);
    temp_str += __SRS_JARRAY_END;
    temp_str += __SRS_JOBJECT_END;

    res = temp_str;

}

void CheckCharge::on_thread_stop()
{

}

int jcc_connect_server(std::string ip, int port, int &fd_socket)
{
    if (ip.length() <= 0 || port <0 || port > 65535) {
        return JCC_NOT_SET_SERVER;
    }


    fd_socket = -1;

    struct sockaddr_in client_addr;
    bzero(&client_addr, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htons(INADDR_ANY);
    client_addr.sin_port = htons(0);
    fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_socket < 0)
    {
        return JCC_CONNECT_SERVER_ERROR;
    }

    if (bind(fd_socket, (struct sockaddr*) &client_addr,
             sizeof(client_addr)))
    {
        return JCC_CONNECT_SERVER_ERROR;
    }

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if (inet_aton(ip.c_str(), &server_addr.sin_addr) == 0)
    {
        return JCC_CONNECT_SERVER_ERROR;
    }

    server_addr.sin_port = htons(port);
    socklen_t server_addr_length = sizeof(server_addr);
    if (connect(fd_socket, (struct sockaddr*) &server_addr,  server_addr_length) < 0)
    {
        return JCC_CONNECT_SERVER_ERROR;
    }


    return JCC_SUCCESS;
}
