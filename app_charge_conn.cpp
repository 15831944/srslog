#include "app_charge_conn.h"
#include "app_config.h"
#include "app_log.h"
#include "app_error_code.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include "execv_ffmpeg.h"
#include <sstream>
#include "jpg2base64.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include "redis_process.h"
#include "redis_dbnum.h"

//  20/1T. 1byte price.
static const long double price_flow_per_byte = 0.00000000001818989403545856475830078125;
static const long double test_price_flow_per_byte = 0.00000001818989403545856475830078125;

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

    enum {SIZE_TMP_BUFF = 512};
    char recvtmp[SIZE_TMP_BUFF] = {'\0'};
    std::string recvstr;
    while (true) {
        bzero(recvtmp, SIZE_TMP_BUFF);
        ssize_t nactually = 0;
        if ((ret = skt->read(recvtmp, SIZE_TMP_BUFF, &nactually) != ERROR_SUCCESS))
        {
            srs_error("recv client ,sock error[%d]", ret);
            return ret;
        }

        recvstr.append(recvtmp, nactually);
        int npos = recvstr.find(JSON_CRLF);
        if (npos != std::string::npos) {
            handle_json_data(recvstr.substr(0, npos));
            recvstr.erase(0, npos + 2);
        }
    }

    return ret;
}

bool SrsChargeConn::handle_json_data(const std::string &data)
{
    srs_trace("recv client:\n %s", data.c_str());

    const nx_json *js_ = NULL;
    js_ = nx_json_parse_utf8(const_cast<char *>(data.c_str()));
    if (NULL == js_)
    {
        srs_error("handle_json_data:js==NULL");
        return false;
    }

    const nx_json * js_action = nx_json_get(js_, JSON_ACTION_STR);
    if (NULL == js_action)
    {
        srs_error("handle_json_data: js_action==NULL.");
        return false;
    }

    int type = JOPACKTYPE_UNKNOWN;
    if (NULL != js_action->text_value)
    {
        type = atoi(js_action->text_value);
    }

    switch (type)
    {
    case JOPACKTYPE_TOP_UP:
    {
        T_ClientTopUpData res;
        get_topup_info(res, js_);        
        handle_topup_data(res);
    }
        break;
    case JOPACKTYPE_STREAMMEDIA_HEARTBEAT:
    {
        type_ = StreamMediaConn;
        handle_heart_beat();
    }
        break;
    case JOPACKTYPE_STREAMMEDIA_FLOWDATA:
    {
        std::vector<T_ClientFlowData> res;
        get_flow_info(res, js_);
        if (res.size() > 0)
        {
            srs_trace("vec size [%d]", res.size());
            handle_flow_data(res);
        }
    }
        break;
    case JOPACKTYPE_CLOUD_STORE_HEARTBEAT:
    {
        type_ = CloudStore;
    }
        break;
    case JOPACKTYPE_CLOUD_STORE_FLOWDATA://云存储流量数据包
    {
        //todo.yangkai.
    }
        break;
    case JOPACKTYPE_UNKNOWN:
    default:
    {
        break;
    }
    }

    if (NULL != js_)
    {
        nx_json_free(js_);
    }

    return true;
}

void SrsChargeConn::get_flow_info(std::vector<T_ClientFlowData> &res, const nx_json *js_)
{
    if (NULL == js_)
    {
        return;
    }

    const nx_json * js_params = nx_json_get(js_, JSON_PARAMS_STR);
    if (NULL == js_params)
    {
        srs_error("get_flow_info: NULL == js_params");
        return;
    }

    int len = js_params->length;
    for (int i = 0; i < len; ++i) {
        const nx_json *js_local = nx_json_item(js_params, i);
        if (NULL == js_local)
        {
            srs_error("get_flow_info: NULL == js_local, i==%d", i);
            continue;
        }

        T_ClientFlowData local;
        const nx_json *js_app = nx_json_get(js_local, JSON_APP_STR);
        if (NULL != js_app)
        {
            local.app = js_app->text_value;
        }

        const nx_json *js_stream = nx_json_get(js_local, JSON_STREAM_STR);
        if (NULL != js_stream)
        {
            local.stream = js_stream->text_value;
        }

        const nx_json* js_mode = nx_json_get(js_local, JSON_MODE_STR);
        if (NULL != js_mode)
        {
            local.mode = (e_JOSTREAMMEDIA)(atoi(js_mode->text_value));
        }

        const nx_json* js_flow = nx_json_get(js_local, JSON_FLOW_STR);
        if (NULL != js_flow)
        {
            if (NULL != js_flow->text_value)
            {
                local.flow_data = atoi(js_flow->text_value);
            }
        }

        const nx_json* js_usr = nx_json_get(js_local, JSON_USRDATA_STR);
        if (NULL != js_usr)
        {
            local.usr_data = js_usr->text_value;
        }

        res.push_back(local);
    }
}

