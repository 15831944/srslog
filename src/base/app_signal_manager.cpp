#include "app_signal_manager.h"
#include "app_log.h"
#include "app_st.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "app_server.h"

SrsSignalManager *SrsSignalManager::instance = NULL;

SrsSignalManager::SrsSignalManager(SrsServer* server)
{
    SrsSignalManager::instance = this;

    _server = server;
    sig_pipe[0] = sig_pipe[1] = -1;
    pthread = new SrsThread(this, 0, true);
    signal_read_stfd = NULL;
}

SrsSignalManager::~SrsSignalManager()
{
    pthread->stop();
    srs_freep(pthread);

    srs_close_stfd(signal_read_stfd);

    if (sig_pipe[0] > 0) {
        ::close(sig_pipe[0]);
    }
    if (sig_pipe[1] > 0) {
        ::close(sig_pipe[1]);
    }
}

int SrsSignalManager::initialize()
{
    int ret = ERROR_SUCCESS;
    return ret;
}

int SrsSignalManager::start()
{
    int ret = ERROR_SUCCESS;

    /**
    * Note that if multiple processes are used (see below),
    * the signal pipe should be initialized after the fork(2) call
    * so that each process has its own private pipe.
    */
    struct sigaction sa;

    /* Create signal pipe */
    if (pipe(sig_pipe) < 0) {
        ret = ERROR_SYSTEM_CREATE_PIPE;
        srs_error("create signal manager pipe failed. ret=%d", ret);
        return ret;
    }

    /* Install sig_catcher() as a signal handler */
    sa.sa_handler = SrsSignalManager::sig_catcher;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGNAL_RELOAD, &sa, NULL);

    sa.sa_handler = SrsSignalManager::sig_catcher;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);

    sa.sa_handler = SrsSignalManager::sig_catcher;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = SrsSignalManager::sig_catcher;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR2, &sa, NULL);

    srs_trace("signal installed");

    return pthread->start();
}

int SrsSignalManager::cycle()
{
    int ret = ERROR_SUCCESS;

    if (signal_read_stfd == NULL) {
        signal_read_stfd = st_netfd_open(sig_pipe[0]);
    }

    int signo;

    /* Read the next signal from the pipe */
    st_read(signal_read_stfd, &signo, sizeof(int), ST_UTIME_NO_TIMEOUT);

    /* Process signal synchronously */
    _server->on_signal(signo);

    return ret;
}

void SrsSignalManager::sig_catcher(int signo)
{
    int err;

    /* Save errno to restore it after the write() */
    err = errno;

    /* write() is reentrant/async-safe */
    int fd = SrsSignalManager::instance->sig_pipe[1];
    write(fd, &signo, sizeof(int));

    errno = err;
}
