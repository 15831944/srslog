#include "app_log.h"
using namespace std;

#include "app_utility.h"
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../conf/config_info.h"

ISrsLog::ISrsLog()
{
}

ISrsLog::~ISrsLog()
{
}

int ISrsLog::initialize()
{
    return ERROR_SUCCESS;
}

void ISrsLog::verbose(const char* /*tag*/, int /*context_id*/, const char* /*fmt*/, ...)
{
}

void ISrsLog::info(const char* /*tag*/, int /*context_id*/, const char* /*fmt*/, ...)
{
}

void ISrsLog::trace(const char* /*tag*/, int /*context_id*/, const char* /*fmt*/, ...)
{
}

void ISrsLog::warn(const char* /*tag*/, int /*context_id*/, const char* /*fmt*/, ...)
{
}

void ISrsLog::error(const char* /*tag*/, int /*context_id*/, const char* /*fmt*/, ...)
{
}

// the max size of a line of log.
#define LOG_MAX_SIZE 4096

// the tail append to each log.
#define LOG_TAIL '\n'
// reserved for the end of log data, it must be strlen(LOG_TAIL)
#define LOG_TAIL_SIZE 1

SrsFastLog::SrsFastLog()
{
    _level = SrsLogLevel::Trace;
    log_data = new char[LOG_MAX_SIZE];

    fd = -1;
    log_to_file_tank = false;
}

SrsFastLog::~SrsFastLog()
{
    srs_freep(log_data);

    if (fd > 0) {
        ::close(fd);
        fd = -1;
    }
}

int SrsFastLog::initialize()
{
    int ret = ERROR_SUCCESS;

    if (g_config) {
        log_to_file_tank = g_config->get_log_tank_file();
        _level = srs_get_log_level(g_config->get_log_level());
    }

    return ret;
}

void SrsFastLog::verbose(const char* tag, int context_id, const char* fmt, ...)
{
    if (_level > SrsLogLevel::Verbose) {
        return;
    }

    int size = 0;
    if (!generate_header(false, tag, context_id, "verb", &size)) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    // we reserved 1 bytes for the new line.
    size += vsnprintf(log_data + size, LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);

    write_log(fd, log_data, size, SrsLogLevel::Verbose);
}

void SrsFastLog::info(const char* tag, int context_id, const char* fmt, ...)
{
    if (_level > SrsLogLevel::Info) {
        return;
    }

    int size = 0;
    if (!generate_header(false, tag, context_id, "debug", &size)) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    // we reserved 1 bytes for the new line.
    size += vsnprintf(log_data + size, LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);

    write_log(fd, log_data, size, SrsLogLevel::Info);
}

void SrsFastLog::trace(const char* tag, int context_id, const char* fmt, ...)
{
    if (_level > SrsLogLevel::Trace) {
        return;
    }

    int size = 0;
    if (!generate_header(false, tag, context_id, "trace", &size)) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    // we reserved 1 bytes for the new line.
    size += vsnprintf(log_data + size, LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);

    write_log(fd, log_data, size, SrsLogLevel::Trace);
}

void SrsFastLog::warn(const char* tag, int context_id, const char* fmt, ...)
{
    if (_level > SrsLogLevel::Warn) {
        return;
    }

    int size = 0;
    if (!generate_header(true, tag, context_id, "warn", &size)) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    // we reserved 1 bytes for the new line.
    size += vsnprintf(log_data + size, LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);

    write_log(fd, log_data, size, SrsLogLevel::Warn);
}

void SrsFastLog::error(const char* tag, int context_id, const char* fmt, ...)
{
    if (_level > SrsLogLevel::Error) {
        return;
    }

    int size = 0;
    if (!generate_header(true, tag, context_id, "error", &size)) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    // we reserved 1 bytes for the new line.
    size += vsnprintf(log_data + size, LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);

    // add strerror() to error msg.
    if (errno != 0) {
        size += snprintf(log_data + size, LOG_MAX_SIZE - size, "(%s)", strerror(errno));
    }

    write_log(fd, log_data, size, SrsLogLevel::Error);
}

