#include "execv_ffmpeg.h"
#include<sys/types.h>
#include<sys/wait.h>

Execv_ffmpeg::Execv_ffmpeg() : cmd_(NULL), started(false)
{

}

Execv_ffmpeg::~Execv_ffmpeg()
{
    stop();
}

void Execv_ffmpeg::SetCmd(char *cmd)
{
    cmd_ = cmd;
}

void Execv_ffmpeg::SetParamsScreenShot(char *ts_file_name, char *jpg_name)
{
    params_[0] = cmd_;
    params_[1] = (  "-i");
    params_[2] = (ts_file_name);
    params_[3] = ("-y");
    params_[4] = ("-f");
    params_[5] = ("image2");
    params_[6] = ("-ss");
    params_[7] = ("1");
    params_[8] = ("-vframes");
    params_[9] = ("1");
    params_[10] = (jpg_name);
    params_[11] = (NULL);
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

        }

        pid = -1;
    }
}
