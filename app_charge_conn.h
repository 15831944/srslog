#pragma once

#include "app_conn.h"
#include "app_reload_handler.h"
#include "app_st_socket.h"
#include "app_charge_server.h"
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

class SrsChargeConn : public virtual SrsConnection, public virtual ISrsReloadHandler
{
private:
    SrsChargeServer *proxy;
public:
    SrsChargeConn(SrsServer* srs_server, st_netfd_t client_stfd);
    virtual ~SrsChargeConn();
public:
    virtual void kbps_resample();
    // interface IKbpsDelta
public:
    virtual int64_t get_send_bytes_delta();
    virtual int64_t get_recv_bytes_delta();
protected:
    virtual int do_cycle();
private:
    bool handle_json_data(std::string data);
private:
    SrsStSocket* skt;

};
