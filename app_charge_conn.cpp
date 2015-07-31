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
#include "aescry.h"
#include <stdint.h>

std::string g_alarm_ip;
std::string g_alarm_port;

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
        srs_error("NULL == proxy");
        return ret;
    }

    proxy->set_recv_timeout(SRS_CONSTS_ACCOUNT_RECV_TIMEOUT_US);
    proxy->set_send_timeout(SRS_CONSTS_ACCOUNT_SEND_TIMEOUT_US);

    int sessionidint = rand();
    std::stringstream ss;
    ss << sessionidint;
    sessionid_ = ss.str();

    //send sessionid firt.
    ssize_t nsize = 0;
    skt->write((char *)sessionid_.c_str(), sessionid_.length(), &nsize);

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

    nsize = 0;
    skt->write((char *)res.c_str(), res.length(), &nsize);

    return ret;
}

void SrsChargeConn::handle_client_data(std::string data, std::string &res)
{
    if (data.length() <= 0) {
        return;
    }

    bool ret = g_aes.SetKey();
    if (!ret) {
        return;
    }

    int len = data.length();
    char buff[1024] = {0};
    g_aes.Decode((uint8_t*)buff, (uint8_t*)data.c_str(), len);

    std::string decode_data;
    decode_data.assign(buff, len);

    int npos = decode_data.find_last_of("}");
    std::string jsondata = decode_data.substr(0, npos + 1);

    handle_json(jsondata, res);
}

void SrsChargeConn::handle_json(std::string jsondata, std::string &res)
{
    const nx_json *js_ = NULL;
    js_ = nx_json_parse_utf8(const_cast<char *>(jsondata.c_str()));
    if (NULL == js_)
    {
        srs_error("handle_json:js==NULL");
        return ;
    }

    const nx_json * js_type = nx_json_get(js_, JS_CHARGE_TYPE);
    if (NULL == js_type)
    {
        srs_error("handle_json: js_type==NULL.");
        return ;
    }

    int type = -1;
    type = atoi(js_type->text_value);

    switch (type) {
    case __SRS_TYPE_CHARGE:
    {
        handle_charge(js_, res);
    }
        break;
    case __SRS_TYPE_BUY_PACK:
    {
        handle_buy_pack(js_, res);
    }
        break;
    case __SRS_TYPE_ALARM_INFO:
    {
        handle_alarminfo(js_, res);
    }
        break;
    case __SRS_TYPE_QUERY_INFO:
    {
        handle_queryinfo(js_, res);
    }
        break;
    default:
        break;
    }


    if (NULL != js_)
    {
        nx_json_free(js_);
    }
}

void SrsChargeConn::handle_charge(const nx_json *js, std::string &res)
{
    std::stringstream ss;

    std::string user;
    std::string money;
    std::string sessionid;

    const nx_json * js_user = nx_json_get(js, JS_CHARGE_USER);
    if (NULL != js_user) {
        user = js_user->text_value;
    }

    const nx_json* js_money = nx_json_get(js, JS_CHARGE_MONEY);
    if (NULL != js_money){
        if (NULL != js_money->text_value) {
            money = js_money->text_value;
        }
    }

    const nx_json* js_ip = nx_json_get(js, JS_CHARGE_SESSIONID);
    if (NULL != js_ip) {
        if (NULL != js_ip->text_value) {
            sessionid = js_ip->text_value;
        }
    }

    if (sessionid != sessionid_) {
       return ;
    }


    float fmoney = atof(money.c_str());
    if (fmoney <= 0) {
        ss << __SRS_JOBJECT_START
           << __SRS_JFIELD_STR(JS_CHARGE_TYPE, __SRS_TYPE_CHARGE) << __SRS_JFIELD_CONT
           << __SRS_JFIELD_STR(JS_CHARGE_RET, JCC_CHARGE_FAILED)
           << __SRS_JOBJECT_END;
        res = ss.str();

        return;
    }

    RedisProcess::mu_.Lock();
    bool ret = redis_charge(user, money);
    RedisProcess::mu_.UnLock();

    if (ret) {
        ss << __SRS_JOBJECT_START
           << __SRS_JFIELD_STR(JS_CHARGE_TYPE, __SRS_TYPE_CHARGE) << __SRS_JFIELD_CONT
           << __SRS_JFIELD_STR(JS_CHARGE_RET, JCC_SUCCESS)
           << __SRS_JOBJECT_END;
    } else {
        ss << __SRS_JOBJECT_START
           << __SRS_JFIELD_STR(JS_CHARGE_TYPE, __SRS_TYPE_CHARGE) << __SRS_JFIELD_CONT
           << __SRS_JFIELD_STR(JS_CHARGE_RET, JCC_CHARGE_FAILED)
           << __SRS_JOBJECT_END;
    }

    res = ss.str();
}

