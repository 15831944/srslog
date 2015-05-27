#include "app_proxy_server.h"
#include "app_macros.h"

SrsScreenShotProtocal::SrsScreenShotProtocal(ISrsProtocolReaderWriter *io)
{
    skt = io;
}

SrsScreenShotProtocal::~SrsScreenShotProtocal()
{

}

void SrsScreenShotProtocal::set_recv_timeout(int64_t timeout_us)
{
    return skt->set_recv_timeout(timeout_us);
}

int64_t SrsScreenShotProtocal::get_recv_timeout()
{
    return skt->get_recv_timeout();
}

void SrsScreenShotProtocal::set_send_timeout(int64_t timeout_us)
{
    return skt->set_send_timeout(timeout_us);
}

int64_t SrsScreenShotProtocal::get_send_timeout()
{
    return skt->get_send_timeout();
}

int64_t SrsScreenShotProtocal::get_recv_bytes()
{
    return skt->get_recv_bytes();
}

int64_t SrsScreenShotProtocal::get_send_bytes()
{
    return skt->get_send_bytes();
}


SrsProxyServer::SrsProxyServer(ISrsProtocolReaderWriter *skt)
{
    io = skt;
    protocol = new SrsScreenShotProtocal(skt);
}

SrsProxyServer::~SrsProxyServer()
{
    srs_freep(protocol);
}

void SrsProxyServer::set_recv_timeout(int64_t timeout_us)
{
    return protocol->set_recv_timeout(timeout_us);
}

int64_t SrsProxyServer::get_recv_timeout()
{
    return protocol->get_recv_timeout();
}

void SrsProxyServer::set_send_timeout(int64_t timeout_us)
{
    return protocol->set_send_timeout(timeout_us);
}

int64_t SrsProxyServer::get_send_timeout()
{
    return protocol->get_send_timeout();
}
