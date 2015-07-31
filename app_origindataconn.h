#pragma once

#include "app_conn.h"
#include "app_reload_handler.h"
#include "app_st_socket.h"
#include "app_charge_server.h"
#include "app_json.h"
#include "charge_pack_type.h"
#include <stdint.h>
#include <vector>
#include "redis_process.h"
#include <map>
#include "app_origindataserver.h"

//the timeout to wait client data,
//if timeout, close the connection.
#define SRS_CONSTS_ORIGINDATA_SEND_TIMEOUT_US (int64_t)(30*1000*1000LL)

// the timeout to send data to client,
// if timeout, close the connection.
#define SRS_CONSTS_ORIGINDATA_RECV_TIMEOUT_US (int64_t)(30*1000*1000LL)

class SrsOriginDataConn : public virtual SrsConnection, public virtual ISrsReloadHandler
{
private:
    SrsOriginDataServer *dataserver;
public:
    SrsOriginDataConn(SrsServer* srs_server, st_netfd_t client_stfd);
    virtual ~SrsOriginDataConn();

public:
    virtual void kbps_resample();
    // interface IKbpsDelta
public:
    virtual int64_t get_send_bytes_delta();
    virtual int64_t get_recv_bytes_delta();
protected:
    virtual int do_cycle();
private:
    void handle_client_data(std::string , std::string &);
private:
    SrsStSocket* skt;
};