void SrsChargeConn::handle_buy_pack(const nx_json *js, std::string &res)
{
    std::stringstream ss;
    std::string user;
    std::string money;
    std::string flow_value;
    std::string session;

    const nx_json * js_user = nx_json_get(js, JS_CHARGE_USER);
    if (NULL != js_user) {
        user = js_user->text_value;
    }

    const nx_json* js_money = nx_json_get(js, JS_CHARGE_MONEY);
    if (NULL != js_money) {
        if (NULL != js_money->text_value){
            money = js_money->text_value;
        }
    }

    const nx_json* js_flowdata = nx_json_get(js, JS_CHARGE_FLOW);
    if (NULL != js_flowdata) {
        if (NULL != js_flowdata->text_value) {
            flow_value = js_flowdata->text_value;
        }
    }

    const nx_json* js_session = nx_json_get(js, JS_CHARGE_SESSIONID);
    if (NULL != js_session) {
        if (NULL != js_session->text_value) {
            session = js_session->text_value;
        }
    }

    if (session != sessionid_) {
        return;
    }

    float f_money = atof(money.c_str());
    uint64_t i_flow_value = atof(flow_value.c_str());
    if (f_money < 0.0 || i_flow_value < 0) {
        ss << __SRS_JOBJECT_START
           << __SRS_JFIELD_STR(JS_CHARGE_TYPE, __SRS_TYPE_BUY_PACK) << __SRS_JFIELD_CONT
           << __SRS_JFIELD_STR(JS_CHARGE_RET, JCC_BUY_PACKAGE_FAILED)
           << __SRS_JOBJECT_END;
        res = ss.str();

        return;
    }

    RedisProcess::mu_.Lock();
    bool ret = redis_buy_pack(user, money, flow_value);
    RedisProcess::mu_.UnLock();

    if (ret) {
        ss << __SRS_JOBJECT_START
           << __SRS_JFIELD_STR(JS_CHARGE_TYPE, __SRS_TYPE_CHARGE) << __SRS_JFIELD_CONT
           << __SRS_JFIELD_STR(JS_CHARGE_RET, JCC_SUCCESS)
           << __SRS_JOBJECT_END;
    } else {
        ss << __SRS_JOBJECT_START
           << __SRS_JFIELD_STR(JS_CHARGE_TYPE, __SRS_TYPE_CHARGE) << __SRS_JFIELD_CONT
           << __SRS_JFIELD_STR(JS_CHARGE_RET, JCC_BUY_PACKAGE_FAILED)
           << __SRS_JOBJECT_END;
    }

    res = ss.str();
}

void SrsChargeConn::handle_queryinfo(const nx_json *js, std::string &res)
{
    std::string user;
    std::string session;

    const nx_json * js_user = nx_json_get(js, JS_CHARGE_USER);
    if (NULL != js_user) {
        if (NULL != js_user->text_value){
            user = js_user->text_value;
        }
    }

    const nx_json* js_session = nx_json_get(js, JS_CHARGE_SESSIONID);
    if (NULL != js_session) {
        if (NULL != js_session->text_value) {
            session = js_session->text_value;
        }
    }

    if (session != sessionid_) {
        return;
    }

    RedisProcess::mu_.Lock();
    bool ret = redis_query_usage(user, res);
    RedisProcess::mu_.UnLock();
}

