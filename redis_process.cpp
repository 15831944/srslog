#include "redis_process.h"
#include <sstream>

RedisProcess* RedisProcess::ob_ = NULL;

RedisProcess::RedisProcess() :
    redis_ip_("127.0.0.1"), redis_port_(6379),
    pcontext_(NULL),
    has_free_redis_(false)
{

}

RedisProcess::~RedisProcess()
{
    if (!has_free_redis_)
    {
        close_connect();
    }

    if (NULL != ob_)
    {
        delete ob_;
        ob_ = NULL;
    }
}

void RedisProcess::set_redis(std::string ip, int port)
{
    if (ip.length() <= 0 || port <= 0)
    {
        return;
    }

    redis_ip_ = ip;
    redis_port_ = port;
}

bool RedisProcess::connect()
{
    close_connect();

    pcontext_ = redisConnect(redis_ip_.c_str(), redis_port_);
    if ( pcontext_->err)
    {
        redisFree(pcontext_);
        has_free_redis_ = true;
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

bool RedisProcess::set(e_RedisDBNum dbnum, const char *command)
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

    redisReply* r = (redisReply*)redisCommand(pcontext_, command);
    if( NULL == r)
    {
        srs_error("Execut command1 %s failure", command);
        return false;
    }

    if( !(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK")==0))
    {
        freeReplyObject(r);
        return false;
    }

    freeReplyObject(r);

    return true;
}

bool RedisProcess::get(std::string &res, e_RedisDBNum dbnum, const char *command)
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

    res = r->str;

    freeReplyObject(r);

    return true;
}

bool RedisProcess::get_smembers(std::vector<std::string> &vec, e_RedisDBNum dbnum, const char *command)
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

    int len = r->elements;
    for (int i = 0; i < len; ++i) {
        vec.push_back(r->element[i]->str);
    }
}

bool RedisProcess::close_connect()
{
    if (NULL != pcontext_)
    {
        redisFree(pcontext_);
        pcontext_ = NULL;
    }

    return true;
}


RedisProcess *RedisProcess::get_instance()
{
    if (NULL == ob_)
    {
        ob_ = new RedisProcess();
    }

    return ob_;
}
