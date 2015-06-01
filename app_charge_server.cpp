#include "app_charge_server.h"
#include "app_macros.h"

SrsChargeProtocal::SrsChargeProtocal(ISrsProtocolReaderWriter *io)
{
    skt = io;
}

SrsChargeProtocal::~SrsChargeProtocal()
{

}

void SrsChargeProtocal::set_recv_timeout(int64_t timeout_us)
{
    return skt->set_recv_timeout(timeout_us);
}

int64_t SrsChargeProtocal::get_recv_timeout()
{
    return skt->get_recv_timeout();
}

void SrsChargeProtocal::set_send_timeout(int64_t timeout_us)
{
    return skt->set_send_timeout(timeout_us);
}

int64_t SrsChargeProtocal::get_send_timeout()
{
    return skt->get_send_timeout();
}

int64_t SrsChargeProtocal::get_recv_bytes()
{
    return skt->get_recv_bytes();
}

int64_t SrsChargeProtocal::get_send_bytes()
{
    return skt->get_send_bytes();
}


SrsChargeServer::SrsChargeServer(ISrsProtocolReaderWriter *skt)
{
    io = skt;
    protocol = new SrsChargeProtocal(skt);
}

SrsChargeServer::~SrsChargeServer()
{
    srs_freep(protocol);
}

void SrsChargeServer::set_recv_timeout(int64_t timeout_us)
{
    return protocol->set_recv_timeout(timeout_us);
}

int64_t SrsChargeServer::get_recv_timeout()
{
    return protocol->get_recv_timeout();
}

void SrsChargeServer::set_send_timeout(int64_t timeout_us)
{
    return protocol->set_send_timeout(timeout_us);
}

int64_t SrsChargeServer::get_send_timeout()
{
    return protocol->get_send_timeout();
}
