#include "app_thread_context.h"

ISrsThreadContext::ISrsThreadContext()
{
}

ISrsThreadContext::~ISrsThreadContext()
{
}

void ISrsThreadContext::generate_id()
{
}

int ISrsThreadContext::get_id()
{
    return 0;
}

SrsThreadContext::SrsThreadContext()
{
}

SrsThreadContext::~SrsThreadContext()
{
}

void SrsThreadContext::generate_id()
{
    static int id = 100;
    cache[st_thread_self()] = id++;
}

int SrsThreadContext::get_id()
{
    return cache[st_thread_self()];
}

ISrsThreadContext* _srs_context = new SrsThreadContext();
