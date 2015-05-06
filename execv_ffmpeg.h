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
    void SetParamsScreenShot(char *ts_file_name, char *jpg_name);
    int start();
    int cycle();
    void stop();
};