void SrsChargeConn::handle_alarminfo(const nx_json *js, std::string &res)
{
    std::string ip;
    std::string port;
    std::string session;

    const nx_json* js_session = nx_json_get(js, JS_CHARGE_SESSIONID);
    if (NULL != js_session) {
        if (NULL != js_session->text_value){
            session = js_session->text_value;
        }
    }

    if (sessionid_ != session) {
        return;
    }

    const nx_json* js_ip = nx_json_get(js, JS_CHARGE__ALARMIP);
    if (NULL != js_ip) {
        if (NULL != js_ip->text_value) {
            ip = js_ip->text_value;
        }
    }

    const nx_json* js_port = nx_json_get(js, JS_CHARGE_ALARMPORT);
    if (NULL != js_port) {
        if (NULL != js_port->text_value) {
            port = js_port->text_value;
        }
    }

    std::stringstream ss;
    if (ip.length() > 0 && port.length() > 0) {
        g_alarm_ip = ip;
        g_alarm_port = port;

        ss << __SRS_JOBJECT_START
           << __SRS_JFIELD_STR(JS_CHARGE_TYPE, __SRS_TYPE_ALARM_INFO) << __SRS_JFIELD_CONT
           << __SRS_JFIELD_STR(JS_CHARGE_RET, JCC_SUCCESS)
           << __SRS_JOBJECT_END;
    } else {
        ss << __SRS_JOBJECT_START
           << __SRS_JFIELD_STR(JS_CHARGE_TYPE, __SRS_TYPE_ALARM_INFO) << __SRS_JFIELD_CONT
           << __SRS_JFIELD_STR(JS_CHARGE_RET, JCC_NOT_SET_SERVER)
           << __SRS_JOBJECT_END;
    }

    res = ss.str();
}

bool SrsChargeConn::redis_charge(const std::string &user, const std::string &money)
{
    RedisProcess *ob = RedisProcess::get_instance();
    if (NULL == ob)
    {
        srs_error("redis_charge: get ReidsProcess NULL.");
        return false;
    }

    std::stringstream cmd;
    cmd << "HINCRBYFLOAT " << user << " money " << money;
    bool ret = ob->set(DB_USER_INFO, cmd.str().c_str());
    return ret;
}

bool SrsChargeConn::redis_buy_pack(const std::string &user, const std::string &money,
                                   const std::string &flow)
{
    RedisProcess *ob = RedisProcess::get_instance();
    if (NULL == ob)
    {
        srs_error("redis_buy_pack: get ReidsProcess NULL.");
        return false;
    }

    std::stringstream cmd;
    //get money left in redis.
    cmd << "HGET " << user << " money";
    std::string res;
    bool ret = ob->get(res, DB_USER_INFO, cmd.str().c_str());
    float leftmoney = atof(res.c_str());
    float use_money = atof(money.c_str());
    if (!ret || leftmoney - use_money < 0 ) {
        return false;
    }

    ob->select_db(DB_USER_INFO);

    ob->start_multi();
    //sub money.
    std::stringstream ss_submoney;
    ss_submoney << "HINCRBYFLOAT " << user << " money " << "-" << money;
    ret = ob->multi_quene_cmd(ss_submoney.str());
    //set flow
    std::stringstream ss_setflow;
    ss_setflow << "HSET " << user << " flowtotal " << flow;
    //add flow.
    std::stringstream ss_addflow;
    ss_addflow << "HINCRBY " << user << " flow " << flow;
    ret = ob->multi_quene_cmd(ss_addflow.str());
    //set open date
    std::string systemdate;
    get_system_date(systemdate);
    std::stringstream ss_open_date;
    ss_open_date << "HSET "<< user << " opendate " << systemdate;
    ret = ob->multi_quene_cmd(ss_open_date.str());

    //exec multi
    ret = ob->exe_multi();

    return ret;
}