int SrsFastLog::on_reload_log_tank()
{
    int ret = ERROR_SUCCESS;

    if (!g_config) {
        return ret;
    }

    bool tank = log_to_file_tank;
    log_to_file_tank = g_config->get_log_tank_file();

    if (tank) {
        return ret;
    }

    if (!log_to_file_tank) {
        return ret;
    }

    if (fd > 0) {
        ::close(fd);
    }
    open_log_file();

    return ret;
}

int SrsFastLog::on_reload_log_level()
{
    int ret = ERROR_SUCCESS;

    if (!g_config) {
        return ret;
    }

    _level = srs_get_log_level(g_config->get_log_level());

    return ret;
}

int SrsFastLog::on_reload_log_file()
{
    int ret = ERROR_SUCCESS;

    if (!g_config) {
        return ret;
    }

    if (!log_to_file_tank) {
        return ret;
    }

    if (fd > 0) {
        ::close(fd);
    }
    open_log_file();

    return ret;
}

bool SrsFastLog::generate_header(bool error, const char* tag, int context_id, const char* level_name, int* header_size)
{
    // clock time
    timeval tv;
    if (gettimeofday(&tv, NULL) == -1) {
        return false;
    }

    // to calendar time
    struct tm* tm;
    if ((tm = localtime(&tv.tv_sec)) == NULL) {
        return false;
    }

    // write log header
    int log_header_size = -1;

    if (error) {
        if (tag) {
            log_header_size = snprintf(log_data, LOG_MAX_SIZE,
                "[%d-%02d-%02d %02d:%02d:%02d.%03d][%s][%s][%d][%d][%d] ",
                1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000),
                level_name, tag, getpid(), context_id, errno);
        } else {
            log_header_size = snprintf(log_data, LOG_MAX_SIZE,
                "[%d-%02d-%02d %02d:%02d:%02d.%03d][%s][%d][%d][%d] ",
                1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000),
                level_name, getpid(), context_id, errno);
        }
    } else {
        if (tag) {
            log_header_size = snprintf(log_data, LOG_MAX_SIZE,
                "[%d-%02d-%02d %02d:%02d:%02d.%03d][%s][%s][%d][%d] ",
                1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000),
                level_name, tag, getpid(), context_id);
        } else {
            log_header_size = snprintf(log_data, LOG_MAX_SIZE,
                "[%d-%02d-%02d %02d:%02d:%02d.%03d][%s][%d][%d] ",
                1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000),
                level_name, getpid(), context_id);
        }
    }

    if (log_header_size == -1) {
        return false;
    }

    // write the header size.
    *header_size = srs_min(LOG_MAX_SIZE - 1, log_header_size);

    return true;
}

void SrsFastLog::write_log(int& fd, char *str_log, int size, int level)
{
    // ensure the tail and EOF of string
    //      LOG_TAIL_SIZE for the TAIL char.
    //      1 for the last char(0).
    size = srs_min(LOG_MAX_SIZE - 1 - LOG_TAIL_SIZE, size);

    // add some to the end of char.
    str_log[size++] = LOG_TAIL;

    // if not to file, to console and return.
    if (!log_to_file_tank) {
        // if is error msg, then print color msg.
        // \033[31m : red text code in shell
        // \033[32m : green text code in shell
        // \033[33m : yellow text code in shell
        // \033[0m : normal text code
        if (level <= SrsLogLevel::Trace) {
            printf("%.*s", size, str_log);
        } else if (level == SrsLogLevel::Warn) {
            printf("\033[33m%.*s\033[0m", size, str_log);
        } else{
            printf("\033[31m%.*s\033[0m", size, str_log);
        }

        return;
    }

    // open log file. if specified
    if (fd < 0) {
        open_log_file();
    }

    // write log to file.
    if (fd > 0) {
        ::write(fd, str_log, size);
    }
}

void SrsFastLog::open_log_file()
{
    if (!g_config) {
        return;
    }

    std::string filename = g_config->get_log_file_name();

    if (filename.empty()) {
        return;
    }

    fd = ::open(filename.c_str(), O_RDWR | O_APPEND);

    if(fd == -1 && errno == ENOENT) {
        fd = open(filename.c_str(),
            O_RDWR | O_CREAT | O_TRUNC,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH
        );
    }
}


ISrsLog* _srs_log = new SrsFastLog();
