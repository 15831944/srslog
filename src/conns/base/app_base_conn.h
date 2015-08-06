#pragma once

#include "app_conn.h"
#include "app_reload_handler.h"
#include "../base/app_st_socket.h"
#include "app_base_server.h"
#include <stdint.h>
#include <vector>
#include <map>
#include <string>

//the timeout to wait client data,
//if timeout, close the connection.
#define SRS_CONSTS_ACCOUNT_SEND_TIMEOUT_US (int64_t)(30*1000*1000LL)

// the timeout to send data to client,
// if timeout, close the connection.
#define SRS_CONSTS_ACCOUNT_RECV_TIMEOUT_US (int64_t)(30*1000*1000LL)

class SrsBaseConn : public virtual SrsConnection, public virtual ISrsReloadHandler
{
private:
    SrsBaseServer *proxy;
public:
    SrsBaseConn(SrsServer* srs_server, st_netfd_t client_stfd);
    virtual ~SrsBaseConn();
public:
    virtual void kbps_resample();
    // interface IKbpsDelta
public:
    virtual int64_t get_send_bytes_delta();
    virtual int64_t get_recv_bytes_delta();
protected:
    virtual int do_cycle();
private:

private:
    SrsStSocket* skt;
};
