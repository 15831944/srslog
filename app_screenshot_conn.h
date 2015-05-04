#pragma once

#include "app_conn.h"
#include "app_reload_handler.h"
#include "app_st_socket.h"
#include "app_screen_shot_server.h"

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

private:
    SrsStSocket* skt;
};