void SrsChargeConn::handle_flow_data(const std::vector<T_ClientFlowData> &res)
{
    RedisProcess* nosql = RedisProcess::get_instance();
    if (NULL == nosql)
    {
        return;
    }

//    MySqlProcess *sql = MySqlProcess::get_instance();
//    if (NULL == sql) {
//        return;
//    }

    double price = 0;
    for (int i = 0; i < res.size(); ++i)
    {
        //get usr info
        t_out_AntiStrealLinkTokenData outdata;
        get_usr_info_token(outdata, res[i].usr_data);

        //decide the price
        switch(outdata.type_)
        {
        case ShareWebsiteAntiStrealLinkToken:
        {
            //            price = price_flow_per_byte;
            price = test_price_flow_per_byte; //for test. todo.
        }
            break;
        default:
        {
            continue;
        }
        }

        //usr play mode.
        if (JOSTREAMMEDIA_FLOWDATA_LINKMODE_PLAY == res[i].mode) {
            //judge if need to charge
            std::stringstream cmd_cancharge;
            cmd_cancharge << "get " << outdata.usr_name_ << ":" << outdata.devguid_ << ":" << outdata.channel_;
            std::string ifcharge;
            nosql->get(ifcharge, DB_USRDEVCHANNEL_SHAREWEBSITEFLAG, cmd_cancharge.str().c_str());
            if (0 == atoi(ifcharge.c_str())) {
                continue;//not open this job, not charge it.
            }

            //charge process
            double this_time_used_money = price * res[i].flow_data;

            double money = 0;
            add_using_money_in_memory(money, outdata.usr_name_, this_time_used_money);

            //calculate the money, if more than 0.01 then update redis, else store it first
            if (money >= 0.01)
            {
                //get data from redis.
                double flow_redis_old = 0;
                get_flow_redis_old(flow_redis_old, nosql, outdata.usr_name_);

                //close the mysql share_website flag.
                if (flow_redis_old <= 0) {
                    tb_user_device_vas tb;
                    tb.share_website_flag = 0;
                    tb.usr_name = outdata.usr_name_;
                    tb.device_guid = outdata.devguid_;
                    tb.channel_no = (outdata.channel_);
                    {
//                        sql->lock_db();
//                        sql->update_tb_user_device_vas("DeviceSys", tb);
//                        sql->unlock_db();
                    }
                    continue;
                }

                set_flow_redis_new(0.01, nosql, outdata.usr_name_);

                subtract_using_money_in_memory(outdata.usr_name_);
            }

            //update mysql database when used more than 0.1
            if (money >= 0.1) {
//                sql->lock_db();
//                std::string moneyleft_str;
//                sql->select_tb_usr_charge_left_single("DeviceSys", outdata.usr_name_, moneyleft_str);
//                double moneynew = atof(moneyleft_str.c_str()) - 0.1;
//                sql->update_tb_usr_charge_left("DeviceSys", outdata.usr_name_, moneynew);
//                sql->unlock_db();
            }

        } else if (JOSTREAMMEDIA_FLOWDATA_LINKMODE_PUSH == res[i].mode) {
            //get all usrs
            std::vector<std::string> users;
            std::stringstream cmd;
            cmd << "SMEMBERS " << outdata.devguid_ << ":" << outdata.channel_;
            nosql->get_smembers(users, DB_SETS_USRDEVCHANNEL_USERS, cmd.str().c_str());
            if (users.size() <= 0) {
                continue;
            }

            for (int i = 0; i < users.size(); ++i) {

                //charge process
                double this_time_used_money = price * res[i].flow_data;

                double money = 0;
                add_using_money_in_memory(money, users[i], this_time_used_money);

                //calculate the money, if more than 0.01 then update redis, else store it first
                if (money >= 0.01)
                {
                    //get data from redis.
                    double flow_redis_old = 0;
                    get_flow_redis_old(flow_redis_old, nosql, users[i]);

                    //close the mysql share_website flag.
                    if (flow_redis_old <= 0) {
                        tb_user_device_vas tb;
                        tb.share_website_flag = 0;
                        tb.usr_name = users[i];
                        tb.device_guid = outdata.devguid_;
                        tb.channel_no = (outdata.channel_);
                        {
//                            sql->lock_db();
//                            sql->update_tb_user_device_vas("DeviceSys", tb);
//                            sql->unlock_db();
                        }
                    }

                    set_flow_redis_new(0.01, nosql, users[i]);

                    subtract_using_money_in_memory(users[i]);
                }

                //update mysql database when used more than 0.1
                if (money >= 0.1) {
//                    sql->lock_db();
//                    std::string moneyleft_str;
//                    sql->select_tb_usr_charge_left_single("DeviceSys", users[i], moneyleft_str);
//                    double moneynew = atof(moneyleft_str.c_str()) - 0.1;
//                    sql->update_tb_usr_charge_left("DeviceSys", users[i], moneynew);
//                    sql->unlock_db();
                }

            }
        }
    }
}

