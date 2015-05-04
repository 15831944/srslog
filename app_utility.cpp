#include "app_utility.h"
#include "ifaddrs.h"
#include "app_log.h"
#include <sys/types.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/time.h>

// this value must:
// equals to (SRS_SYS_CYCLE_INTERVAL*SRS_SYS_TIME_RESOLUTION_MS_TIMES)*1000
// @see SRS_SYS_TIME_RESOLUTION_MS_TIMES
#define SYS_TIME_RESOLUTION_US 300*1000

vector<string> _srs_system_ipv4_ips;

static int64_t _srs_system_time_us_cache = 0;
static int64_t _srs_system_time_startup_time = 0;

void retrieve_local_ipv4_ips()
{
    vector<string>& ips = _srs_system_ipv4_ips;

    ips.clear();

    ifaddrs* ifap;
    if (getifaddrs(&ifap) == -1) {
        srs_warn("retrieve local ips, ini ifaddrs failed.");
        return;
    }

    ifaddrs* p = ifap;
    while (p != NULL) {
        sockaddr* addr = p->ifa_addr;

        // retrieve ipv4 addr
        // ignore the tun0 network device,
        // which addr is NULL.
        // @see: https://github.com/winlinvip/simple-rtmp-server/issues/141
        if (addr && addr->sa_family == AF_INET) {
            in_addr* inaddr = &((sockaddr_in*)addr)->sin_addr;

            char buf[16];
            memset(buf, 0, sizeof(buf));

            if ((inet_ntop(addr->sa_family, inaddr, buf, sizeof(buf))) == NULL) {
                srs_warn("convert local ip failed");
                break;
            }

            std::string ip = buf;
            if (ip != SRS_CONSTS_LOCALHOST) {
                srs_trace("retrieve local ipv4 ip=%s, index=%d", ip.c_str(), (int)ips.size());
                ips.push_back(ip);
            }
        }

        p = p->ifa_next;
    }

    freeifaddrs(ifap);
}

vector<string>& srs_get_local_ipv4_ips()
{
    if (_srs_system_ipv4_ips.empty()) {
        retrieve_local_ipv4_ips();
    }

    return _srs_system_ipv4_ips;
}

int srs_get_log_level(std::string level)
{
    if ("verbose" == level) {
        return SrsLogLevel::Verbose;
    } else if ("info" == level) {
        return SrsLogLevel::Info;
    } else if ("trace" == level) {
        return SrsLogLevel::Trace;
    } else if ("warn" == level) {
        return SrsLogLevel::Warn;
    } else if ("error" == level) {
        return SrsLogLevel::Error;
    } else {
        return SrsLogLevel::Disabled;
    }
}

string srs_get_peer_ip(int fd)
{
    std::string ip;

    // discovery client information
    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    if (getpeername(fd, (sockaddr*)&addr, &addrlen) == -1) {
        return ip;
    }
    srs_verbose("get peer name success.");

    // ip v4 or v6
    char buf[INET6_ADDRSTRLEN];
    memset(buf, 0, sizeof(buf));

    if ((inet_ntop(addr.sin_family, &addr.sin_addr, buf, sizeof(buf))) == NULL) {
        return ip;
    }
    srs_verbose("get peer ip of client ip=%s, fd=%d", buf, fd);

    ip = buf;

    srs_verbose("get peer ip success. ip=%s, fd=%d", ip.c_str(), fd);

    return ip;
}

int64_t srs_get_system_time_ms()
{
    if (_srs_system_time_us_cache <= 0) {
        srs_update_system_time_ms();
    }

    return _srs_system_time_us_cache / 1000;
}

void srs_update_system_time_ms()
{
    timeval now;

    if (gettimeofday(&now, NULL) < 0) {
        srs_warn("gettimeofday failed, ignore");
        return;
    }

    // @see: https://github.com/winlinvip/simple-rtmp-server/issues/35
    // we must convert the tv_sec/tv_usec to int64_t.
    int64_t now_us = ((int64_t)now.tv_sec) * 1000 * 1000 + (int64_t)now.tv_usec;

    // for some ARM os, the starttime maybe invalid,
    // for example, on the cubieboard2, the srs_startup_time is 1262304014640,
    // while now is 1403842979210 in ms, diff is 141538964570 ms, 1638 days
    // it's impossible, and maybe the problem of startup time is invalid.
    // use date +%s to get system time is 1403844851.
    // so we use relative time.
    if (_srs_system_time_us_cache <= 0) {
        _srs_system_time_us_cache = now_us;
        _srs_system_time_startup_time = now_us;
        return;
    }

    // use relative time.
    int64_t diff = now_us - _srs_system_time_us_cache;
    diff = srs_max(0, diff);
    if (diff < 0 || diff > 1000 * SYS_TIME_RESOLUTION_US) {
        srs_warn("system time jump, history=%"PRId64"us, now=%"PRId64"us, diff=%"PRId64"us",
            _srs_system_time_us_cache, now_us, diff);
        // @see: https://github.com/winlinvip/simple-rtmp-server/issues/109
        _srs_system_time_startup_time += diff;
    }

    _srs_system_time_us_cache = now_us;
    srs_info("system time updated, startup=%"PRId64"us, now=%"PRId64"us",
        _srs_system_time_startup_time, _srs_system_time_us_cache);
}

bool srs_is_little_endian()
{
    // convert to network(big-endian) order, if not equals,
    // the system is little-endian, so need to convert the int64
    static int little_endian_check = -1;

    if(little_endian_check == -1) {
        union {
            int32_t i;
            int8_t c;
        } little_check_union;

        little_check_union.i = 0x01;
        little_endian_check = little_check_union.c;
    }

    return (little_endian_check == 1);
}
