#pragma once

#include <string>
#include <map>
#include <stdint.h>
#include <vector>
#include <app_error_code.h>

#define CONF_LISTEN "listen"
#define CONF_MAX_CONNECTIONS "max_connections"
#define CONF_DAEMON "daemon"
#define CONF_LOG_TANK "log_tank"
#define CONF_LOG_LEVEL "log_level"
#define CONF_LOG_FILE "log_file"
#define CONF_PID_FILE "pid_file"
#define CONF_REDIS_PORT "redis_port"
#define CONF_HLS_PATH   "hls_path"

class ConfigInfo
{
public:
    ConfigInfo();
    ~ConfigInfo();

public:
    int load_configfile(const char* file);
    int parse_and_check();

    void get_value(std::string key, std::string &value) const;
    bool get_daemon() const;
    bool get_log_tank_file() const;
    std::string  get_log_level() const;
    std::string get_log_file_name() const;
    std::string get_pid_file() const;
    int get_max_connections() const;
    int64_t get_heartbeat_interval() const;
    std::vector<std::string> get_listen() const;
    int get_redis_port() const;
    std::string get_hls_path() const;
private:
    std::string json;
    std::map<std::string, std::string> json_map;
private:
    ConfigInfo(const ConfigInfo&);
    ConfigInfo & operator = (const ConfigInfo&);
};

extern ConfigInfo* g_config;
