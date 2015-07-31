#include "app_origindataserver.h"

SrsOriginDataProtocal::SrsOriginDataProtocal(ISrsProtocolReaderWriter *io)
{
    skt = io;
}

SrsOriginDataProtocal::~SrsOriginDataProtocal()
{

}


void SrsOriginDataProtocal::set_recv_timeout(int64_t timeout_us)
{
    return skt->set_recv_timeout(timeout_us);
}

int64_t SrsOriginDataProtocal::get_recv_timeout()
{
    return skt->get_recv_timeout();
}

void SrsOriginDataProtocal::set_send_timeout(int64_t timeout_us)
{
    return skt->set_send_timeout(timeout_us);
}

int64_t SrsOriginDataProtocal::get_send_timeout()
{
    return skt->get_send_timeout();
}

int64_t SrsOriginDataProtocal::get_recv_bytes()
{
    return skt->get_recv_bytes();
}

int64_t SrsOriginDataProtocal::get_send_bytes()
{
    return skt->get_send_bytes();
}


SrsOriginDataServer::SrsOriginDataServer(ISrsProtocolReaderWriter *skt)
{
    io = skt;
    protocol = new SrsOriginDataProtocal(skt);
}

SrsOriginDataServer::~SrsOriginDataServer()
{
    srs_freep(protocol);
}

void SrsOriginDataServer::set_recv_timeout(int64_t timeout_us)
{
    return protocol->set_recv_timeout(timeout_us);
}

int64_t SrsOriginDataServer::get_recv_timeout()
{
    return protocol->get_recv_timeout();
}

void SrsOriginDataServer::set_send_timeout(int64_t timeout_us)
{
    return protocol->set_send_timeout(timeout_us);
}

int64_t SrsOriginDataServer::get_send_timeout()
{
    return protocol->get_send_timeout();
}

