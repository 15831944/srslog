#include "app_server.h"

SrsServer::SrsServer()
{
    signal_reload = false;
    signal_gmc_stop = false;
    pid_fd = -1;

    signal_manager = NULL;
    kbps = NULL;

    // donot new object in constructor,
    // for some global instance is not ready now,
    // new these objects in initialize instead.
#ifdef SRS_AUTO_HTTP_API
    http_api_handler = NULL;
#endif
#ifdef SRS_AUTO_HTTP_SERVER
    http_stream_handler = NULL;
#endif
#ifdef SRS_AUTO_HTTP_PARSER
    http_heartbeat = NULL;
#endif
#ifdef SRS_AUTO_INGEST
    ingester = NULL;
#endif
}

SrsServer::~SrsServer()
{
    destroy();
}

void SrsServer::destroy()
{
    srs_warn("start destroy server");

    _srs_config->unsubscribe(this);

    close_listeners(SrsListenerRtmpStream);
    close_listeners(SrsListenerHttpApi);
    close_listeners(SrsListenerHttpStream);

#ifdef SRS_AUTO_INGEST
    ingester->stop();
#endif

#ifdef SRS_AUTO_HTTP_API
    srs_freep(http_api_handler);
#endif

#ifdef SRS_AUTO_HTTP_SERVER
    srs_freep(http_stream_handler);
#endif

#ifdef SRS_AUTO_HTTP_PARSER
    srs_freep(http_heartbeat);
#endif

#ifdef SRS_AUTO_INGEST
    srs_freep(ingester);
#endif

    if (pid_fd > 0) {
        ::close(pid_fd);
        pid_fd = -1;
    }

    srs_freep(signal_manager);
    srs_freep(kbps);

    // @remark never destroy the connections,
    // for it's still alive.

    // @remark never destroy the source,
    // when we free all sources, the fmle publish may retry
    // and segment fault.
}

int SrsServer::initialize()
{
    int ret = ERROR_SUCCESS;

    // ensure the time is ok.
    srs_update_system_time_ms();

    // for the main objects(server, config, log, context),
    // never subscribe handler in constructor,
    // instead, subscribe handler in initialize method.
    srs_assert(_srs_config);
    _srs_config->subscribe(this);

    srs_assert(!signal_manager);
    signal_manager = new SrsSignalManager(this);

    srs_assert(!kbps);
    kbps = new SrsKbps();
    kbps->set_io(NULL, NULL);

#ifdef SRS_AUTO_HTTP_API
    srs_assert(!http_api_handler);
    http_api_handler = SrsHttpHandler::create_http_api();
#endif
#ifdef SRS_AUTO_HTTP_SERVER
    srs_assert(!http_stream_handler);
    http_stream_handler = SrsHttpHandler::create_http_stream();
#endif
#ifdef SRS_AUTO_HTTP_PARSER
    srs_assert(!http_heartbeat);
    http_heartbeat = new SrsHttpHeartbeat();
#endif
#ifdef SRS_AUTO_INGEST
    srs_assert(!ingester);
    ingester = new SrsIngester();
#endif

#ifdef SRS_AUTO_HTTP_API
    if ((ret = http_api_handler->initialize()) != ERROR_SUCCESS) {
        return ret;
    }
#endif

#ifdef SRS_AUTO_HTTP_SERVER
    if ((ret = http_stream_handler->initialize()) != ERROR_SUCCESS) {
        return ret;
    }
#endif

    return ret;
}

int SrsServer::initialize_signal()
{
    return signal_manager->initialize();
}

