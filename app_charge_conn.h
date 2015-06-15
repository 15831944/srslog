#pragma once

#include "app_conn.h"
#include "app_reload_handler.h"
#include "app_st_socket.h"
#include "app_charge_server.h"
#include "app_json.h"
#include "charge_pack_type.h"
#include <stdint.h>
#include <vector>
#include "mysql_process.h"
#include "redis_process.h"
#include <map>
#include "generate_token.h"

typedef enum CHARGE_TYPE
{
    TYPE_FLOW_SHARE = 1,
}e_CHARGE_TYPE;

typedef struct ClientFlowData
{
    std::string app;
    std::string stream;
    e_JOSTREAMMEDIA mode;
    int64_t flow_data;
    std::string usr_data;
    bool can_charge_;
    ClientFlowData()
    {
        flow_data = 0;
        mode = JOSTREAMMEDIA_FLOWDATA_LINKMODE_UNKNOWN;
        can_charge_ = false;
    }
}T_ClientFlowData;

typedef struct ClientTopUpData
{
    std::string sessionid_;
    std::string money;
    ClientTopUpData()
    {
        money = "0.00";
    }
}T_ClientTopUpData;

//the timeout to wait client data,
//if timeout, close the connection.
#define SRS_CONSTS_ACCOUNT_SEND_TIMEOUT_US (int64_t)(30*1000*1000LL)

// the timeout to send data to client,
// if timeout, close the connection.
#define SRS_CONSTS_ACCOUNT_RECV_TIMEOUT_US (int64_t)(30*1000*1000LL)


class SrsChargeConn : public virtual SrsConnection, public virtual ISrsReloadHandler
{
private:
    SrsChargeServer *proxy;
public:
    SrsChargeConn(SrsServer* srs_server, st_netfd_t client_stfd);
    virtual ~SrsChargeConn();
public:
    virtual void kbps_resample();
    // interface IKbpsDelta
public:
    virtual int64_t get_send_bytes_delta();
    virtual int64_t get_recv_bytes_delta();
protected:
    virtual int do_cycle();
private:
    bool handle_json_data(const std::string &data);
    void get_flow_info(std::vector<T_ClientFlowData> &res, const nx_json *js_);
    void handle_flow_data(const std::vector<T_ClientFlowData> &res);
    void handle_heart_beat();
    void get_topup_info(T_ClientTopUpData &, const nx_json * );
    void handle_topup_data(const T_ClientTopUpData &);
    void get_usr_name_redis(std::string &, const std::string &);
    void update_money_redis(const std::string &, const uint32_t &);
    void get_usr_info_token(t_out_AntiStrealLinkTokenData &,const std::string &token);
    void get_flow_redis_old(double &flow, RedisProcess* nosql, const std::string &usrname);
    void set_flow_redis_new(double flow, RedisProcess* nosql, const std::string &usrname);
    void add_using_money_in_memory(double &money, const std::string &usrname, const double &used_money);
    void subtract_using_money_in_memory(const std::string &usrname);
private:
    SrsStSocket* skt;
    std::map<std::string, double> map_users_money;
};

void make_json_back(std::stringstream &res, e_CHARGE_ERROR_CODE err, const char* msg);
void get_local_system_time(std::stringstream &stringstream);
