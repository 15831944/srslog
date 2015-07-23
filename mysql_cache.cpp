#include "mysql_cache.h"
#include "app_log.h"
#include "app_error_code.h"
#include "app_config.h"
#include "app_server.h"
#include "app_utility.h"
#include "st.h"
#include "app_st.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include "redis_process.h"
#include "redis_dbnum.h"
#include <sstream>
#include <set>

#define SRS_AUTO_STATISTIC_FLOW_SLEEP_US (int64_t)(6*100*1000LL)

extern SrsServer *_srs_server;

#if 0

MysqlCache::MysqlCache()
{
    _srs_config->subscribe(this);
    pthread = new SrsThread(this, SRS_AUTO_STATISTIC_FLOW_SLEEP_US, true);
}

MysqlCache::~MysqlCache()
{
    _srs_config->unsubscribe(this);
    srs_freep(pthread);
}

int MysqlCache::start()
{
    int ret = ERROR_SUCCESS;

    if ((ret = pthread->start()) != ERROR_SUCCESS) {
        srs_error("SrsStatisticalFlow: st_thread_create failed. ret=%d", ret);
        return ret;
    }

    srs_trace("SrsStatisticalFlow thread cid=%d, current_cid=%d", pthread->cid(), _srs_context->get_id());

    return ret;
}

void MysqlCache::stop()
{
    pthread->stop();
}

int MysqlCache::cycle()
{
    int ret = ERROR_SUCCESS;

    MySqlProcess* mysql = MySqlProcess::get_instance();
    if (NULL == mysql) {
        return -1;
    }

    srs_trace("mysql init:");
    mysql->init(_srs_config->get_mysql_ip(), _srs_config->get_mysql_port(),
                _srs_config->get_mysql_usr(), _srs_config->get_mysql_passwd());

    srs_trace("begin cache mysql data to redis.");
    cache_TB_tb_usr_charge_left_2_redis();
    cache_TB_user_device_channel_vas_2_redis();

    while (true) {

        st_sleep(CACHE_DATABASE_TIME_INTERVAL);
    }

    return ret;
}

void MysqlCache::on_thread_stop()
{

}

void MysqlCache::cache_TB_tb_usr_charge_left_2_redis()
{
    //get data from mysql.
    MySqlProcess* mysql = MySqlProcess::get_instance();
    if (NULL == mysql) {
        return;
    }

    std::vector<tb_usr_charge_left> balance_res;
    {
        mysql->lock_db();
        mysql->select_tb_usr_charge_left_all("DeviceSys", balance_res);
        mysql->unlock_db();
    }

    if (balance_res.size() <= 0) {
        return;
    }

    RedisProcess* nosql = RedisProcess::get_instance();
    if (NULL == nosql) {
        srs_error("cache_TB_tb_usr_charge_left_2_redis: NULL == nosql");
        return;
    }

    for (int i = 0; i < balance_res.size(); ++i) {
        std::stringstream cmd;
        cmd <<  "set "
                << balance_res[i].usr << " " << balance_res[i].charge;
        nosql->set(DB_USRNAME_MONEYLEFT, cmd.str().c_str());
    }
}

void MysqlCache::cache_TB_user_device_channel_vas_2_redis()
{
    //get data from mysql.
    MySqlProcess* mysql = MySqlProcess::get_instance();
    if (NULL == mysql) {
        return;
    }

    std::vector<tb_user_device_vas> res;
    {
        mysql->lock_db();
        mysql->select_tb_user_device_vas("DeviceSys", res);
        mysql->unlock_db();
    }

    if(res.size() <= 0) {
        return;
    }

    //cache to redis. user:device:channel_flag
    RedisProcess* nosql = RedisProcess::get_instance();
    if (NULL == nosql) {
        srs_error("cache_TB_user_device_vas: NULL == nosql");
        return;
    }

    for (int i = 0; i < res.size(); ++i) {
        std::stringstream cmd;
        cmd <<  "set "
                << res[i].usr_name << ":" << res[i].device_guid << ":" << res[i].channel_no
                << " " << res[i].share_website_flag;
        nosql->set(DB_USRDEVCHANNEL_SHAREWEBSITEFLAG, cmd.str().c_str());

        std::stringstream cmd_sets;
        cmd_sets << "SADD "
                 << res[i].device_guid << ":" << res[i].channel_no << " "
                 << res[i].usr_name ;
        nosql->set(DB_SETS_USRDEVCHANNEL_USERS, cmd_sets.str().c_str());
    }
}

#endif
