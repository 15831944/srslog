#pragma once
#include "app_thread.h"
// listener type for server to identify the connection,
// that is, use different type to process the connection.
enum SrsListenerType
{
    // RTMP client,
    SrsListenerRtmpStream   = 0,
    // HTTP api,
    SrsListenerHttpApi      = 1,
    // HTTP stream, HDS/HLS/DASH
    SrsListenerHttpStream   = 2,
    // Screen Shot service.
    SrsListenerScreenShot,
};

#define SERVER_LISTEN_BACKLOG 512

class SrsServer;
class SrsListener : public ISrsThreadHandler
{
public:
    SrsListenerType _type;
private:
    int fd;
    st_netfd_t stfd;
    int _port;
    SrsServer* _server;
    SrsThread* pthread;
public:
    SrsListener(SrsServer* server, SrsListenerType type);
    virtual ~SrsListener();
public:
    virtual SrsListenerType type();
    virtual int listen(int port);
// interface ISrsThreadHandler.
public:
    virtual void on_thread_start();
    virtual int cycle();
};
