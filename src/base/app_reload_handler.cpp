#include "app_reload_handler.h"
#include "app_error_code.h"

using namespace std;

ISrsReloadHandler::ISrsReloadHandler()
{
}

ISrsReloadHandler::~ISrsReloadHandler()
{
}

int ISrsReloadHandler::on_reload_listen()
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_pid()
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_log_tank()
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_log_level()
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_log_file()
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_pithy_print()
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_http_api_enabled()
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_http_api_disabled()
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_http_stream_enabled()
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_http_stream_disabled()
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_http_stream_updated()
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_vhost_http_updated()
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_vhost_added(string /*vhost*/)
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_vhost_removed(string /*vhost*/)
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_vhost_atc(string /*vhost*/)
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_vhost_gop_cache(string /*vhost*/)
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_vhost_queue_length(string /*vhost*/)
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_vhost_time_jitter(string /*vhost*/)
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_vhost_forward(string /*vhost*/)
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_vhost_hls(string /*vhost*/)
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_vhost_dvr(string /*vhost*/)
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_vhost_transcode(string /*vhost*/)
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_ingest_removed(string /*vhost*/, string /*ingest_id*/)
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_ingest_added(string /*vhost*/, string /*ingest_id*/)
{
    return ERROR_SUCCESS;
}

int ISrsReloadHandler::on_reload_ingest_updated(string /*vhost*/, string /*ingest_id*/)
{
    return ERROR_SUCCESS;
}

