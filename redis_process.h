#pragma once
#include <stdio.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <hiredis/hiredis.h>
#include <vector>
#include "app_log.h"
#include "redis_dbnum.h"
#include "mutexlock.h"
#include "checkcharge.h"

class RedisProcess
{
public:
    static RedisProcess *get_instance();
    ~RedisProcess();

    void set_redis(std::string ip, int port);
    bool connect();

    bool select_db(e_RedisDBNum dbnum);
    bool set(e_RedisDBNum , const char *);
    bool get(std::string &, e_RedisDBNum ,  const char *);
    bool get(int&, e_RedisDBNum ,  const char *);
    bool get_smembers(std::vector<std::string> &vec, e_RedisDBNum, const char*);
    bool get_all_keys(std::vector<std::string>&);
    bool get_single_userinfo(std::string user, T_UserDataInfo &data);

    //multi
    bool start_multi();
    bool multi_quene_cmd(const std::string &cmd);
    bool exe_multi();

    bool query_usage(e_RedisDBNum dbnum, const std::string &cmd,
                     std::map<std::string, std::string> &res);

    static MutexLock mu_;
private:
    RedisProcess();
    bool close_connect();

private:
    static RedisProcess *ob_;

    std::string redis_ip_;
    int redis_port_;

    redisContext* pcontext_;
    bool has_free_redis_;
};

