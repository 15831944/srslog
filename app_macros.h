#pragma once

// current release version
#define VERSION_MAJOR       1
#define VERSION_MINOR       0
#define VERSION_REVISION    30

// server info.
#define RTMP_SIG_SRS_KEY "SRS"
#define RTMP_SIG_SRS_CODE "HuKaiqun"
#define RTMP_SIG_SRS_ROLE "origin/edge server"
#define RTMP_SIG_SRS_NAME RTMP_SIG_SRS_KEY"(Simple RTMP Server)"
#define RTMP_SIG_SRS_URL_SHORT "github.com/winlinvip/simple-rtmp-server"
#define RTMP_SIG_SRS_URL "https://"RTMP_SIG_SRS_URL_SHORT
#define RTMP_SIG_SRS_WEB "http://blog.csdn.net/win_lin"
#define RTMP_SIG_SRS_EMAIL "winlin@vip.126.com"
#define RTMP_SIG_SRS_LICENSE "The MIT License (MIT)"
#define RTMP_SIG_SRS_COPYRIGHT "Copyright (c) 2013-2014 winlin"
#define RTMP_SIG_SRS_PRIMARY "winlin"
#define RTMP_SIG_SRS_AUTHROS "wenjie.zhao"
#define RTMP_SIG_SRS_CONTRIBUTORS_URL RTMP_SIG_SRS_URL"/blob/master/AUTHORS.txt"
#define RTMP_SIG_SRS_HANDSHAKE RTMP_SIG_SRS_KEY"("RTMP_SIG_SRS_VERSION")"
#define RTMP_SIG_SRS_RELEASE "https://github.com/winlinvip/simple-rtmp-server/tree/1.0release"
#define RTMP_SIG_SRS_HTTP_SERVER "https://github.com/winlinvip/simple-rtmp-server/wiki/v1_CN_HTTPServer#feature"
#define RTMP_SIG_SRS_VERSION __SRS_XSTR(VERSION_MAJOR)"."__SRS_XSTR(VERSION_MINOR)"."__SRS_XSTR(VERSION_REVISION)
#define RTMP_SIG_SRS_SERVER RTMP_SIG_SRS_KEY"/"RTMP_SIG_SRS_VERSION"("RTMP_SIG_SRS_CODE")"

#define SRS_WIKI_URL_LOG "https://github.com/winlinvip/simple-rtmp-server/wiki/v1_CN_SrsLog"

#define SRS_AUTO_BUILD_TS "1430490206"
#define SRS_AUTO_BUILD_DATE "2015-05-01 22:23:26"
#define SRS_AUTO_UNAME "Linux yangkai 2.6.32-431.el6.x86_64 #1 SMP Fri Nov 22 03:15:09 UTC 2013 x86_64 x86_64 x86_64 GNU/Linux"
#define SRS_AUTO_USER_CONFIGURE "--x86-x64 "
#define SRS_AUTO_CONFIGURE "--prefix=/usr/local/srs --with-hls --with-dvr --without-nginx --with-ssl --without-ffmpeg --with-transcode --with-ingest --with-stat --with-http-callback --with-http-server --with-http-api --with-librtmp --without-research --with-utest --without-gperf --without-gmc --without-gmp --without-gcp --without-gprof --without-arm-ubuntu12 --without-mips-ubuntu12 --log-trace"

#define SRS_AUTO_EMBEDED_TOOL_CHAIN "normal x86/x64 gcc"
///////////////////////////////////////////////////////////
// default consts values
///////////////////////////////////////////////////////////
#define SRS_CONF_DEFAULT_PID_FILE "./objs/srs.pid"
#define SRS_CONF_DEFAULT_LOG_FILE "./objs/srs.log"
#define SRS_CONF_DEFAULT_LOG_LEVEL "trace"
#define SRS_CONF_DEFAULT_LOG_TANK_CONSOLE "console"
#define SRS_CONF_DEFAULT_COFNIG_FILE "conf/srs.conf"
#define SRS_CONF_DEFAULT_FF_LOG_DIR "./objs"

#define SRS_CONF_DEFAULT_MAX_CONNECTIONS 1000
#define SRS_CONF_DEFAULT_HLS_PATH "./objs/nginx/html"
#define SRS_CONF_DEFAULT_HLS_FRAGMENT 10
#define SRS_CONF_DEFAULT_HLS_WINDOW 60
#define SRS_CONF_DEFAULT_HLS_ON_ERROR_IGNORE "ignore"
#define SRS_CONF_DEFAULT_HLS_ON_ERROR_DISCONNECT "disconnect"
#define SRS_CONF_DEFAULT_HLS_ON_ERROR_CONTINUE "continue"
#define SRS_CONF_DEFAULT_HLS_ON_ERROR SRS_CONF_DEFAULT_HLS_ON_ERROR_IGNORE
#define SRS_CONF_DEFAULT_DVR_PATH "./objs/nginx/html"
#define SRS_CONF_DEFAULT_DVR_PLAN_SESSION "session"
#define SRS_CONF_DEFAULT_DVR_PLAN_SEGMENT "segment"
#define SRS_CONF_DEFAULT_DVR_PLAN SRS_CONF_DEFAULT_DVR_PLAN_SESSION
#define SRS_CONF_DEFAULT_DVR_DURATION 30
#define SRS_CONF_DEFAULT_TIME_JITTER "full"
// in seconds, the live queue length.
#define SRS_CONF_DEFAULT_QUEUE_LENGTH 30
// in seconds, the paused queue length.
#define SRS_CONF_DEFAULT_PAUSED_LENGTH 10
// the interval in seconds for bandwidth check
#define SRS_CONF_DEFAULT_BANDWIDTH_INTERVAL 30
// the interval in seconds for bandwidth check
#define SRS_CONF_DEFAULT_BANDWIDTH_LIMIT_KBPS 1000

