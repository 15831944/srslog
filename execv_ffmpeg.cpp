#include "execv_ffmpeg.h"
#include<sys/types.h>
#include<sys/wait.h>
#include "app_log.h"
#include <sstream>
#include<stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
# include <stdlib.h>
# include <time.h>
#include <unistd.h>
#include <string.h>

Execv_ffmpeg::Execv_ffmpeg() : cmd_(NULL), started(false)
{
    bzero(params_, PARAM_LEN);
}

Execv_ffmpeg::~Execv_ffmpeg()
{
    stop();
}

void Execv_ffmpeg::SetCmd(char *cmd)
{
    cmd_ = cmd;
}

void Execv_ffmpeg::set_params(char *video_file_name, char *jpg_name, char *time_offset)
{      
    params_[0] = cmd_;
    params_[1] = ("-loglevel");
    params_[2] = "-8";
    params_[3] = (  "-i");
    params_[4] = (video_file_name);
    params_[5] = ("-y");
    params_[6] = ("-f");
    params_[7] = ("image2");
    params_[8] = ("-ss");
    params_[9] = (time_offset);
    params_[10] = ("-vframes");
    params_[11] = ("1");
    params_[12] = (jpg_name);
    params_[13] = (NULL);
}

void Execv_ffmpeg::set_params_vod(char *video_file_name, char *jpg_name, char *time_offset)
{

}

int Execv_ffmpeg::start()
{
    int ret = 0;
    if (started) {
        return ret;
    }

    if (pid = vfork())
    {
        return -1;
    }

    if (0 == pid)//child
    {
        execv(cmd_, params_);
    }

    if (pid > 0)//parent
    {
        started = true;
    }

    return ret;
}

int Execv_ffmpeg::cycle()
{
    int ret = 0;

    if (!started) {
        return ret;
    }

    status_ = 0;
    pid_t p = waitpid(pid, &status_, WNOHANG);

    if (p < 0) {
        ret = -1;
        return ret;
    }

    if (p == 0) {
        return ret;
    }

    started = false;

    return ret;
}

void Execv_ffmpeg::stop()
{
    if (!started) {
        return;
    }

    if (pid > 0) {
        if (kill(pid, SIGKILL) < 0) {
        }

        status_ = 0;
        if (waitpid(pid, &status_, 0) < 0) {
            std::stringstream ss;
            ss << cmd_ << " ";
            for (int i = 0; i < PARAM_LEN; ++i)
            {
                if (params_[i] != 0)
                {
                    ss << params_[i] << " ";
                }
            }

            srs_error("execv_ffmpeg error, cmd=%s, err=%d", ss.str().c_str(), errno);
        }

        pid = -1;
    }

}

int generate_rand(int X, int Y)
{
    srand((unsigned)time(NULL)); /*随机种子*/
    int n=rand()%(Y-X+1)+X; /*n为X~Y之间的随机数*/
    return n;
}
