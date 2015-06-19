#ifndef SRSSTATISTICALFLOW_H
#define SRSSTATISTICALFLOW_H

#include "app_thread.h"
#include "app_reload_handler.h"
#include "mysql_process.h"
#include "redis_dbnum.h"

/**
 * @brief statistic the flow.
 */
class MysqlCache : public ISrsThreadHandler, public ISrsReloadHandler
{
public:
    MysqlCache();
    ~MysqlCache();
public:
    virtual int start();
    virtual void stop();
// interface ISrsThreadHandler.
public:
    virtual int cycle();
    virtual void on_thread_stop();
private:
    void cache_TB_tb_usr_charge_left_2_redis();
    void cache_TB_user_device_channel_vas_2_redis();

private:
    SrsThread* pthread;

    enum {CACHE_DATABASE_TIME_INTERVAL = 60};
};

#endif // SRSSTATISTICALFLOW_H