int SrsServer::acquire_pid_file()
{
    int ret = ERROR_SUCCESS;

    std::string pid_file = _srs_config->get_pid_file();

    // -rw-r--r--
    // 644
    int mode = S_IRUSR | S_IWUSR |  S_IRGRP | S_IROTH;

    int fd;
    // open pid file
    if ((fd = ::open(pid_file.c_str(), O_WRONLY | O_CREAT, mode)) < 0) {
        ret = ERROR_SYSTEM_PID_ACQUIRE;
        srs_error("open pid file %s error, ret=%#x", pid_file.c_str(), ret);
        return ret;
    }

    // require write lock
    struct flock lock;

    lock.l_type = F_WRLCK; // F_RDLCK, F_WRLCK, F_UNLCK
    lock.l_start = 0; // type offset, relative to l_whence
    lock.l_whence = SEEK_SET;  // SEEK_SET, SEEK_CUR, SEEK_END
    lock.l_len = 0;

    if (fcntl(fd, F_SETLK, &lock) < 0) {
        if(errno == EACCES || errno == EAGAIN) {
            ret = ERROR_SYSTEM_PID_ALREADY_RUNNING;
            srs_error("srs is already running! ret=%#x", ret);
            return ret;
        }

        ret = ERROR_SYSTEM_PID_LOCK;
        srs_error("require lock for file %s error! ret=%#x", pid_file.c_str(), ret);
        return ret;
    }

    // truncate file
    if (ftruncate(fd, 0) < 0) {
        ret = ERROR_SYSTEM_PID_TRUNCATE_FILE;
        srs_error("truncate pid file %s error! ret=%#x", pid_file.c_str(), ret);
        return ret;
    }

    int pid = (int)getpid();

    // write the pid
    char buf[512];
    snprintf(buf, sizeof(buf), "%d", pid);
    if (write(fd, buf, strlen(buf)) != (int)strlen(buf)) {
        ret = ERROR_SYSTEM_PID_WRITE_FILE;
        srs_error("write our pid error! pid=%d file=%s ret=%#x", pid, pid_file.c_str(), ret);
        return ret;
    }

    // auto close when fork child process.
    int val;
    if ((val = fcntl(fd, F_GETFD, 0)) < 0) {
        ret = ERROR_SYSTEM_PID_GET_FILE_INFO;
        srs_error("fnctl F_GETFD error! file=%s ret=%#x", pid_file.c_str(), ret);
        return ret;
    }
    val |= FD_CLOEXEC;
    if (fcntl(fd, F_SETFD, val) < 0) {
        ret = ERROR_SYSTEM_PID_SET_FILE_INFO;
        srs_error("fcntl F_SETFD error! file=%s ret=%#x", pid_file.c_str(), ret);
        return ret;
    }

    srs_trace("write pid=%d to %s success!", pid, pid_file.c_str());
    pid_fd = fd;

    return ret;
}

int SrsServer::initialize_st()
{
    int ret = ERROR_SUCCESS;

    // init st
    if ((ret = srs_init_st()) != ERROR_SUCCESS) {
        srs_error("init st failed. ret=%d", ret);
        return ret;
    }

    // @remark, st alloc segment use mmap, which only support 32757 threads,
    // if need to support more, for instance, 100k threads, define the macro MALLOC_STACK.
    // TODO: FIXME: maybe can use "sysctl vm.max_map_count" to refine.
    if (_srs_config->get_max_connections() > 32756) {
        ret = ERROR_ST_EXCEED_THREADS;
        srs_error("st mmap for stack allocation must <= %d threads, "
            "@see Makefile of st for MALLOC_STACK, please build st manually by "
            "\"make EXTRA_CFLAGS=-DMALLOC_STACK linux-debug\", ret=%d", ret);
        return ret;
    }

    // set current log id.
    _srs_context->generate_id();
    srs_trace("server main cid=%d", _srs_context->get_id());

    return ret;
}

int SrsServer::listen()
{
    int ret = ERROR_SUCCESS;

    if ((ret = listen_rtmp()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = listen_http_api()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = listen_http_stream()) != ERROR_SUCCESS) {
        return ret;
    }

    return ret;
}

int SrsServer::register_signal()
{
    // start signal process thread.
    return signal_manager->start();
}

int SrsServer::ingest()
{
    int ret = ERROR_SUCCESS;

#ifdef SRS_AUTO_INGEST
    if ((ret = ingester->start()) != ERROR_SUCCESS) {
        srs_error("start ingest streams failed. ret=%d", ret);
        return ret;
    }
#endif

    return ret;
}

int SrsServer::cycle()
{
    int ret = ERROR_SUCCESS;

    ret = do_cycle();

#ifdef SRS_AUTO_GPERF_MC
    destroy();

    // remark, for gmc, never invoke the exit().
    srs_warn("sleep a long time for system st-threads to cleanup.");
    st_usleep(3 * 1000 * 1000);
    srs_warn("system quit");
#else
    srs_warn("main cycle terminated, system quit normally.");
    exit(0);
#endif

    return ret;
}

