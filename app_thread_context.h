#pragma once
#include <map>

// the context for multiple clients.
class ISrsThreadContext
{
public:
    ISrsThreadContext();
    virtual ~ISrsThreadContext();
public:
    virtual void generate_id();
    virtual int get_id();
};

/**
* st thread context, get_id will get the st-thread id,
* which identify the client.
*/
class SrsThreadContext : public ISrsThreadContext
{
private:
    std::map<st_thread_t, int> cache;
public:
    SrsThreadContext();
    virtual ~SrsThreadContext();
public:
    virtual void generate_id();
    virtual int get_id();
};

extern ISrsThreadContext* _srs_context;
