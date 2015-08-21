#pragma once

#include "app_conn.h"
#include "app_reload_handler.h"
#include "../base/app_st_socket.h"
#include "app_record_server.h"
#include <stdint.h>
#include <vector>
#include <map>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include "video_record.h"
#include "mutexlock.h"

//the timeout to wait client data,
//if timeout, close the connection.
#define SRS_CONSTS_ACCOUNT_SEND_TIMEOUT_US (int64_t)(30*1000*1000LL)

// the timeout to send data to client,
// if timeout, close the connection.
#define SRS_CONSTS_ACCOUNT_RECV_TIMEOUT_US (int64_t)(30*1000*1000LL)

#define CRNL "\r\n"

#define JSON_TYPE "type"
#define JSON_STREAM "stream"
#define JSON_PUBLISHER "publisher"
#define JSON_FILENAME "filename"
#define JSON_TIMEOUT  "timeout"
#define JSON_RET      "ret"

#define TYPE_START_RECORD   1000
#define TYPE_STOP_RECORD    1001
#define TYPE_DELETE_RECORD  1002

enum {
    RET_CODE_SUCCESS                       = 0,
    RET_FILE_FORMAT_NOT_MP4,
    RET_CODE_HAS_RECORDING,
    RET_CODE_STOPRECORD_FAILED,
    RET_CODE_INSERT_REDIS_FAILED ,
    RET_CODE_PONIT_NULL,
    RET_CODE_INIT_RECORD_FAILED,
    RET_INPUTPARAMS_ERROR,
};

class SrsRecordConn : public virtual SrsConnection, public virtual ISrsReloadHandler
{
private:
    SrsRecordServer *proxy;
public:
    SrsRecordConn(SrsServer* srs_server, st_netfd_t client_stfd);
    virtual ~SrsRecordConn();
public:
    virtual void kbps_resample();
    // interface IKbpsDelta
public:
    virtual int64_t get_send_bytes_delta();
    virtual int64_t get_recv_bytes_delta();
protected:
    virtual int do_cycle();
private:
    void handle_client_data(const std::string &data);
    int handle_start_record(std::string stream, std::string publisher, std::string file, std::string timeout);
    int handle_stop_record(std::string stream, std::string publisher, std::string file);
    int handle_delete_record(std::string stream, std::string publisher, std::string file);
private:
    //for start record.
    bool ask_if_recording(const std::string &key);
    bool insert_record_redis(std::string key, std::string stream, std::string publisher, std::string timeout);
    int do_start_record(std::string key, std::string stream, std::string publisher);
    int send_client(const std::string &res);
private:
    SrsStSocket* skt;
};

void pack_ret_start_record(int ret, std::string &res);
void pack_ret_stop_record(int ret, std::string &res);
void pack_ret_delete_record(int ret, std::string &res);