void SrsServer::remove(SrsConnection* conn)
{
    std::vector<SrsConnection*>::iterator it = std::find(conns.begin(), conns.end(), conn);

    // removed by destroy, ignore.
    if (it == conns.end()) {
        srs_warn("server moved connection, ignore.");
        return;
    }

    conns.erase(it);

    srs_info("conn removed. conns=%d", (int)conns.size());

    // resample the resource of specified connection.
    resample_kbps(conn);

    // all connections are created by server,
    // so we free it here.
    srs_freep(conn);
}

void SrsServer::on_signal(int signo)
{
    if (signo == SIGNAL_RELOAD) {
        signal_reload = true;
        return;
    }

    if (signo == SIGINT || signo == SIGUSR2) {
#ifdef SRS_AUTO_GPERF_MC
        srs_trace("gmc is on, main cycle will terminate normally.");
        signal_gmc_stop = true;
#else
        srs_trace("user terminate program");
        exit(0);
#endif
        return;
    }

    if (signo == SIGTERM) {
        srs_trace("user terminate program");
        exit(0);
        return;
    }
}

int SrsServer::do_cycle()
{
    int ret = ERROR_SUCCESS;

    // find the max loop
    int max = srs_max(0, SRS_SYS_TIME_RESOLUTION_MS_TIMES);

#ifdef SRS_AUTO_STAT
    max = srs_max(max, SRS_SYS_RUSAGE_RESOLUTION_TIMES);
    max = srs_max(max, SRS_SYS_CPU_STAT_RESOLUTION_TIMES);
    max = srs_max(max, SRS_SYS_DISK_STAT_RESOLUTION_TIMES);
    max = srs_max(max, SRS_SYS_MEMINFO_RESOLUTION_TIMES);
    max = srs_max(max, SRS_SYS_PLATFORM_INFO_RESOLUTION_TIMES);
    max = srs_max(max, SRS_SYS_NETWORK_DEVICE_RESOLUTION_TIMES);
    max = srs_max(max, SRS_SYS_NETWORK_RTMP_SERVER_RESOLUTION_TIMES);
#endif

    // the deamon thread, update the time cache
    while (true) {
        // the interval in config.
        int heartbeat_max_resolution = (int)(_srs_config->get_heartbeat_interval() / SRS_SYS_CYCLE_INTERVAL);

        // dynamic fetch the max.
        int __max = max;
        __max = srs_max(__max, heartbeat_max_resolution);

        for (int i = 0; i < __max; i++) {
            st_usleep(SRS_SYS_CYCLE_INTERVAL * 1000);

// for gperf heap checker,
// @see: research/gperftools/heap-checker/heap_checker.cc
// if user interrupt the program, exit to check mem leak.
// but, if gperf, use reload to ensure main return normally,
// because directly exit will cause core-dump.
#ifdef SRS_AUTO_GPERF_MC
            if (signal_gmc_stop) {
                srs_warn("gmc got singal to stop server.");
                return ret;
            }
#endif

            if (signal_reload) {
                signal_reload = false;
                srs_info("get signal reload, to reload the config.");

                if ((ret = _srs_config->reload()) != ERROR_SUCCESS) {
                    srs_error("reload config failed. ret=%d", ret);
                    return ret;
                }
                srs_trace("reload config success.");
            }

            // update the cache time or rusage.
            if ((i % SRS_SYS_TIME_RESOLUTION_MS_TIMES) == 0) {
                srs_info("update current time cache.");
                srs_update_system_time_ms();
            }

#ifdef SRS_AUTO_STAT
            if ((i % SRS_SYS_RUSAGE_RESOLUTION_TIMES) == 0) {
                srs_info("update resource info, rss.");
                srs_update_system_rusage();
            }
            if ((i % SRS_SYS_CPU_STAT_RESOLUTION_TIMES) == 0) {
                srs_info("update cpu info, cpu usage.");
                srs_update_proc_stat();
            }
            if ((i % SRS_SYS_DISK_STAT_RESOLUTION_TIMES) == 0) {
                srs_info("update disk info, disk iops.");
                srs_update_disk_stat();
            }
            if ((i % SRS_SYS_MEMINFO_RESOLUTION_TIMES) == 0) {
                srs_info("update memory info, usage/free.");
                srs_update_meminfo();
            }
            if ((i % SRS_SYS_PLATFORM_INFO_RESOLUTION_TIMES) == 0) {
                srs_info("update platform info, uptime/load.");
                srs_update_platform_info();
            }
            if ((i % SRS_SYS_NETWORK_DEVICE_RESOLUTION_TIMES) == 0) {
                srs_info("update network devices info.");
                srs_update_network_devices();
            }
            if ((i % SRS_SYS_NETWORK_RTMP_SERVER_RESOLUTION_TIMES) == 0) {
                srs_info("update network rtmp server info.");
                resample_kbps(NULL);
                srs_update_rtmp_server((int)conns.size(), kbps);
            }
    #ifdef SRS_AUTO_HTTP_PARSER
            if (_srs_config->get_heartbeat_enabled()) {
                if ((i % heartbeat_max_resolution) == 0) {
                    srs_info("do http heartbeat, for internal server to report.");
                    http_heartbeat->heartbeat();
                }
            }
    #endif
#endif
            srs_info("server main thread loop");
        }
    }

    return ret;
}

