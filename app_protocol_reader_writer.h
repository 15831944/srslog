#pragma once
#include "app_protocal_statistic.h"
#include <stddef.h>
#include <sys/uio.h>
#include "app_kernel_buffer.h"

/**
* the system io reader/writer architecture:
+---------------+     +--------------------+      +---------------+
| IBufferReader |     |    IStatistic      |      | IBufferWriter |
+---------------+     +--------------------+      +---------------+
| + read()      |     | + get_recv_bytes() |      | + write()     |
+------+--------+     | + get_recv_bytes() |      | + writev()    |
      / \             +---+--------------+-+      +-------+-------+
       |                 / \            / \              / \
       |                  |              |                |
+------+------------------+-+      +-----+----------------+--+
| IProtocolReader           |      | IProtocolWriter         |
+---------------------------+      +-------------------------+
| + readfully()             |      | + set_send_timeout()    |
| + set_recv_timeout()      |      +-------+-----------------+
+------------+--------------+             / \
            / \                            |
             |                             |
          +--+-----------------------------+-+
          |       IProtocolReaderWriter      |
          +----------------------------------+
          | + is_never_timeout()             |
          +----------------------------------+
*/

/**
* the writer for the buffer to write to whatever channel.
*/
class ISrsBufferWriter
{
public:
    ISrsBufferWriter();
    virtual ~ISrsBufferWriter();
// for protocol
public:
    /**
    * write bytes over writer.
    * @nwrite the actual written bytes. NULL to ignore.
    */
    virtual int write(void* buf, size_t size, ssize_t* nwrite) = 0;
    /**
    * write iov over writer.
    * @nwrite the actual written bytes. NULL to ignore.
    */
    virtual int writev(const iovec *iov, int iov_size, ssize_t* nwrite) = 0;
};

/**
* the reader for the protocol to read from whatever channel.
*/
class ISrsProtocolReader : public virtual ISrsBufferReader, public virtual ISrsProtocolStatistic
{
public:
    ISrsProtocolReader();
    virtual ~ISrsProtocolReader();
// for protocol
public:
    /**
    * set the recv timeout in us, recv will error when timeout.
    * @remark, if not set, use ST_UTIME_NO_TIMEOUT, never timeout.
    */
    virtual void set_recv_timeout(int64_t timeout_us) = 0;
    /**
    * get the recv timeout in us.
    */
    virtual int64_t get_recv_timeout() = 0;
// for handshake.
public:
    /**
    * read specified size bytes of data
    * @param nread, the actually read size, NULL to ignore.
    */
    virtual int read_fully(void* buf, size_t size, ssize_t* nread) = 0;
};

/**
* the writer for the protocol to write to whatever channel.
*/
class ISrsProtocolWriter : public virtual ISrsBufferWriter, public virtual ISrsProtocolStatistic
{
public:
    ISrsProtocolWriter();
    virtual ~ISrsProtocolWriter();
// for protocol
public:
    /**
    * set the send timeout in us, send will error when timeout.
    * @remark, if not set, use ST_UTIME_NO_TIMEOUT, never timeout.
    */
    virtual void set_send_timeout(int64_t timeout_us) = 0;
    /**
    * get the send timeout in us.
    */
    virtual int64_t get_send_timeout() = 0;
};

/**
* the reader and writer.
*/
class ISrsProtocolReaderWriter : public virtual ISrsProtocolReader, public virtual ISrsProtocolWriter
{
public:
    ISrsProtocolReaderWriter();
    virtual ~ISrsProtocolReaderWriter();
// for protocol
public:
    /**
    * whether the specified timeout_us is never timeout.
    */
    virtual bool is_never_timeout(int64_t timeout_us) = 0;
};
