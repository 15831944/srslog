#include "app_utility.h"
#include "ifaddrs.h"
#include "app_log.h"
#include <sys/types.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

vector<string> _srs_system_ipv4_ips;

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