int SrsServer::listen_rtmp()
{
    int ret = ERROR_SUCCESS;

    // stream service port.
    std::vector<std::string> ports = _srs_config->get_listen();
    srs_assert((int)ports.size() > 0);

    close_listeners(SrsListenerRtmpStream);

    for (int i = 0; i < (int)ports.size(); i++) {
        SrsListener* listener = new SrsListener(this, SrsListenerRtmpStream);
        listeners.push_back(listener);

        int port = ::atoi(ports[i].c_str());
        if ((ret = listener->listen(port)) != ERROR_SUCCESS) {
            srs_error("RTMP stream listen at port %d failed. ret=%d", port, ret);
            return ret;
        }
    }

    return ret;
}

int SrsServer::listen_http_api()
{
    int ret = ERROR_SUCCESS;

#ifdef SRS_AUTO_HTTP_API
    close_listeners(SrsListenerHttpApi);
    if (_srs_config->get_http_api_enabled()) {
        SrsListener* listener = new SrsListener(this, SrsListenerHttpApi);
        listeners.push_back(listener);

        int port = _srs_config->get_http_api_listen();
        if ((ret = listener->listen(port)) != ERROR_SUCCESS) {
            srs_error("HTTP api listen at port %d failed. ret=%d", port, ret);
            return ret;
        }
    }
#endif

    return ret;
}

int SrsServer::listen_http_stream()
{
    int ret = ERROR_SUCCESS;

#ifdef SRS_AUTO_HTTP_SERVER
    close_listeners(SrsListenerHttpStream);
    if (_srs_config->get_http_stream_enabled()) {
        SrsListener* listener = new SrsListener(this, SrsListenerHttpStream);
        listeners.push_back(listener);

        int port = _srs_config->get_http_stream_listen();
        if ((ret = listener->listen(port)) != ERROR_SUCCESS) {
            srs_error("HTTP stream listen at port %d failed. ret=%d", port, ret);
            return ret;
        }
    }
#endif

    return ret;
}

void SrsServer::close_listeners(SrsListenerType type)
{
    std::vector<SrsListener*>::iterator it;
    for (it = listeners.begin(); it != listeners.end();) {
        SrsListener* listener = *it;

        if (listener->type() != type) {
            ++it;
            continue;
        }

        srs_freep(listener);
        it = listeners.erase(it);
    }
}

void SrsServer::resample_kbps(SrsConnection* conn, bool do_resample)
{
    // resample all when conn is NULL.
    if (!conn) {
        for (std::vector<SrsConnection*>::iterator it = conns.begin(); it != conns.end(); ++it) {
            SrsConnection* client = *it;
            srs_assert(client);

            // only resample, do resample when all finished.
            resample_kbps(client, false);
        }

        kbps->sample();
        return;
    }

    // resample for connection.
    conn->kbps_resample();

    kbps->add_delta(conn);

    // resample for server.
    if (do_resample) {
        kbps->sample();
    }
}

