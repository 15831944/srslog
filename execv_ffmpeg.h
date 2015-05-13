#pragma once

#include <vector>
#include <unistd.h>

enum {PARAM_LEN = 64};
class Execv_ffmpeg
{
public:
    Execv_ffmpeg();
    ~Execv_ffmpeg();
private:
    char *cmd_;
    char *params_[PARAM_LEN];
    bool started;
    pid_t pid;
    int status_;
public:
    void SetCmd(char *cmd);
    void set_params(char *ts_file_name, char *jpg_name, char *time_offset);
    void set_params_vod(char *ts_file_name, char *jpg_name, char* time_offset);
    int start();
    int cycle();
    void stop();
};

int get_file_size_time (const char *filename, int &filesize, int &last_modify);
int generate_rand(int X, int Y);
