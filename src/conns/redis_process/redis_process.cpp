#include "redis_process.h"
#include <sstream>

RedisProcess* g_redis = new RedisProcess();
MutexLock g_mu_redis;

RedisProcess::RedisProcess() :
    redis_ip_("127.0.0.1"), redis_port_(15870),
    pcontext_(NULL)
{

}

RedisProcess::~RedisProcess()
{
    close_connect();
}

void RedisProcess::set_redis(std::string ip, int port)
{
    if (ip.length() <= 0 || port <= 0)
    {
        return;
    }

    {
        redis_ip_ = ip;
        redis_port_ = port;
    }
}

bool RedisProcess::connect()
{
    close_connect();

    pcontext_ = redisConnect(redis_ip_.c_str(), redis_port_);
    if ( pcontext_->err)
    {
        redisFree(pcontext_);
        srs_error("Connect to redisServer faile");
        return false;
    }

    srs_trace("Connect to redisServer Success");
    return true;
}

bool RedisProcess::select_db(e_RedisDBNum dbnum)
{
    std::stringstream cmd_changedb;
    cmd_changedb << "SELECT " << dbnum;
    redisReply* rchangedb = (redisReply*)redisCommand(pcontext_, cmd_changedb.str().c_str());
    if (NULL == rchangedb)
    {
        srs_error("set:select db %d failed.NULL == rchangedb", dbnum);
        return false;
    }
    if (!(strcasecmp(rchangedb->str,"OK") == 0))
    {
        srs_error("set:select db %d failed.", dbnum);
        freeReplyObject(rchangedb);
        return false;
    }
    freeReplyObject(rchangedb);
    return true;
}

//bool RedisProcess::get_recordinfo(std::string &res, e_RedisDBNum dbnum, const char *command)
//{
//    if (NULL == command)
//    {
//        return false;
//    }

//    //change db
//    if (!select_db(dbnum))
//    {
//        srs_error("changedb %d failed.", dbnum);
//        return false;
//    }

//    redisReply *r = (redisReply*)redisCommand(pcontext_, command);
//    if ( REDIS_REPLY_NIL == r->type)
//    {
//        freeReplyObject(r);
//        return false;
//    }

//    res = r->integer;

//    freeReplyObject(r);

//    return true;
//}

bool RedisProcess::close_connect()
{
    if (NULL != pcontext_)
    {
        redisFree(pcontext_);
        pcontext_ = NULL;
    }

    return true;
}

bool RedisProcess::record_key_exist(int &res, e_RedisDBNum dbnum, const char *command)
{
    if (NULL == command)
    {
        return false;
    }

    //change db
    if (!select_db(dbnum))
    {
        srs_error("changedb %d failed.", dbnum);
        return false;
    }

    redisReply *r = (redisReply*)redisCommand(pcontext_, command);
    if ( REDIS_REPLY_NIL == r->type)
    {
        freeReplyObject(r);
        return false;
    }

    res = r->integer;

    freeReplyObject(r);

    return true;
}

bool RedisProcess::insert_record_key(e_RedisDBNum dbnum, const char *command)
{
    if (NULL == command)
    {
        return false;
    }

    //change db
    if (!select_db(dbnum))
    {
        srs_error("changedb %d failed.", dbnum);
        return false;
    }

    redisReply *r = (redisReply*)redisCommand(pcontext_, command);
    if ( REDIS_REPLY_NIL == r->type)
    {
        freeReplyObject(r);
        return false;
    }

    freeReplyObject(r);

    return true;
}

bool RedisProcess::delete_record_key(e_RedisDBNum dbnum, const char *command)
{
    if (NULL == command)
    {
        return false;
    }

    //change db
    if (!select_db(dbnum))
    {
        srs_error("changedb %d failed.", dbnum);
        return false;
    }

    redisReply *r = (redisReply*)redisCommand(pcontext_, command);
    if ( REDIS_REPLY_NIL == r->type)
    {
        freeReplyObject(r);
        return false;
    }

    freeReplyObject(r);

    return true;
}

bool RedisProcess::start_multi()
{
    redisReply *r = (redisReply*)redisCommand(pcontext_, "MULTI");
    if ( REDIS_REPLY_NIL == r->type) {
        freeReplyObject(r);
        return false;
    }

    if (r->str != "OK") {
        freeReplyObject(r);
        return false;
    }

    freeReplyObject(r);

    return true;
}

bool RedisProcess::multi_quene_cmd(const std::string &cmd)
{
    if (cmd.length() <= 0) {
        return false;
    }

    redisReply *r = (redisReply*)redisCommand(pcontext_, cmd.c_str());
    if ( REDIS_REPLY_NIL == r->type) {
        freeReplyObject(r);
        return false;
    }

    if (strcmp(r->str, "QUEUED") != 0) {
        freeReplyObject(r);
        return false;
    }

    freeReplyObject(r);
    return true;
}

bool RedisProcess::exe_multi()
{
    redisReply *r = (redisReply*)redisCommand(pcontext_, "EXEC");
    if ( REDIS_REPLY_NIL == r->type) {
        freeReplyObject(r);
        return false;
    }

    freeReplyObject(r);

    return true;
}

