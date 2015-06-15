#include "mysql_process.h"
#include "app_log.h"
#include <sstream>
#include <stdlib.h>
#include <vector>

MySqlProcess* MySqlProcess::ob_ = NULL;

MySqlProcess::MySqlProcess() : dbip_("127.0.0.1"), dbport_(3306)
{

}

void MySqlProcess::do_connect(MYSQL &my_connection, const char* db, const char* cmd)
{
    mysql_init(&my_connection);

    if (mysql_real_connect(&my_connection, dbip_.c_str(), usr_.c_str(), passwd_.c_str(), db, 0,NULL,0))
    {
        int res = mysql_query(&my_connection, cmd);
        if (!res)
        {
        }
        else
        {
            srs_error("failed. cmd=%s", cmd);
        }

        mysql_close(&my_connection);
    }
    else
    {
        if (mysql_errno(&my_connection))
        {
            srs_error("Connection error %d: %sn",mysql_errno(&my_connection),mysql_error(&my_connection));
        }
    }
}

MySqlProcess *MySqlProcess::get_instance()
{
    if (NULL == ob_)
    {
        ob_ = new MySqlProcess();
    }

    return ob_;
}

MySqlProcess::~MySqlProcess()
{
    if (NULL != ob_)
    {
        delete ob_;
        ob_ == NULL;
    }
}

void MySqlProcess::init(std::string dbip, int dbport, std::string usr, std::string passwd)
{
    if (dbip.length() <= 0 || dbport < 0 || usr.length() <= 0 || passwd.length() <= 0)
    {
        return;
    }

    dbip_ = dbip;
    dbport_ = dbport;
    usr_ = usr;
    passwd_ = passwd;
}

void MySqlProcess::lock_db()
{
    lock_.Lock();
}

void MySqlProcess::unlock_db()
{
    lock_.UnLock();
}

bool MySqlProcess::select_tb_user_device_vas(const char *db, std::vector<tb_user_device_vas> &vec_res)
{
    std::stringstream cmd;
    cmd << "select username, device_guid, channel_no, share_website_flag from tb_user_device_vas";

    MYSQL my_connection;
    mysql_init(&my_connection);

    if (mysql_real_connect(&my_connection, dbip_.c_str(), usr_.c_str(), passwd_.c_str(), db, 0,NULL,0))
    {
        int res = mysql_query(&my_connection, cmd.str().c_str());
        if (!res)
        {
            //get data to vector.
            MYSQL_ROW sqlrow;
            MYSQL_RES *res_ptr;
            res_ptr=mysql_store_result(&my_connection);

            if(res_ptr) {
                int column = mysql_num_fields(res_ptr);
                while((sqlrow=mysql_fetch_row(res_ptr))) {
                    tb_user_device_vas vas;
                    for (int j = 0; j < column; ++j) {
                        switch (j)
                        {
                        case 0:
                        {
                            vas.usr_name = sqlrow[j];

                        }
                            break;
                        case 1:
                        {
                            vas.device_guid = sqlrow[j];
                        }
                            break;
                        case 2:
                        {
                            if (NULL != sqlrow[j])
                            {
                                srs_trace("atoi sqlrow[2]==%s", sqlrow[j]);
                                vas.channel_no = atoi(sqlrow[j]);
                                srs_trace("after assign sqlrow[2]==%s", sqlrow[j]);

                            } else {
                                vas.channel_no = 1;
                            }

                        }
                            break;
                        case 3:
                        {
                            if (NULL != sqlrow[j])
                            {
                                vas.share_website_flag = atoi(sqlrow[j]);
                            } else {
                                vas.share_website_flag = 1;
                            }
                        }
                        default:
                        {
                            ;
                        }
                        }
                    }

                    vec_res.push_back(vas);
                }

                if (mysql_errno(&my_connection))  {
                    srs_error("Retrive error:%s\n",mysql_error(&my_connection));
                }
            }

            mysql_free_result(res_ptr);
        }
        else
        {
            srs_error("failed. cmd=%s", cmd.str().c_str());
        }

        mysql_close(&my_connection);
    }
    else
    {
        if (mysql_errno(&my_connection))
        {
            srs_error("Connection error %d: %sn",mysql_errno(&my_connection),mysql_error(&my_connection));
        }
    }
}

bool MySqlProcess::update_tb_user_device_vas(const char *db, const tb_user_device_vas &data)
{
    std::stringstream cmd;
    cmd << "update tb_user_device_vas set share_website_flag=" << data.share_website_flag
        << " where usr=\"" << data.usr_name << "\""
        << " and device_guid=\"" << data.device_guid << "\""
        << " and channel_no=\"" << data.channel_no << "\"";

    MYSQL my_connection;
    do_connect(my_connection, db, cmd.str().c_str());
}

