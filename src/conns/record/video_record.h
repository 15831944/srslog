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
    bool init(const char* hls_path, const char* tmp_ts_path,
              const char* mp4_path, const char* ffmpeg_cmd);
    bool start_record(const char* stream_name, const char* publisher, const char* mp4filename);
    bool stop_record();
public:
    void do_copy_job(std::vector<std::string>&);
    bool stop_;//停止录像标志
    MutexLock mu_;

private:
    bool create_ts_folder();
    bool create_mp4_folder();
    bool create_recording_folder();
    void get_all_ts(std::vector<std::string> &tsfiles);
    void get_file_last_modify_time(std::string filename, time_t &modify_time);
public:
    std::string hls_path_; //hls路径，不包含live
    std::string tmp_ts_path_;//ts临时文件路径
    std::string mp4_path_;//生成mp4文件路径
    std::string recording_path_;//记录当前录像情况的路径
    std::string stream_name_;//流名
    std::string publisher_;//发布者
    std::string mp4filename_;//录像文件名
    std::string ffmpeg_cmd_;//ffmpeg bin文件。
    bool has_init_;//是否初始化标志
    std::string ts_full_path_;//ts文件全路径
    std::string mp4_full_path_;//mp4文件全路径
    pthread_t rc_pid_;
private:
    DISSALLOW_COPY_AND_ASSIGN(VideoRecord);
};

void* copy_ts(void*);
