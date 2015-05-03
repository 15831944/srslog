#include "app_conn.h"
#include "app_error_code.h"
#include "app_thread_context.h"
#include "app_utility.h"
#include "app_log.h"
#include "app_server.h"

SrsConnection::SrsConnection(SrsServer* srs_server, st_netfd_t client_stfd)
{
    server = srs_server;
    stfd = client_stfd;

    // the client thread should reap itself,
    // so we never use joinable.
    // TODO: FIXME: maybe other thread need to stop it.
    // @see: https://github.com/winlinvip/simple-rtmp-server/issues/78
    pthread = new SrsThread(this, 0, false);
}

SrsConnection::~SrsConnection()
{
    stop();
}

int SrsConnection::start()
{
    return pthread->start();
}

int SrsConnection::cycle()
{
    int ret = ERROR_SUCCESS;

    _srs_context->generate_id();
    ip = srs_get_peer_ip(st_netfd_fileno(stfd));

    ret = do_cycle();

    // if socket io error, set to closed.
    if (srs_is_client_gracefully_close(ret)) {
        ret = ERROR_SOCKET_CLOSED;
    }

    // success.
    if (ret == ERROR_SUCCESS) {
        srs_trace("client finished.");
    }

    // client close peer.
    if (ret == ERROR_SOCKET_CLOSED) {
        srs_warn("client disconnect peer. ret=%d", ret);
    }

    // set loop to stop to quit.
    pthread->stop_loop();

    return ERROR_SUCCESS;
}

void SrsConnection::on_thread_stop()
{
    // TODO: FIXME: never remove itself, use isolate thread to do cleanup.
    server->remove(this);
}

void SrsConnection::stop()
{
    srs_close_stfd(stfd);
    srs_freep(pthread);
}


