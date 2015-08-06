#pragma once
#include <stdint.h>
#include "app_protocol_reader_writer.h"
#include "app_st.h"

/**
* the socket provides TCP socket over st,
* that is, the sync socket mechanism.
*/
class SrsStSocket : public ISrsProtocolReaderWriter
{
private:
    int64_t recv_timeout;
    int64_t send_timeout;
    int64_t recv_bytes;
    int64_t send_bytes;
    st_netfd_t stfd;
public:
    SrsStSocket(st_netfd_t client_stfd);
    virtual ~SrsStSocket();
public:
    virtual bool is_never_timeout(int64_t timeout_us);
    virtual void set_recv_timeout(int64_t timeout_us);
    virtual int64_t get_recv_timeout();
    virtual void set_send_timeout(int64_t timeout_us);
    virtual int64_t get_send_timeout();
    virtual int64_t get_recv_bytes();
    virtual int64_t get_send_bytes();
public:
    /**
    * @param nread, the actual read bytes, ignore if NULL.
    */
    virtual int read(void* buf, size_t size, ssize_t* nread);
    virtual int read_fully(void* buf, size_t size, ssize_t* nread);
    /**
    * @param nwrite, the actual write bytes, ignore if NULL.
    */
    virtual int write(void* buf, size_t size, ssize_t* nwrite);
    virtual int writev(const iovec *iov, int iov_size, ssize_t* nwrite);
};
