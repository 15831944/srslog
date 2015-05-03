#include "app_st.h"
int srs_init_st()
{
    int ret = ERROR_SUCCESS;

    // check epoll, some old linux donot support epoll.
    // @see https://github.com/winlinvip/simple-rtmp-server/issues/162
    if (!srs_st_epoll_is_supported()) {
        ret = ERROR_ST_SET_EPOLL;
        srs_error("epoll required. ret=%d", ret);
        return ret;
    }

    // use linux epoll.
    if (st_set_eventsys(ST_EVENTSYS_ALT) == -1) {
        ret = ERROR_ST_SET_EPOLL;
        srs_error("st_set_eventsys use linux epoll failed. ret=%d", ret);
        return ret;
    }
    srs_verbose("st_set_eventsys use linux epoll success");

    if(st_init() != 0){
        ret = ERROR_ST_INITIALIZE;
        srs_error("st_init failed. ret=%d", ret);
        return ret;
    }
    srs_verbose("st_init success");

    return ret;
}

void srs_close_stfd(st_netfd_t& stfd)
{
    if (stfd) {
        int fd = st_netfd_fileno(stfd);
        st_netfd_close(stfd);
        stfd = NULL;

        // st does not close it sometimes,
        // close it manually.
        close(fd);
    }
}

