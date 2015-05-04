#pragma once
#include <string>

#include "app_thread.h"
#include "app_kbps.h"

class SrsServer;

/**
* srs connection type.
*/
enum SrsConnType
{
    SrsConnUnknown = 0,
    SrsConnRtmp,
    SrsConnHttpConn,
    SrsConnHttpApi,
    SrsConnInquiry,
    SrsConnAccount,
};

/**
* the basic connection of SRS,
* all connections accept from listener must extends from this base class,
* server will add the connection to manager, and delete it when remove.
*/
class SrsConnection : public virtual ISrsThreadHandler, public virtual IKbpsDelta
{
private:
    /**
    * each connection start a green thread,
    * when thread stop, the connection will be delete by server.
    */
    SrsThread* pthread;
protected:
    /**
    * the server object to manage the connection.
    */
    SrsServer* server;
    /**
    * the underlayer st fd handler.
    */
    st_netfd_t stfd;
    /**
    * the ip of client.
    */
    std::string ip;
public:
    SrsConnType type_;

public:
    SrsConnection(SrsServer* srs_server, st_netfd_t client_stfd);
    virtual ~SrsConnection();
public:
    /**
    * start the client green thread.
    * when server get a client from listener,
    * 1. server will create an concrete connection(for instance, RTMP connection),
    * 2. then add connection to its connection manager,
    * 3. start the client thread by invoke this start()
    * when client cycle thread stop, invoke the on_thread_stop(), which will use server
    * to remove the client by server->remove(this).
    */
    virtual int start();
    /**
    * the thread cycle function,
    * when serve connection completed, terminate the loop which will terminate the thread,
    * thread will invoke the on_thread_stop() when it terminated.
    */
    virtual int cycle();
    /**
    * when the thread cycle finished, thread will invoke the on_thread_stop(),
    * which will remove self from server, server will remove the connection from manager
    * then delete the connection.
    */
    virtual void on_thread_stop();
public:
    /**
    * when server to get the kbps of connection,
    * it cannot wait the connection terminated then get the kbps,
    * it must sample the kbps every some interval, for instance, 9s to sample all connections kbps,
    * all connections will extends from IKbpsDelta which provides the bytes delta,
    * while the delta must be update by the sample which invoke by the kbps_resample().
    */
    virtual void kbps_resample() = 0;
protected:
    /**
    * for concrete connection to do the cycle.
    */
    virtual int do_cycle() = 0;
private:
    /**
    * when delete the connection, stop the connection,
    * close the underlayer socket, delete the thread.
    */
    virtual void stop();
};
