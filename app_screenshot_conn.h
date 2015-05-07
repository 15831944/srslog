#pragma once

#include "app_conn.h"
#include "app_reload_handler.h"
#include "app_st_socket.h"
#include "app_screen_shot_server.h"
#include <stdint.h>
#include <vector>

typedef struct ClientReqData
{
    std::string action;
    std::string app;
    std::string stream;
    std::string file_status;
    std::string time_offset;
}ClientReqData;

//the timeout to wait client data,
//if timeout, close the connection.
#define SRS_CONSTS_ACCOUNT_SEND_TIMEOUT_US (int64_t)(30*1000*1000LL)

// the timeout to send data to client,
// if timeout, close the connection.
#define SRS_CONSTS_ACCOUNT_RECV_TIMEOUT_US (int64_t)(30*1000*1000LL)

class SrsScreenShotConn : public virtual SrsConnection, public virtual ISrsReloadHandler
{
private:
    SrsScreenShotServer *screenshot;
public:
    SrsScreenShotConn(SrsServer* srs_server, st_netfd_t client_stfd);
    virtual ~SrsScreenShotConn();
public:
    virtual void kbps_resample();
    // interface IKbpsDelta
public:
    virtual int64_t get_send_bytes_delta();
    virtual int64_t get_recv_bytes_delta();
protected:
    virtual int do_cycle();
private:
    SrsStSocket* skt;

private:
    void parse_client_data(char *json_data, int len, ClientReqData &screenshotdata);
    void do_screen_shot_job(const ClientReqData &screenshotdata);
    void do_check_vod_file_status(ClientReqData &screenshotdata);
    bool parse_json(char *json_data, int len, ClientReqData &res);
    bool get_tsfile(const char *stream, std::string &file_name);
    bool shot_picture(char *ts_name, char *jpg_name);
    void make_screen_shot_pack(const ClientReqData &data, char *buff_base64, int len_base64, std::stringstream &res);
    void make_file_status_pack(const ClientReqData &data, std::stringstream &res);
};

bool ListDirectoryFile( char *path, std::vector<std::string>& vec_files);
int is_file_exist(const char *file_path);
