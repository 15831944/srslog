#include "config_info.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "app_macros.h"
#include "../json/json_process.h"

const char* CONST_KEYS[] = { "listen",
                             "max_connections",
                             "daemon",
                             "log_tank",
                             "log_level",
                             "log_file",
                             "pid_file"
                           };

ConfigInfo::ConfigInfo()
{

}

ConfigInfo::~ConfigInfo()
{

}

int ConfigInfo::load_configfile(const char *file)
{
    if (NULL == file) {
        return ERROR_SYSTEM_FILE_OPENE;
    }

    FILE *pf = fopen(file, "rb");
    if (NULL == pf) {
        return ERROR_SYSTEM_FILE_OPENE;
    }

    enum {BUFFSIZE = 1024};
    char buff[BUFFSIZE] = {0};
    while (1) {
        bzero(buff, BUFFSIZE);
        int nread = fread(buff, 1, BUFFSIZE, pf);
        if (nread <= 0) {
            break;
        }

        json.append(buff, nread);
    }

    if (NULL != pf) {
        fclose(pf);
    }

    return ERROR_SUCCESS;
}

int ConfigInfo::parse_and_check()
{
    std::vector<std::string> keys;

    for (int i = 0; i < sizeof(CONST_KEYS)/sizeof(CONST_KEYS[0]); ++i) {
        std::string tmp = CONST_KEYS[i];
        keys.push_back(tmp);
    }

    parse_json_no_array(json.c_str(), keys, json_map);

    if (json_map.size() <= 0) {
        return ERROR_SYSTEM_CONFIG_INVALID;
    }

    return ERROR_SUCCESS;
}

void ConfigInfo::get_value(std::string key, std::string &value) const
{
    for (std::map<std::string, std::string>::const_iterator iter = json_map.begin();
         iter != json_map.end();
         ++iter) {
        if (iter->first == key) {
            value = iter->second;
            break;
        }
    }
}

bool ConfigInfo::get_daemon() const
{
    std::string res;
    get_value(CONF_DAEMON, res);

    if (res == "on") {
        return true;
    }

    if (res == "off") {
        return false;
    }

    return true;
}

bool ConfigInfo::get_log_tank_file() const
{
    std::string res;
    get_value(CONF_LOG_TANK, res);

    if (res == "file") {
        return true;
    }

    if (res == "console") {
        return false;
    }

    return true;
}

std::string ConfigInfo::get_log_level() const
{
    std::string res;
    get_value(CONF_LOG_LEVEL, res);

    return res;
}

std::string ConfigInfo::get_log_file_name() const
{
    std::string res;
    get_value(CONF_LOG_FILE, res);

    return res;
}

std::string ConfigInfo::get_pid_file() const
{
    std::string res;
    get_value(CONF_PID_FILE, res);

    return res;
}

int ConfigInfo::get_max_connections() const
{
    std::string res;
    get_value(CONF_MAX_CONNECTIONS, res);

    return atoi(res.c_str());
}

int64_t ConfigInfo::get_heartbeat_interval() const
{
    return (int64_t)(SRS_CONF_DEFAULT_HTTP_HEAETBEAT_INTERVAL * 1000);
}

std::vector<std::string> ConfigInfo::get_listen() const
{
    std::vector<std::string> ports;
    std::string res;
    get_value(CONF_LISTEN, res);

    ports.push_back(res);
    return ports;
}

ConfigInfo* g_config = new ConfigInfo();

