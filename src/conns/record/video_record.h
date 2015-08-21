#pragma once

#include <string>
#include <semaphore.h>
#include <vector>
#include <dirent.h>
#include <sstream>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "mutexlock.h"

class VideoRecord
{
public:
    VideoRecord();
    ~VideoRecord();

public:
    bool init(std::string key,
              const char* hls_path,
              const char* tmp_ts_path,
              const char* mp4_path,
              const char* ffmpeg_cmd);
    bool start_record(const char* stream_name, const char* publisher);
    bool stop_record();
public:
    void do_copy_job();
    bool stop_;//停止录像标志
    MutexLock mu_;

private:
    bool create_m3u8_folder();
    bool create_mp4_folder();
    void get_all_ts(std::vector<std::string> &tsfiles);
    void get_file_last_modify_time(std::string filename, time_t &modify_time);
public:
    std::string key_;
    std::string root_hls_; //hls路径，不包含live
    std::string root_m3u8_;//m3u8文件路径
    std::string root_mp4_;//生成mp4文件路径

    std::string stream_name_;//流名
    std::string publisher_;//发布者
    std::string ffmpeg_cmd_;//ffmpeg bin文件。

    bool has_init_;//是否初始化标志
    bool has_write_m3u8_header_;
    bool first_copy_ts_;
    pthread_t rc_pid_;
private:
    DISSALLOW_COPY_AND_ASSIGN(VideoRecord);
};

void* copy_ts(void*);