int SrsServer::accept_client(SrsListenerType type, st_netfd_t client_stfd)
{
    int ret = ERROR_SUCCESS;

    int max_connections = _srs_config->get_max_connections();
    if ((int)conns.size() >= max_connections) {
        int fd = st_netfd_fileno(client_stfd);

        srs_error("exceed the max connections, drop client: "
            "clients=%d, max=%d, fd=%d", (int)conns.size(), max_connections, fd);

        srs_close_stfd(client_stfd);

        return ret;
    }

    SrsConnection* conn = NULL;
    if (type == SrsListenerRtmpStream) {
        conn = new SrsRtmpConn(this, client_stfd);
    } else if (type == SrsListenerHttpApi) {
#ifdef SRS_AUTO_HTTP_API
        conn = new SrsHttpApi(this, client_stfd, http_api_handler);
#else
        srs_warn("close http client for server not support http-api");
        srs_close_stfd(client_stfd);
        return ret;
#endif
    } else if (type == SrsListenerHttpStream) {
#ifdef SRS_AUTO_HTTP_SERVER
        conn = new SrsHttpConn(this, client_stfd, http_stream_handler);
#else
        srs_warn("close http client for server not support http-server");
        srs_close_stfd(client_stfd);
        return ret;
#endif
    } else {
        // TODO: FIXME: handler others
    }
    srs_assert(conn);

    // directly enqueue, the cycle thread will remove the client.
    conns.push_back(conn);
    srs_verbose("add conn to vector.");

    // cycle will start process thread and when finished remove the client.
    // @remark never use the conn, for it maybe destroyed.
    if ((ret = conn->start()) != ERROR_SUCCESS) {
        return ret;
    }
    srs_verbose("conn started success.");

    srs_verbose("accept client finished. conns=%d, ret=%d", (int)conns.size(), ret);

    return ret;
}

int SrsServer::on_reload_listen()
{
    return listen();
}

int SrsServer::on_reload_pid()
{
    if (pid_fd > 0) {
        ::close(pid_fd);
        pid_fd = -1;
    }

    return acquire_pid_file();
}

int SrsServer::on_reload_vhost_added(std::string vhost)
{
    int ret = ERROR_SUCCESS;

#ifdef SRS_AUTO_HTTP_SERVER
    if (!_srs_config->get_vhost_http_enabled(vhost)) {
        return ret;
    }

    if ((ret = on_reload_vhost_http_updated()) != ERROR_SUCCESS) {
        return ret;
    }
#endif

    return ret;
}

int SrsServer::on_reload_vhost_removed(std::string /*vhost*/)
{
    int ret = ERROR_SUCCESS;

#ifdef SRS_AUTO_HTTP_SERVER
    if ((ret = on_reload_vhost_http_updated()) != ERROR_SUCCESS) {
        return ret;
    }
#endif

    return ret;
}

int SrsServer::on_reload_vhost_http_updated()
{
    int ret = ERROR_SUCCESS;

#ifdef SRS_AUTO_HTTP_SERVER
    srs_freep(http_stream_handler);
    http_stream_handler = SrsHttpHandler::create_http_stream();

    if ((ret = http_stream_handler->initialize()) != ERROR_SUCCESS) {
        return ret;
    }
#endif

    return ret;
}

int SrsServer::on_reload_http_api_enabled()
{
    int ret = ERROR_SUCCESS;

#ifdef SRS_AUTO_HTTP_API
    ret = listen_http_api();
#endif

    return ret;
}

int SrsServer::on_reload_http_api_disabled()
{
    int ret = ERROR_SUCCESS;

#ifdef SRS_AUTO_HTTP_API
    close_listeners(SrsListenerHttpApi);
#endif

    return ret;
}

int SrsServer::on_reload_http_stream_enabled()
{
    int ret = ERROR_SUCCESS;

#ifdef SRS_AUTO_HTTP_SERVER
    ret = listen_http_stream();
#endif

    return ret;
}

int SrsServer::on_reload_http_stream_disabled()
{
    int ret = ERROR_SUCCESS;

#ifdef SRS_AUTO_HTTP_SERVER
    close_listeners(SrsListenerHttpStream);
#endif

    return ret;
}

int SrsServer::on_reload_http_stream_updated()
{
    int ret = ERROR_SUCCESS;

#ifdef SRS_AUTO_HTTP_SERVER
    if ((ret = on_reload_http_stream_enabled()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = on_reload_vhost_http_updated()) != ERROR_SUCCESS) {
        return ret;
    }
#endif

    return ret;
}