void SrsChargeConn::handle_heart_beat()
{
    std::stringstream res;
    res << __SRS_JOBJECT_START
        << __SRS_JFIELD_STR(JSON_ACTION_STR, JOPACKTYPE_STREAMMEDIA_HEARTBEAT)
        << __SRS_JOBJECT_END
        << JSON_CRLF;
//    ssize_t nsize;
//    skt->write((char*)res.str().c_str(), res.str().length(), &nsize);
}

void SrsChargeConn::get_topup_info(T_ClientTopUpData &res, const nx_json *js)
{
    if (NULL == js)
    {
        return;
    }

    const nx_json * js_session = nx_json_get(js, JSON_SESSION_STR);
    if (NULL != js_session)
    {
        res.sessionid_ = js_session->text_value;
    }

    const nx_json* js_money = nx_json_get(js, JSON_MONEY_STR);
    if (NULL != js_money)
    {
        if (NULL != js_money->text_value)
        {
            res.money = js_money->text_value;
        }
    }
}

void SrsChargeConn::handle_topup_data(const T_ClientTopUpData &data)
{
    std::stringstream res;
    ssize_t nsize = 0;
//    MySqlProcess* sql = MySqlProcess::get_instance();
    std::string moneyleft_str;
    double moneynew = 0.00;
    std::string usr_name;

//    if (NULL == sql) {
//        srs_error("handle_topup_data, NULL==sql.");
//        goto chargefailed;
//    }

    get_usr_name_redis(usr_name, data.sessionid_);
    if (usr_name.length() <= 0)
    {
        srs_error("handle_topup_data:get usr name NULL.");
        goto chargefailed;
    }

    //input money format illigal.
    if (data.money.length() <= 0)
    {
        srs_error("handle_topup_data: money <=0");
        goto chargefailed;
    }

//    {
//        sql->lock_db();

//        sql->select_tb_usr_charge_left_single("DeviceSys", usr_name, moneyleft_str);
//        if (moneyleft_str.length() <= 0) {//means there is no this user in mysql, should insert into.
//            sql->insert_tb_usr_charge("DeviceSys", usr_name, data.money);
//            update_money_redis(usr_name, atof(data.money.c_str()));
//        } else {
//            //update mysql
//            moneynew = atof(moneyleft_str.c_str()) + atof(data.money.c_str());
//            sql->update_tb_usr_charge_left("DeviceSys", usr_name, moneynew);
//            //update redis
//            update_money_redis(usr_name, moneynew);
//        }

//        //insert tb_usr_charge_history
//        sql->insert_tb_usr_charge_history("DeviceSys", usr_name, data.money);

//        sql->unlock_db();
//    }

    make_json_back(res, CHARGE_ERROR_SUCCESS, "topup success.");
    skt->write((char *)res.str().c_str(), res.str().length(), &nsize);
    return;

chargefailed:
    make_json_back(res, CHARGE_ERROR_TOPUP_FAILED, "failed: topup failed..");
    skt->write((char *)res.str().c_str(), res.str().length(), &nsize);
}

