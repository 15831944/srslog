#include "app_config.h"
#include "app_log.h"
#include "app_error_code.h"
#include "app_utility.h"
#include "app_server.h"
#include <sys/wait.h>
#include <stdlib.h>

int run();
int run_master();

int main(int argc, char *argv[])
{
    int ret = ERROR_SUCCESS;

    // never use srs log(srs_trace, srs_error, etc) before config parse the option,
    // which will load the log config and apply it.
    if ((ret = _srs_config->parse_options(argc, argv)) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = _srs_log->initialize()) != ERROR_SUCCESS) {
        return ret;
    }

    // we check the config when the log initialized.
    if ((ret = _srs_config->check_config()) != ERROR_SUCCESS) {
        return ret;
    }

    srs_trace("srs"RTMP_SIG_SRS_VERSION);
    srs_trace("license: "RTMP_SIG_SRS_LICENSE);
    srs_trace("primary: "RTMP_SIG_SRS_PRIMARY);
    srs_trace("authors: "RTMP_SIG_SRS_AUTHROS);
    srs_trace("uname: "SRS_AUTO_UNAME);
    srs_trace("build: %s, %s", SRS_AUTO_BUILD_DATE, srs_is_little_endian()? "little-endian":"big-endian");
    srs_trace("configure: "SRS_AUTO_USER_CONFIGURE);
    srs_trace("features: "SRS_AUTO_CONFIGURE);
    srs_trace("conf: %s, limit: %d", _srs_config->config().c_str(), _srs_config->get_max_connections());


    /**
    * we do nothing in the constructor of server,
    * and use initialize to create members, set hooks for instance the reload handler,
    * all initialize will done in this stage.
    */
    if ((ret = _srs_server->initialize()) != ERROR_SUCCESS) {
        return ret;
    }

    return run();
}

int run()
{
    // if not deamon, directly run master.
    if (!_srs_config->get_deamon()) {
        return run_master();
    }

    srs_trace("start deamon mode...");

    int pid = fork();

    if(pid < 0) {
        srs_error("create process error. ret=-1"); //ret=0
        return -1;
    }

    // grandpa
    if(pid > 0) {
        int status = 0;
        if(waitpid(pid, &status, 0) == -1) {
            srs_error("wait child process error! ret=-1"); //ret=0
        }
        srs_trace("grandpa process exit.");
        exit(0);
    }

    // father
    pid = fork();

    if(pid < 0) {
        srs_error("create process error. ret=0");
        return -1;
    }

    if(pid > 0) {
        srs_trace("father process exit. ret=0");
        exit(0);
    }

    // son
    srs_trace("son(deamon) process running.");

    return run_master();
}

int run_master()
{
    int ret = ERROR_SUCCESS;

    if ((ret = _srs_server->initialize_signal()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = _srs_server->acquire_pid_file()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = _srs_server->initialize_st()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = _srs_server->listen()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = _srs_server->register_signal()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = _srs_server->ingest()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = _srs_server->connecdb_redis()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret =_srs_server->check_charge()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = _srs_server->cache_mysql_to_redis()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = _srs_server->cycle()) != ERROR_SUCCESS) {
        return ret;
    }

    return 0;
}
