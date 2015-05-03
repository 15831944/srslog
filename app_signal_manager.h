#pragma once
#include "app_thread.h"

// signal defines.
#define SIGNAL_RELOAD SIGHUP

class SrsServer;
/**
* convert signal to io,
* @see: st-1.9/docs/notes.html
*/
class SrsSignalManager : public ISrsThreadHandler
{
private:
    /* Per-process pipe which is used as a signal queue. */
    /* Up to PIPE_BUF/sizeof(int) signals can be queued up. */
    int sig_pipe[2];
    st_netfd_t signal_read_stfd;
private:
    SrsServer* _server;
    SrsThread* pthread;
public:
    SrsSignalManager(SrsServer* server);
    virtual ~SrsSignalManager();
public:
    virtual int initialize();
    virtual int start();
// interface ISrsThreadHandler.
public:
    virtual int cycle();
private:
    // global singleton instance
    static SrsSignalManager* instance;
    /* Signal catching function. */
    /* Converts signal event to I/O event. */
    static void sig_catcher(int signo);
};
