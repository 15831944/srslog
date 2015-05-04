#pragma once

#include <vector>
#include "app_reload_handler.h"
#include "app_conn.h"
#include "app_listener.h"
#include "app_kbps.h"
#include "app_signal_manager.h"

/**
* SRS RTMP server, initialize and listen,
* start connection service thread, destroy client.
*/
class SrsServer : public ISrsReloadHandler
{
private:
#ifdef SRS_AUTO_HTTP_API
    SrsHttpHandler* http_api_handler;
#endif
#ifdef SRS_AUTO_HTTP_SERVER
    SrsHttpHandler* http_stream_handler;
#endif
#ifdef SRS_AUTO_HTTP_PARSER
    SrsHttpHeartbeat* http_heartbeat;
#endif
#ifdef SRS_AUTO_INGEST
    SrsIngester* ingester;
#endif
private:
    /**
    * the pid file fd, lock the file write when server is running.
    * @remark the init.d script should cleanup the pid file, when stop service,
    *       for the server never delete the file; when system startup, the pid in pid file
    *       maybe valid but the process is not SRS, the init.d script will never start server.
    */
    int pid_fd;
    /**
    * all connections, connection manager
    */
    std::vector<SrsConnection*> conns;
    /**
    * all listners, listener manager.
    */
    std::vector<SrsListener*> listeners;
    /**
    * signal manager which convert gignal to io message.
    */
    SrsSignalManager* signal_manager;
    /**
    * server total kbps.
    */
    SrsKbps* kbps;
    /**
    * user send the signal, convert to variable.
    */
    bool signal_reload;
    bool signal_gmc_stop;
public:
    SrsServer();
    virtual ~SrsServer();
public:
    /**
    * the destroy is for gmc to analysis the memory leak,
    * if not destroy global/static data, the gmc will warning memory leak.
    * in service, server never destroy, directly exit when restart.
    */
    virtual void destroy();
// server startup workflow, @see run_master()
public:
    virtual int initialize();
    virtual int initialize_signal();
    virtual int acquire_pid_file();
    virtual int initialize_st();
    virtual int listen();
    virtual int register_signal();
    virtual int ingest();
    virtual int cycle();
// server utility
public:
    /**
    * callback for connection to remove itself.
    * when connection thread cycle terminated, callback this to delete connection.
    * @see SrsConnection.on_thread_stop().
    */
    virtual void remove(SrsConnection* conn);
    /**
    * callback for signal manager got a signal.
    * the signal manager convert signal to io message,
    * whatever, we will got the signo like the orignal signal(int signo) handler.
    * @remark, direclty exit for SIGTERM.
    * @remark, do reload for SIGNAL_RELOAD.
    * @remark, for SIGINT and SIGUSR2:
    *       no gmc, directly exit.
    *       for gmc, set the variable signal_gmc_stop, the cycle will return and cleanup for gmc.
    */
    virtual void on_signal(int signo);
private:
    /**
    * the server thread main cycle,
    * update the global static data, for instance, the current time,
    * the cpu/mem/network statistic.
    */
    virtual int do_cycle();
    /**
    * listen at specified protocol.
    */
    virtual int listen_rtmp();
    virtual int listen_http_api();
    virtual int listen_http_stream();
    /**
    * close the listeners for specified type,
    * remove the listen object from manager.
    */
    virtual void close_listeners(SrsListenerType type);
    /**
    * resample the server kbps.
    * if conn is NULL, resample all connections delta, then calc the total kbps.
    * @param conn, the connection to do resample the kbps. NULL to resample all connections.
    * @param do_resample, whether resample the server kbps. always false when sample a connection.
    */
    virtual void resample_kbps(SrsConnection* conn, bool do_resample = true);
// internal only
public:
    /**
    * when listener got a fd, notice server to accept it.
    * @param type, the client type, used to create concrete connection,
    *       for instance RTMP connection to serve client.
    * @param client_stfd, the client fd in st boxed, the underlayer fd.
    */
    virtual int accept_client(SrsListenerType type, st_netfd_t client_stfd);
// interface ISrsThreadHandler.
public:
    virtual int on_reload_listen();
    virtual int on_reload_pid();
    virtual int on_reload_vhost_added(std::string vhost);
    virtual int on_reload_vhost_removed(std::string vhost);
    virtual int on_reload_vhost_http_updated();
    virtual int on_reload_http_api_enabled();
    virtual int on_reload_http_api_disabled();
    virtual int on_reload_http_stream_enabled();
    virtual int on_reload_http_stream_disabled();
    virtual int on_reload_http_stream_updated();
};
