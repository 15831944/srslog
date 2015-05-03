#pragma once

#include <string>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <map>
#include "app_error_code.h"
#include "app_reload_handler.h"
#include "app_macros.h"
#include "st.h"
#include "app_thread_context.h"
/**
* the log level, for example:
* if specified Debug level, all level messages will be logged.
* if specified Warn level, only Warn/Error/Fatal level messages will be logged.
*/
class SrsLogLevel
{
public:
    // only used for very verbose debug, generally,
    // we compile without this level for high performance.
    static const int Verbose = 0x01;
    static const int Info = 0x02;
    static const int Trace = 0x03;
    static const int Warn = 0x04;
    static const int Error = 0x05;
    // specified the disabled level, no log, for utest.
    static const int Disabled = 0x06;
};


/**
* the log interface provides method to write log.
* but we provides some macro, which enable us to disable the log when compile.
* @see also SmtDebug/SmtTrace/SmtWarn/SmtError which is corresponding to Debug/Trace/Warn/Fatal.
*/
class ISrsLog
{
public:
    ISrsLog();
    virtual ~ISrsLog();
public:
    /**
    * initialize log utilities.
    */
    virtual int initialize();
public:
    /**
    * log for verbose, very verbose information.
    */
    virtual void verbose(const char* tag, int context_id, const char* fmt, ...);
    /**
    * log for debug, detail information.
    */
    virtual void info(const char* tag, int context_id, const char* fmt, ...);
    /**
    * log for trace, important information.
    */
    virtual void trace(const char* tag, int context_id, const char* fmt, ...);
    /**
    * log for warn, warn is something should take attention, but not a error.
    */
    virtual void warn(const char* tag, int context_id, const char* fmt, ...);
    /**
    * log for error, something error occur, do something about the error, ie. close the connection,
    * but we will donot abort the program.
    */
    virtual void error(const char* tag, int context_id, const char* fmt, ...);
};

/**
* we use memory/disk cache and donot flush when write log.
* it's ok to use it without config, which will log to console, and default trace level.
* when you want to use different level, override this classs, set the protected _level.
*/
class SrsFastLog : public ISrsLog, public ISrsReloadHandler
{
// for utest to override
protected:
    // defined in SrsLogLevel.
    int _level;
private:
    char* log_data;
    // log to file if specified srs_log_file
    int fd;
    // whether log to file tank
    bool log_to_file_tank;
public:
    SrsFastLog();
    virtual ~SrsFastLog();
public:
    virtual int initialize();
    virtual void verbose(const char* tag, int context_id, const char* fmt, ...);
    virtual void info(const char* tag, int context_id, const char* fmt, ...);
    virtual void trace(const char* tag, int context_id, const char* fmt, ...);
    virtual void warn(const char* tag, int context_id, const char* fmt, ...);
    virtual void error(const char* tag, int context_id, const char* fmt, ...);
// interface ISrsThreadHandler.
public:
    virtual int on_reload_log_tank();
    virtual int on_reload_log_level();
    virtual int on_reload_log_file();
private:
    virtual bool generate_header(bool error, const char* tag, int context_id, const char* level_name, int* header_size);
    virtual void write_log(int& fd, char* str_log, int size, int level);
    virtual void open_log_file();
};

extern ISrsLog* _srs_log;

// donot print method
#if 1
    #define srs_verbose(msg, ...) _srs_log->verbose(NULL, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_info(msg, ...)    _srs_log->info(NULL, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_trace(msg, ...)   _srs_log->trace(NULL, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_warn(msg, ...)    _srs_log->warn(NULL, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_error(msg, ...)   _srs_log->error(NULL, _srs_context->get_id(), msg, ##__VA_ARGS__)
// use __FUNCTION__ to print c method
#elif 0
    #define srs_verbose(msg, ...) _srs_log->verbose(__FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_info(msg, ...)    _srs_log->info(__FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_trace(msg, ...)   _srs_log->trace(__FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_warn(msg, ...)    _srs_log->warn(__FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_error(msg, ...)   _srs_log->error(__FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
// use __PRETTY_FUNCTION__ to print c++ class:method
#else
    #define srs_verbose(msg, ...) _srs_log->verbose(__PRETTY_FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_info(msg, ...)    _srs_log->info(__PRETTY_FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_trace(msg, ...)   _srs_log->trace(__PRETTY_FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_warn(msg, ...)    _srs_log->warn(__PRETTY_FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_error(msg, ...)   _srs_log->error(__PRETTY_FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
#endif