void SrsChargeConn::get_usr_name_redis(std::string &usr_name, const std::string &sessionid)
{
    RedisProcess *ob = RedisProcess::get_instance();
    if (NULL == ob)
    {
        srs_error("get_usr_name_redis:get ReidsProcess NULL.");
        return;
    }

    std::stringstream cmd;
    cmd << "GET " << sessionid;
    ob->get(usr_name, DB_SESSIONID_USRNAME, cmd.str().c_str());

}

void SrsChargeConn::update_money_redis(const std::string &usr, const uint32_t &money)
{
    RedisProcess *ob = RedisProcess::get_instance();
    if (NULL == ob)
    {
        srs_error("add_money_redis: get ReidsProcess NULL.");
        return;
    }

    std::stringstream cmd;
    cmd << "SET " << usr << " " << money;
    ob->set(DB_USRNAME_MONEYLEFT, cmd.str().c_str());
}

void SrsChargeConn::get_usr_info_token(t_out_AntiStrealLinkTokenData &out, const std::string &token)
{
    JPLTDecodeToken(&out, (uint8_t* )token.c_str());
}

void SrsChargeConn::get_flow_redis_old(double &old_money, RedisProcess* nosql, const std::string &usrname)
{
    if (NULL == nosql)
    {
        return;
    }

    std::string val;
    std::stringstream cmd;
    cmd << "GET " << usrname;
    nosql->get(val, DB_USRNAME_MONEYLEFT, cmd.str().c_str());

    old_money = atof(val.c_str());
}

void SrsChargeConn::set_flow_redis_new(double should_sub, RedisProcess *nosql, const std::string &usrname)
{
    if (NULL == nosql)
    {
        return;
    }

    should_sub = (-1) * should_sub;
    std::stringstream cmd;
    cmd << "INCRBYFLOAT " << usrname << " " << should_sub;
    nosql->set(DB_USRNAME_MONEYLEFT, cmd.str().c_str());
}

void SrsChargeConn::add_using_money_in_memory(double &money, const std::string &usrname, const double &used_money)
{
    std::map<std::string, double>::iterator iter = map_users_money.find(usrname);

    if (iter == map_users_money.end())
    {
        map_users_money.insert(std::make_pair(usrname, used_money));

        iter = map_users_money.find(usrname);
        iter->second = 0;//init money value.
    }

    iter->second += used_money;
    money = iter->second;
}

void SrsChargeConn::subtract_using_money_in_memory(const std::string &usrname)
{
    std::map<std::string, double>::iterator iter = map_users_money.find(usrname);
    if (iter != map_users_money.end())
    {
        iter->second -= 0.01;
    }
}

void make_json_back(std::stringstream &res, e_CHARGE_ERROR_CODE err, const char* msg)
{
    res << __SRS_JOBJECT_START
        << __SRS_JFIELD_STR(JSON_ERROR_STR, err) << __SRS_JFIELD_CONT
        << __SRS_JFIELD_STR(JSON_MSG_STR, msg)
        << __SRS_JOBJECT_END << JSON_CRLF;
}


void get_local_system_time(std::stringstream &timenow_str)
{
    time_t now;
    struct tm *timenow;
    time(&now);
    timenow = localtime(&now);
    timenow_str << (timenow->tm_year + 1900) <<"-"
                << (timenow->tm_mon + 1) << "-"
                << timenow->tm_mday << " "
                << timenow->tm_hour << ":" << timenow->tm_min << ":" << timenow->tm_sec;
}