bool SrsChargeConn::redis_query_usage(const std::string &user, std::string &res)
{
    RedisProcess *ob = RedisProcess::get_instance();
    if (NULL == ob) {
        srs_error("redis_query_usage: get ReidsProcess NULL.");
        return false;
    }

    std::map<std::string, std::string> resdata;
    std::stringstream cmd;
    cmd << "HGETALL " << user;
    bool ret = ob->query_usage(DB_USER_INFO, cmd.str(), resdata);
    if (!ret) {
        return false;
    }

    std::stringstream ss;
    ss << __SRS_JOBJECT_START;
    ss  << __SRS_JFIELD_STR(JS_CHARGE_TYPE, __SRS_TYPE_QUERY_INFO) << __SRS_JFIELD_CONT;

    for (std::map<std::string, std::string>::iterator iter = resdata.begin();
         iter != resdata.end();
         ++iter) {

        if (JS_CHARGE_MONEY == iter->first){
            ss  << __SRS_JFIELD_STR(JS_CHARGE_MONEY, iter->second) << __SRS_JFIELD_CONT;
        } else if (JS_CHARGE_FLOW == iter->first) {
            ss  << __SRS_JFIELD_STR(JS_CHARGE_FLOW, iter->second) << __SRS_JFIELD_CONT;
        } else if (JS_CHARGE_OPENDATE == iter->first) {
            ss  << __SRS_JFIELD_STR(JS_CHARGE_OPENDATE, iter->second) << __SRS_JFIELD_CONT;
        } else if (JS_CHARGE_FLOWTOTAL == iter->first) {
            ss  << __SRS_JFIELD_STR(JS_CHARGE_FLOWTOTAL, iter->second) << __SRS_JFIELD_CONT;
        } else if (JS_CHARGE_NFLOWUSED == iter->first) {
            ss  << __SRS_JFIELD_STR(JS_CHARGE_NFLOWUSED, iter->second) << __SRS_JFIELD_CONT;
        } else if (JS_CHARGE_NMONEYUSED == iter->first) {
            ss  << __SRS_JFIELD_STR(JS_CHARGE_NMONEYUSED, iter->second) << __SRS_JFIELD_CONT;
        }

    }

    std::string temp_str;
    temp_str = ss.str();
    int npos = temp_str.find_last_of(",");
    temp_str = temp_str.substr(0, npos);
    temp_str += __SRS_JOBJECT_END;
    res = temp_str;

    return true;
}

