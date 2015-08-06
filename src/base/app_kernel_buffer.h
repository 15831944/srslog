#pragma once
#include <stddef.h>
#include <sys/uio.h>

/**
* the reader for the buffer to read from whatever channel.
*/
class ISrsBufferReader
{
public:
    ISrsBufferReader();
    virtual ~ISrsBufferReader();
// for protocol/amf0/msg-codec
public:
    virtual int read(void* buf, size_t size, ssize_t* nread) = 0;
};
