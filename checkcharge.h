#pragma once

#include "app_thread.h"
#include "app_reload_handler.h"
#include <vector>
#include <map>

typedef struct UserDataInfo{
    std::string user;
    std::string money_left_;
    std::string flow_left_;
    std::string flow_total_;
}T_UserDataInfo;

class CheckCharge : public ISrsThreadHandler, public ISrsReloadHandler
{
public:
    CheckCharge();
    ~CheckCharge();

public:
    virtual int start();
    virtual void stop();
// interface ISrsThreadHandler.
public:
    virtual int cycle();
    virtual void on_thread_stop();
private:
    void get_users_need_upload(std::string &);
    void get_single_user_info(const std::string& , T_UserDataInfo&);
    void tell_me_result(const std::map<std::string, T_UserDataInfo>&, std::string &res);
private:
    SrsThread* pthread;
};

extern std::string g_alarm_ip;
extern std::string g_alarm_port;

int jcc_connect_server(std::string ip, int port, int &fd_socket);
