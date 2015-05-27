#pragma once
#include "app_protocol_reader_writer.h"

class SrsScreenShotProtocal
{
private:
    /**
    * underlayer socket object, send/recv bytes.
    */
    ISrsProtocolReaderWriter* skt;
public:
    SrsScreenShotProtocal(ISrsProtocolReaderWriter *io);
    virtual ~SrsScreenShotProtocal();
public:
    /**
    * set/get the recv timeout in us.
    * if timeout, recv/send message return ERROR_SOCKET_TIMEOUT.
    */
    virtual void set_recv_timeout(int64_t timeout_us);
    virtual int64_t get_recv_timeout();
    /**
    * set/get the send timeout in us.
    * if timeout, recv/send message return ERROR_SOCKET_TIMEOUT.
    */
    virtual void set_send_timeout(int64_t timeout_us);
    virtual int64_t get_send_timeout();
    /**
    * get recv/send bytes.
    */
    virtual int64_t get_recv_bytes();
    virtual int64_t get_send_bytes();
};

class SrsProxyServer
{
private:
    ISrsProtocolReaderWriter* io;
    SrsScreenShotProtocal *protocol;
public:
    SrsProxyServer(ISrsProtocolReaderWriter *skt);
    virtual ~SrsProxyServer();
public:
    /**
    * set/get the recv timeout in us.
    * if timeout, recv/send message return ERROR_SOCKET_TIMEOUT.
    */
    virtual void set_recv_timeout(int64_t timeout_us);
    virtual int64_t get_recv_timeout();

    /**
   * set/get the send timeout in us.
   * if timeout, recv/send message return ERROR_SOCKET_TIMEOUT.
   */
    virtual void set_send_timeout(int64_t timeout_us);
    virtual int64_t get_send_timeout();
};
