#include "app_config.h"
#include "app_log.h"
#include "app_error_code.h"

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

    while (1)
    {
        srs_info("test now, time= %d", time(0));
        srs_error("test now, time= %d", time(0));
        srs_trace("test now, time= %d", time(0));
        srs_warn("test now, time= %d", time(0));
        srs_verbose("test now, time= %d", time(0));

        sleep(1);
    }

    return ret;
}
