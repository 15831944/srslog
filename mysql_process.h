#pragma once
#include <stdio.h>
#include <string>
#include <mysql/mysql.h>
#include <vector>

typedef struct tb_user_device_vas
{
    std::string usr_name;
    std::string device_guid;
    int channel_no;
    int share_website_flag;

    tb_user_device_vas()
    {
        share_website_flag = 0;
        channel_no = 0;
    }
}tb_user_device_vas;

typedef struct tb_usr_charge_left
{
    std::string usr;
    std::string charge;
}tb_usr_charge_left;


#if 0

class MySqlProcess
{
public:
    static MySqlProcess * get_instance();
    ~MySqlProcess();

    void init(std::string dbip, int dbport, std::string usr, std::string passwd);

    void lock_db();
    void unlock_db();

    bool select_tb_user_device_vas(const char* db, std::vector<tb_user_device_vas> &res);
    bool update_tb_user_device_vas(const char* db, const tb_user_device_vas &data);
    bool select_tb_usr_charge_left_all(const char* db, std::vector<tb_usr_charge_left> &res);
    bool select_tb_usr_charge_left_single(const char* db, const std::string &usr, std::string &moneyleft);
    bool insert_tb_usr_charge(const char*db, const std::string &usr, const std::string &money);
    bool insert_tb_usr_charge_history(const char*db, const std::string &usr, const std::string &money);
    bool update_tb_usr_charge_left(const char* db, std::string usr, double charge);

private:
    MySqlProcess();
    void do_connect(MYSQL &my_connection, const char* db, const char* cmd);

private:
    static MySqlProcess *ob_;
    std::string dbip_;
    int dbport_;
    std::string usr_;
    std::string passwd_;
    MutexLock lock_;
};

#endif

extern void get_local_system_time(std::stringstream &stringstream);