void SrsChargeConn::get_system_date(std::string &res)
{
    time_t now;
    struct tm *timenow;
    time(&now);
    timenow = localtime(&now);
    char buff[64] = {'\0'};
    sprintf( buff,"%d0%d%d",
             timenow->tm_year + 1900,
             timenow->tm_mon + 1,
             timenow->tm_mday);
    res.assign(buff, strlen(buff));
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
            //            handle_flow_data(res);
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

        const nx_json* js_flow = nx_json_get(js_local, JS_CHARGE_FLOW);
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

//void SrsChargeConn::handle_flow_data(const std::vector<T_ClientFlowData> &res)
//{
//    RedisProcess* nosql = RedisProcess::get_instance();
//    if (NULL == nosql)
//    {
//        return;
//    }

////    MySqlProcess *sql = MySqlProcess::get_instance();
////    if (NULL == sql) {
////        return;
////    }

//    double price = 0;
//    for (int i = 0; i < res.size(); ++i)
//    {
//        //get usr info
//        t_out_AntiStrealLinkTokenData outdata;
//        get_usr_info_token(outdata, res[i].usr_data);

//        //decide the price
//        switch(outdata.type_)
//        {
//        case ShareWebsiteAntiStrealLinkToken:
//        {
//            //            price = price_flow_per_byte;
//            price = test_price_flow_per_byte; //for test. todo.
//        }
//            break;
//        default:
//        {
//            continue;
//        }
//        }

//        //usr play mode.
//        if (JOSTREAMMEDIA_FLOWDATA_LINKMODE_PLAY == res[i].mode) {
//            //judge if need to charge
//            std::stringstream cmd_cancharge;
//            cmd_cancharge << "get " << outdata.usr_name_ << ":" << outdata.devguid_ << ":" << outdata.channel_;
//            std::string ifcharge;
//            nosql->get(ifcharge, DB_USRDEVCHANNEL_SHAREWEBSITEFLAG, cmd_cancharge.str().c_str());
//            if (0 == atoi(ifcharge.c_str())) {
//                continue;//not open this job, not charge it.
//            }

//            //charge process
//            double this_time_used_money = price * res[i].flow_data;

//            double money = 0;
//            add_using_money_in_memory(money, outdata.usr_name_, this_time_used_money);

//            //calculate the money, if more than 0.01 then update redis, else store it first
//            if (money >= 0.01)
//            {
//                //get data from redis.
//                double flow_redis_old = 0;
//                get_flow_redis_old(flow_redis_old, nosql, outdata.usr_name_);

//                //close the mysql share_website flag.
//                if (flow_redis_old <= 0) {
//                    tb_user_device_vas tb;
//                    tb.share_website_flag = 0;
//                    tb.usr_name = outdata.usr_name_;
//                    tb.device_guid = outdata.devguid_;
//                    tb.channel_no = (outdata.channel_);
//                    {
////                        sql->lock_db();
////                        sql->update_tb_user_device_vas("DeviceSys", tb);
////                        sql->unlock_db();
//                    }
//                    continue;
//                }

//                set_flow_redis_new(0.01, nosql, outdata.usr_name_);

//                subtract_using_money_in_memory(outdata.usr_name_);
//            }

//            //update mysql database when used more than 0.1
//            if (money >= 0.1) {
////                sql->lock_db();
////                std::string moneyleft_str;
////                sql->select_tb_usr_charge_left_single("DeviceSys", outdata.usr_name_, moneyleft_str);
////                double moneynew = atof(moneyleft_str.c_str()) - 0.1;
////                sql->update_tb_usr_charge_left("DeviceSys", outdata.usr_name_, moneynew);
////                sql->unlock_db();
//            }

//        } else if (JOSTREAMMEDIA_FLOWDATA_LINKMODE_PUSH == res[i].mode) {
//            //get all usrs
//            std::vector<std::string> users;
//            std::stringstream cmd;
//            cmd << "SMEMBERS " << outdata.devguid_ << ":" << outdata.channel_;
//            nosql->get_smembers(users, DB_SETS_USRDEVCHANNEL_USERS, cmd.str().c_str());
//            if (users.size() <= 0) {
//                continue;
//            }

//            for (int i = 0; i < users.size(); ++i) {

//                //charge process
//                double this_time_used_money = price * res[i].flow_data;

//                double money = 0;
//                add_using_money_in_memory(money, users[i], this_time_used_money);

//                //calculate the money, if more than 0.01 then update redis, else store it first
//                if (money >= 0.01)
//                {
//                    //get data from redis.
//                    double flow_redis_old = 0;
//                    get_flow_redis_old(flow_redis_old, nosql, users[i]);

//                    //close the mysql share_website flag.
//                    if (flow_redis_old <= 0) {
//                        tb_user_device_vas tb;
//                        tb.share_website_flag = 0;
//                        tb.usr_name = users[i];
//                        tb.device_guid = outdata.devguid_;
//                        tb.channel_no = (outdata.channel_);
//                        {
////                            sql->lock_db();
////                            sql->update_tb_user_device_vas("DeviceSys", tb);
////                            sql->unlock_db();
//                        }
//                    }

//                    set_flow_redis_new(0.01, nosql, users[i]);

//                    subtract_using_money_in_memory(users[i]);
//                }

//                //update mysql database when used more than 0.1
//                if (money >= 0.1) {
////                    sql->lock_db();
////                    std::string moneyleft_str;
////                    sql->select_tb_usr_charge_left_single("DeviceSys", users[i], moneyleft_str);
////                    double moneynew = atof(moneyleft_str.c_str()) - 0.1;
////                    sql->update_tb_usr_charge_left("DeviceSys", users[i], moneynew);
////                    sql->unlock_db();
//                }

//            }
//        }
//    }
//}

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

//void SrsChargeConn::get_usr_info_token(t_out_AntiStrealLinkTokenData &out, const std::string &token)
//{
//    JPLTDecodeToken(&out, (uint8_t* )token.c_str());
//}

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

