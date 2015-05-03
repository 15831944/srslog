#include "app_listener.h"
#include "app_st.h"
#include "app_macros.h"
#include "app_error_code.h"
#include "app_log.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include "app_server.h"

SrsListener::SrsListener(SrsServer* server, SrsListenerType type)
{
    fd = -1;
    stfd = NULL;

    _port = 0;
    _server = server;
    _type = type;

    pthread = new SrsThread(this, 0, true);
}

SrsListener::~SrsListener()
{
    srs_close_stfd(stfd);

    pthread->stop();
    srs_freep(pthread);

    // st does not close it sometimes,
    // close it manually.
    close(fd);
}

SrsListenerType SrsListener::type()
{
    return _type;
}

int SrsListener::listen(int port)
{
    int ret = ERROR_SUCCESS;

    _port = port;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        ret = ERROR_SOCKET_CREATE;
        srs_error("create linux socket error. ret=%d", ret);
        return ret;
    }
    srs_verbose("create linux socket success. fd=%d", fd);

    int reuse_socket = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse_socket, sizeof(int)) == -1) {
        ret = ERROR_SOCKET_SETREUSE;
        srs_error("setsockopt reuse-addr error. ret=%d", ret);
        return ret;
    }
    srs_verbose("setsockopt reuse-addr success. fd=%d", fd);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(fd, (const sockaddr*)&addr, sizeof(sockaddr_in)) == -1) {
        ret = ERROR_SOCKET_BIND;
        srs_error("bind socket error. ret=%d", ret);
        return ret;
    }
    srs_verbose("bind socket success. fd=%d", fd);

    if (::listen(fd, SERVER_LISTEN_BACKLOG) == -1) {
        ret = ERROR_SOCKET_LISTEN;
        srs_error("listen socket error. ret=%d", ret);
        return ret;
    }
    srs_verbose("listen socket success. fd=%d", fd);

    if ((stfd = st_netfd_open_socket(fd)) == NULL){
        ret = ERROR_ST_OPEN_SOCKET;
        srs_error("st_netfd_open_socket open socket failed. ret=%d", ret);
        return ret;
    }
    srs_verbose("st open socket success. fd=%d", fd);

    if ((ret = pthread->start()) != ERROR_SUCCESS) {
        srs_error("st_thread_create listen thread error. ret=%d", ret);
        return ret;
    }
    srs_verbose("create st listen thread success.");

    srs_trace("listen thread cid=%d, current_cid=%d, "
        "listen at port=%d, type=%d, fd=%d started success",
        pthread->cid(), _srs_context->get_id(), _port, _type, fd);

    return ret;
}

void SrsListener::on_thread_start()
{
    srs_trace("listen cycle start, port=%d, type=%d, fd=%d", _port, _type, fd);
}

int SrsListener::cycle()
{
    int ret = ERROR_SUCCESS;

    st_netfd_t client_stfd = st_accept(stfd, NULL, NULL, ST_UTIME_NO_TIMEOUT);

    if(client_stfd == NULL){
        // ignore error.
        srs_error("ignore accept thread stoppped for accept client error");
        return ret;
    }
    srs_verbose("get a client. fd=%d", st_netfd_fileno(client_stfd));

    if ((ret = _server->accept_client(_type, client_stfd)) != ERROR_SUCCESS) {
        srs_warn("accept client error. ret=%d", ret);
        return ret;
    }

    return ret;
}
