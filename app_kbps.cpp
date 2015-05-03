#include "app_kbps.h"

IKbpsDelta::IKbpsDelta()
{
}

IKbpsDelta::~IKbpsDelta()
{
}

SrsKbps::SrsKbps()
{
}

SrsKbps::~SrsKbps()
{
}

void SrsKbps::set_io(ISrsProtocolStatistic* in, ISrsProtocolStatistic* out)
{
    // set input stream
    // now, set start time.
    if (is.starttime == 0) {
        is.starttime = srs_get_system_time_ms();
    }
    // save the old in bytes.
    if (is.io.in) {
        is.bytes += is.last_bytes - is.io_bytes_base;
    }
    // use new io.
    is.io.in = in;
    is.last_bytes = is.io_bytes_base = 0;
    if (in) {
        is.last_bytes = is.io_bytes_base = in->get_recv_bytes();
    }
    // resample
    is.sample();

    // set output stream
    // now, set start time.
    if (os.starttime == 0) {
        os.starttime = srs_get_system_time_ms();
    }
    // save the old in bytes.
    if (os.io.out) {
        os.bytes += os.last_bytes - os.io_bytes_base;
    }
    // use new io.
    os.io.out = out;
    os.last_bytes = os.io_bytes_base = 0;
    if (out) {
        os.last_bytes = os.io_bytes_base = out->get_send_bytes();
    }
    // resample
    os.sample();
}

int SrsKbps::get_send_kbps()
{
    int64_t duration = srs_get_system_time_ms() - is.starttime;
    if (duration <= 0) {
        return 0;
    }
    int64_t bytes = get_send_bytes();
    return bytes * 8 / duration;
}

int SrsKbps::get_recv_kbps()
{
    int64_t duration = srs_get_system_time_ms() - os.starttime;
    if (duration <= 0) {
        return 0;
    }
    int64_t bytes = get_recv_bytes();
    return bytes * 8 / duration;
}

int SrsKbps::get_send_kbps_30s()
{
    return os.sample_30s.kbps;
}

int SrsKbps::get_recv_kbps_30s()
{
    return is.sample_30s.kbps;
}

int SrsKbps::get_send_kbps_5m()
{
    return os.sample_5m.kbps;
}

int SrsKbps::get_recv_kbps_5m()
{
    return is.sample_5m.kbps;
}

int64_t SrsKbps::get_send_bytes()
{
    return os.get_total_bytes();
}

int64_t SrsKbps::get_recv_bytes()
{
    return is.get_total_bytes();
}

int64_t SrsKbps::get_send_bytes_delta()
{
    int64_t delta = os.get_total_bytes() - os.delta_bytes;
    os.delta_bytes = os.get_total_bytes();
    return delta;
}

int64_t SrsKbps::get_recv_bytes_delta()
{
    int64_t delta = is.get_total_bytes() - is.delta_bytes;
    is.delta_bytes = is.get_total_bytes();
    return delta;
}

void SrsKbps::add_delta(IKbpsDelta* delta)
{
    srs_assert(delta);

    // update the total bytes
    is.last_bytes += delta->get_recv_bytes_delta();
    os.last_bytes += delta->get_send_bytes_delta();

    // we donot sample, please use sample() to do resample.
}

void SrsKbps::sample()
{
    // update the total bytes
    if (os.io.out) {
        os.last_bytes = os.io.out->get_send_bytes();
    }

    if (is.io.in) {
        is.last_bytes = is.io.in->get_recv_bytes();
    }

    // resample
    is.sample();
    os.sample();
}
