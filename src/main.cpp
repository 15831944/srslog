#include "app_log.h"
#include "app_error_code.h"
#include "app_utility.h"
#include "app_server.h"
#include <sys/wait.h>
#include <stdlib.h>
#include "conf/config_info.h"

int run();
int run_master();

int main(int argc, char *argv[])
{
    int ret = ERROR_SUCCESS;

    if (argc < 2) {
        printf("not start, useage: app configfile.");
        return ret;
    }

    if ((ret = g_config->load_configfile(argv[1])) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = g_config->parse_and_check()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = _srs_log->initialize()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = _srs_server->initialize()) != ERROR_SUCCESS) {
        return ret;
    }

    return run();
}

int run()
{
    if (!g_config->get_daemon()) {
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

    //fix.  add the connect type you want to create.
    if ((ret = _srs_server->connect_redis()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = _srs_server->cycle()) != ERROR_SUCCESS) {
        return ret;
    }

    return 0;
}
