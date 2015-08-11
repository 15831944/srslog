#pragma once
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
#include "config_info.h"
#include "mutexlock.h"

typedef enum e_RedisDBNum {
    DB_REDIS_RECORDINFO = 0,
}e_RedisDBNum;

class RedisProcess
{
public:
    RedisProcess();
    ~RedisProcess();

    void set_redis(std::string ip, int port);
    bool connect();
    bool close_connect();

    bool select_db(e_RedisDBNum dbnum);

    //for record
    bool record_key_exist(int &res, e_RedisDBNum, const char*);
    bool insert_record_key(e_RedisDBNum, const char*);
    bool delete_record_key(e_RedisDBNum, const char*);

    //multi
    bool start_multi();
    bool multi_quene_cmd(const std::string &cmd);
    bool exe_multi();

private:
    std::string redis_ip_;
    int redis_port_;
    redisContext* pcontext_;
};

extern RedisProcess* g_redis;
extern MutexLock g_mu_redis;
