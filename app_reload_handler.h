#pragma once

#include <string>
/**
* the handler for config reload.
* when reload callback, the config is updated yet.
*
* features not support reload,
* @see: https://github.com/winlinvip/simple-rtmp-server/wiki/v1_CN_Reload#notsupportedfeatures
*/
class ISrsReloadHandler
{
public:
    ISrsReloadHandler();
    virtual ~ISrsReloadHandler();
public:
    virtual int on_reload_listen();
    virtual int on_reload_pid();
    virtual int on_reload_log_tank();
    virtual int on_reload_log_level();
    virtual int on_reload_log_file();
    virtual int on_reload_pithy_print();
    virtual int on_reload_http_api_enabled();
    virtual int on_reload_http_api_disabled();
    virtual int on_reload_http_stream_enabled();
    virtual int on_reload_http_stream_disabled();
    virtual int on_reload_http_stream_updated();
    virtual int on_reload_vhost_http_updated();
    virtual int on_reload_vhost_added(std::string vhost);
    virtual int on_reload_vhost_removed(std::string vhost);
    virtual int on_reload_vhost_atc(std::string vhost);
    virtual int on_reload_vhost_gop_cache(std::string vhost);
    virtual int on_reload_vhost_queue_length(std::string vhost);
    virtual int on_reload_vhost_time_jitter(std::string vhost);
    virtual int on_reload_vhost_forward(std::string vhost);
    virtual int on_reload_vhost_hls(std::string vhost);
    virtual int on_reload_vhost_dvr(std::string vhost);
    virtual int on_reload_vhost_transcode(std::string vhost);
    virtual int on_reload_ingest_removed(std::string vhost, std::string ingest_id);
    virtual int on_reload_ingest_added(std::string vhost, std::string ingest_id);
    virtual int on_reload_ingest_updated(std::string vhost, std::string ingest_id);
};
