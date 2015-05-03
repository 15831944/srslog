#pragma once
#include <stdint.h>

/**
* the interface which provices delta of bytes.
*/
class IKbpsDelta
{
public:
    IKbpsDelta();
    virtual ~IKbpsDelta();
public:
    virtual int64_t get_send_bytes_delta() = 0;
    virtual int64_t get_recv_bytes_delta() = 0;
};

/**
* to statistic the kbps of io.
* itself can be a statistic source, for example, used for SRS bytes stat.
* there are two usage scenarios:
* 1. connections to calc kbps:
*       set_io(in, out)
*       sample()
*       get_xxx_kbps().
*   the connections know how many bytes already send/recv.
* 2. server to calc kbps:
*       set_io(NULL, NULL)
*       for each connection in connections:
*           add_delta(connections) // where connection is a IKbpsDelta*
*       sample()
*       get_xxx_kbps().
*   the server never know how many bytes already send/recv, for the connection maybe closed.
*/
class SrsKbps : public virtual ISrsProtocolStatistic, public virtual IKbpsDelta
{
private:
    SrsKbpsSlice is;
    SrsKbpsSlice os;
public:
    SrsKbps();
    virtual ~SrsKbps();
public:
    /**
    * set io to start new session.
    * set the underlayer reader/writer,
    * if the io destroied, for instance, the forwarder reconnect,
    * user must set the io of SrsKbps to NULL to continue to use the kbps object.
    * @param in the input stream statistic. can be NULL.
    * @param out the output stream statistic. can be NULL.
    * @remark if in/out is NULL, use the cached data for kbps.
    */
    virtual void set_io(ISrsProtocolStatistic* in, ISrsProtocolStatistic* out);
public:
    /**
    * get total kbps, duration is from the startup of io.
    * @remark, use sample() to update data.
    */
    virtual int get_send_kbps();
    virtual int get_recv_kbps();
    // 30s
    virtual int get_send_kbps_30s();
    virtual int get_recv_kbps_30s();
    // 5m
    virtual int get_send_kbps_5m();
    virtual int get_recv_kbps_5m();
public:
    /**
    * get the total send/recv bytes, from the startup of the oldest io.
    * @remark, use sample() to update data.
    */
    virtual int64_t get_send_bytes();
    virtual int64_t get_recv_bytes();
    /**
    * get the delta of send/recv bytes.
    * @remark, used for add_delta to calc the total system bytes/kbps.
    */
    virtual int64_t get_send_bytes_delta();
    virtual int64_t get_recv_bytes_delta();
public:
    /**
    * add delta to kbps clac mechenism.
    * we donot know the total bytes, but know the delta, for instance,
    * for rtmp server to calc total bytes and kbps.
    * @remark user must invoke sample() when invoke this method.
    * @param delta, assert should never be NULL.
    */
    virtual void add_delta(IKbpsDelta* delta);
    /**
    * resample all samples, ignore if in/out is NULL.
    * used for user to calc the kbps, to sample new kbps value.
    * @remark if user, for instance, the rtmp server to calc the total bytes,
    *       use the add_delta() is better solutions.
    */
    virtual void sample();
};