#define SRS_CONF_DEFAULT_HTTP_MOUNT "/"
#define SRS_CONF_DEFAULT_HTTP_DIR SRS_CONF_DEFAULT_HLS_PATH

#define SRS_CONF_DEFAULT_HTTP_STREAM_PORT 8080
#define SRS_CONF_DEFAULT_HTTP_API_PORT 1985

#define SRS_CONF_DEFAULT_HTTP_HEAETBEAT_ENABLED false
#define SRS_CONF_DEFAULT_HTTP_HEAETBEAT_INTERVAL 9.9
#define SRS_CONF_DEFAULT_HTTP_HEAETBEAT_URL "http://"SRS_CONSTS_LOCALHOST":8085/api/v1/servers"
#define SRS_CONF_DEFAULT_HTTP_HEAETBEAT_SUMMARIES false

#define SRS_CONF_DEFAULT_STATS_NETWORK_DEVICE_INDEX 0

#define SRS_CONF_DEFAULT_STAGE_PLAY_USER_INTERVAL_MS 10000
#define SRS_CONF_DEFAULT_STAGE_PUBLISH_USER_INTERVAL_MS 10000
#define SRS_CONF_DEFAULT_STAGE_FORWARDER_INTERVAL_MS 10000
#define SRS_CONF_DEFAULT_STAGE_ENCODER_INTERVAL_MS 10000
#define SRS_CONF_DEFAULT_STAGE_INGESTER_INTERVAL_MS 10000
#define SRS_CONF_DEFAULT_STAGE_HLS_INTERVAL_MS 10000
#define SRS_CONF_DEFAULT_STAGE_EDGE_INTERVAL_MS 10000

#define SRS_CONF_DEFAULT_INGEST_TYPE_FILE "file"
#define SRS_CONF_DEFAULT_INGEST_TYPE_STREAM "stream"

#define SRS_CONF_DEFAULT_TRANSCODE_IFORMAT "flv"
#define SRS_CONF_DEFAULT_TRANSCODE_OFORMAT "flv"

// system interval
// all resolution times should be times togother,
// for example, system-time is 3(300ms),
// then rusage can be 3*x, for instance, 3*10=30(3s),
// the meminfo canbe 30*x, for instance, 30*2=60(6s)
#define SRS_SYS_CYCLE_INTERVAL 100

// update time interval:
//      SRS_SYS_CYCLE_INTERVAL * SRS_SYS_TIME_RESOLUTION_MS_TIMES
// @see SYS_TIME_RESOLUTION_US
#define SRS_SYS_TIME_RESOLUTION_MS_TIMES 3

///////////////////////////////////////////////////////////
// RTMP consts values
///////////////////////////////////////////////////////////
// default vhost of rtmp
#define SRS_CONSTS_RTMP_DEFAULT_VHOST "__defaultVhost__"
// default port of rtmp
#define SRS_CONSTS_RTMP_DEFAULT_PORT "1935"

// the default chunk size for system.
#define SRS_CONSTS_RTMP_SRS_CHUNK_SIZE 60000
// 6. Chunking, RTMP protocol default chunk size.
#define SRS_CONSTS_RTMP_PROTOCOL_CHUNK_SIZE 128

/**
* 6. Chunking
* The chunk size is configurable. It can be set using a control
* message(Set Chunk Size) as described in section 7.1. The maximum
* chunk size can be 65536 bytes and minimum 128 bytes. Larger values
* reduce CPU usage, but also commit to larger writes that can delay
* other content on lower bandwidth connections. Smaller chunks are not
* good for high-bit rate streaming. Chunk size is maintained
* independently for each direction.
*/
#define SRS_CONSTS_RTMP_MIN_CHUNK_SIZE 128
#define SRS_CONSTS_RTMP_MAX_CHUNK_SIZE 65536

// internal macros, covert macro values to str,
// see: read https://gcc.gnu.org/onlinedocs/cpp/Stringification.html#Stringification
#define __SRS_XSTR(v) __SRS_STR(v)
#define __SRS_STR(v) #v

/**
* the core provides the common defined macros, utilities,
* user must include the srs_core.hpp before any header, or maybe
* build failed.
*/

// for 32bit os, 2G big file limit for unistd io,
// ie. read/write/lseek to use 64bits size for huge file.
#ifndef _FILE_OFFSET_BITS
    #define _FILE_OFFSET_BITS 64
#endif

// for int64_t print using PRId64 format.
#ifndef __STDC_FORMAT_MACROS
    #define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

///////////////////////////////////////////////////////////
// SRS consts values
///////////////////////////////////////////////////////////
#define SRS_CONSTS_NULL_FILE "/dev/null"
#define SRS_CONSTS_LOCALHOST "127.0.0.1"

// free the p and set to NULL.
// p must be a T*.
#define srs_freep(p) \
    if (p) { \
        delete p; \
        p = NULL; \
    } \
    (void)0

#include <assert.h>
#define srs_assert(expression) assert(expression)

// compare
#define srs_min(a, b) (((a) < (b))? (a) : (b))
#define srs_max(a, b) (((a) < (b))? (b) : (a))
