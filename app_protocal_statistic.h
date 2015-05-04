#pragma once
#include <stdint.h>

/**
* get the statistic of channel.
*/
class ISrsProtocolStatistic
{
public:
    ISrsProtocolStatistic();
    virtual ~ISrsProtocolStatistic();
// for protocol
public:
    /**
    * get the total recv bytes over underlay fd.
    */
    virtual int64_t get_recv_bytes() = 0;
    /**
    * get the total send bytes over underlay fd.
    */
    virtual int64_t get_send_bytes() = 0;
};