bool MySqlProcess::select_tb_usr_charge_left_all(const char *db, std::vector<tb_usr_charge_left> &vec_res)
{
    std::stringstream cmd;
    cmd << "select usr, charge from tb_usr_charge_left";

    MYSQL my_connection;
    mysql_init(&my_connection);

    if (mysql_real_connect(&my_connection, dbip_.c_str(), usr_.c_str(), passwd_.c_str(), db, 0,NULL,0))
    {
        int res = mysql_query(&my_connection, cmd.str().c_str());
        if (!res)
        {
            //get data to vector.
            MYSQL_ROW sqlrow;
            MYSQL_RES *res_ptr;
            res_ptr=mysql_store_result(&my_connection);

            if(res_ptr) {
                int column = mysql_num_fields(res_ptr);
                while((sqlrow=mysql_fetch_row(res_ptr))) {
                    tb_usr_charge_left vas;
                    for (int j = 0; j < column; ++j) {
                        switch (j)
                        {
                        case 0:
                        {
                            vas.usr = sqlrow[j];

                        }
                            break;
                        case 1:
                        {
                            vas.charge = sqlrow[j];
                        }
                            break;
                        default:
                        {
                            ;
                        }
                        }
                    }

                    vec_res.push_back(vas);
                }

                if (mysql_errno(&my_connection))  {
                    srs_error("Retrive error:%s\n",mysql_error(&my_connection));
                }
            }

            mysql_free_result(res_ptr);
        }
        else
        {
            srs_error("failed. cmd=%s", cmd.str().c_str());
        }

        mysql_close(&my_connection);
    }
    else
    {
        if (mysql_errno(&my_connection))
        {
            srs_error("Connection error %d: %sn",mysql_errno(&my_connection),mysql_error(&my_connection));
        }
    }
}

bool MySqlProcess::select_tb_usr_charge_left_single(const char *db, const std::string &usr, std::string &moneyleft)
{
    std::stringstream cmd;
    cmd << "select usr, charge from tb_usr_charge_left where usr=\"" << usr << "\"";

    MYSQL my_connection;
    mysql_init(&my_connection);

    if (mysql_real_connect(&my_connection, dbip_.c_str(), usr_.c_str(), passwd_.c_str(), db, 0,NULL,0))
    {
        int res = mysql_query(&my_connection, cmd.str().c_str());
        if (!res)
        {
            //get data to vector.
            MYSQL_ROW sqlrow;
            MYSQL_RES *res_ptr;
            res_ptr=mysql_store_result(&my_connection);

            if(res_ptr) {
                int column = mysql_num_fields(res_ptr);
                while((sqlrow=mysql_fetch_row(res_ptr))) {
                    for (int j = 0; j < column; ++j) {
                        printf("[%d]=%s |", j, sqlrow[j]);
                        switch (j)
                        {
                        case 1:
                        {
                            moneyleft = sqlrow[j];
                        }
                            break;
                        default:
                        {
                            ;
                        }
                        }
                    }
                }

                if (mysql_errno(&my_connection))  {
                    srs_error("Retrive error:%s\n",mysql_error(&my_connection));
                }
            }

            mysql_free_result(res_ptr);
        }
        else
        {
            srs_error("failed. cmd=%s", cmd.str().c_str());
        }

        mysql_close(&my_connection);
    }
    else
    {
        if (mysql_errno(&my_connection))
        {
            srs_error("Connection error %d: %sn",mysql_errno(&my_connection),mysql_error(&my_connection));
            return false;
        }
    }

    return true;
}

bool MySqlProcess::insert_tb_usr_charge(const char*db, const std::string &usr, const std::string &money)
{
    std::stringstream cmd;
    cmd << "insert into tb_usr_charge_left (usr, charge) values(\""
        << usr << "\"," << money << ");";

    MYSQL my_connection;
    do_connect(my_connection, db, cmd.str().c_str());
}

bool MySqlProcess::insert_tb_usr_charge_history(const char *db, const std::string &usr, const std::string &money)
{
    std::stringstream timenow;
    get_local_system_time(timenow);
    std::stringstream cmd;
    cmd << "insert into tb_user_charge_history (usr, charge, time) values(\""
        << usr << "\"," << money << "," << "\"" <<timenow.str().c_str() << "\");";

    MYSQL my_connection;
    do_connect(my_connection, db, cmd.str().c_str());
}

bool MySqlProcess::update_tb_usr_charge_left(const char *db, std::string usr, double charge)
{
    std::stringstream cmd;
    cmd << "update tb_usr_charge_left" << " set charge=" << charge << " where usr=\"" << usr << "\";";

    MYSQL my_connection;
    mysql_init(&my_connection);
    if (mysql_real_connect(&my_connection, dbip_.c_str(), usr_.c_str(), passwd_.c_str(), db, 0,NULL,0))
    {
        int res = mysql_query(&my_connection, cmd.str().c_str());
        if (!res)
        {
        }
        else
        {
            srs_error("failed. cmd=%s", cmd.str().c_str());
        }

        mysql_close(&my_connection);
    }
    else
    {
        if (mysql_errno(&my_connection))
        {
            srs_error("Connection error %d: %sn",mysql_errno(&my_connection),mysql_error(&my_connection));
        }
    }
}

