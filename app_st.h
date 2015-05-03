#pragma once

#include <st.h>

// initialize st, requires epoll.
extern int srs_init_st();

// close the netfd, and close the underlayer fd.
extern void srs_close_stfd(st_netfd_t& stfd);
