#include "app_screenshot_conn.h"
#include "app_config.h"
#include "app_log.h"
#include "app_error_code.h"
#include <stdio.h>
#include <stdlib.h>
#include "app_json.h"
#include <dirent.h>
#include "execv_ffmpeg.h"
#include <sstream>
#include "jpg2base64.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>


SrsScreenShotConn::SrsScreenShotConn(SrsServer *srs_server, st_netfd_t client_stfd)
    : SrsConnection(srs_server, client_stfd)

{
    type_ = SrsConnUnknown;
    skt = new SrsStSocket(client_stfd);
    screenshot = new SrsScreenShotServer(skt);
    _srs_config->subscribe(this);
}

SrsScreenShotConn::~SrsScreenShotConn()
{
    type_ = SrsConnUnknown;

    _srs_config->unsubscribe(this);
    srs_freep(skt);
    srs_freep(screenshot);
}

void SrsScreenShotConn::kbps_resample()
{

}

int64_t SrsScreenShotConn::get_send_bytes_delta()
{
    return 0;
}

int64_t SrsScreenShotConn::get_recv_bytes_delta()
{
    return 0;
}

int SrsScreenShotConn::do_cycle()
{
    int ret = ERROR_SUCCESS;

    enum {HEAD_BUFFER_LEN = 17};
    char head_buffer[HEAD_BUFFER_LEN] = {'\0'};

    if (NULL == screenshot) {
        ret = ERROR_POINT_NULL;
        srs_error("NULL == screenshot");
        return ret;
    }

    screenshot->set_recv_timeout(SRS_CONSTS_ACCOUNT_RECV_TIMEOUT_US);
    screenshot->set_send_timeout(SRS_CONSTS_ACCOUNT_SEND_TIMEOUT_US);

    srs_trace("begin recv client head data.");

    //recv head.
    ssize_t nsize = 0;
    if ((ret = skt->read(head_buffer, HEAD_BUFFER_LEN - 1, &nsize)) != ERROR_SUCCESS) {
        srs_error("read screenshot head timeout. ret=%d", ret);
        return ret;
    }

    srs_trace("recv client head data success.");

    int json_len = atoi(head_buffer);
    enum {JOSN_BODY_LEN = 4096};
    char json_buff[JOSN_BODY_LEN] = {'\0'};
    //recv json body.
    nsize = 0;
    if ((ret = skt->read(json_buff, json_len, &nsize)) != ERROR_SUCCESS) {
        srs_error("read screen json body timeout. ret=%d", ret);
        return ret;
    }

    srs_trace("recv client origin:%s", json_buff);

    ClientReqData clientdata;
    parse_client_data(json_buff, json_len, clientdata);

    srs_trace("parse data: action=%s,app=%s,stream=%s,file_status=%s, time_offset=%s",
              clientdata.action.c_str(),
              clientdata.app.c_str(),
              clientdata.stream.c_str(),
              clientdata.file_status.c_str(),
              clientdata.time_offset.c_str());

    if (0 == strcmp("get_picture", clientdata.action.c_str()))
    {
        do_screen_shot_job(clientdata);
    }
    else if (0 == strcmp("get_vod_file_status", clientdata.action.c_str()))
    {
        do_check_vod_file_status(clientdata);
    }
    return ret;
}

void SrsScreenShotConn::parse_client_data(char *json_data, int len, ClientReqData &clientdata)
{
    if (!parse_json(json_data, len, clientdata)) {
        srs_error("do_screen_shot_job:parse json failed.");
        return ;
    }
}

void SrsScreenShotConn::do_screen_shot_job(const ClientReqData &screenshotdata)
{
    srs_trace("do_screen_shot_job in----");
    std::stringstream jpgfile;
    jpgfile << "/var/hls/" << screenshotdata.app << "/" <<screenshotdata.stream << ".jpg";

    std::stringstream videofile;
    //live mode
    if (0 == strcmp("live",screenshotdata.app.c_str()))
    {
        std::string tsfile;
        if (!get_tsfile(screenshotdata.stream.c_str(), tsfile)) {
            srs_error("do_screen_shot_job: get ts file failed. stream name=%s", screenshotdata.stream.c_str());
            return ;
        }
        videofile << "/var/hls/" << screenshotdata.app << "/" << tsfile;
        //shot a picture from ts file. jpg format.
        shot_picture(const_cast<char *>(videofile.str().c_str()),
                     const_cast<char *>(jpgfile.str().c_str()),
                     "3");
    }
    else if (0 == strcmp("vod", screenshotdata.app.c_str())) //vod mode
    {
        videofile << "/var/hls/" << screenshotdata.app << "/" << screenshotdata.stream;//vod file name.
        shot_picture(const_cast<char *>(videofile.str().c_str()),
                     const_cast<char *>(jpgfile.str().c_str()),
                     const_cast<char *>(screenshotdata.time_offset.c_str()));
    }


    usleep(1000 * 200);

    if (jpgfile.str().length() <= 0)
    {
        srs_error("shot pic to disk failed.");
        return;
    }

    //jpg to base64
    Jpg2Base64 jb;
    char *buff_base64 = new char[1024 * 1024];
    int base64_len = 0;
    jb.Convert(const_cast<char *>(jpgfile.str().c_str()), buff_base64, base64_len);
    if (base64_len <= 0)
    {
        srs_error("convert to base64 failed.");
    }

    //make send package
    std::stringstream res;
    make_screen_shot_pack(screenshotdata, buff_base64, base64_len, res);

    //send to client.
    ssize_t actually_write = 0;
    int ret = ERROR_SUCCESS;
    if (ret = skt->write(const_cast<char *>(res.str().c_str()), res.str().length(), &actually_write)) {
        srs_error("do get screen shot error, ret=%d", ret);
    }

    remove(jpgfile.str().c_str());
    if (NULL != buff_base64)
    {
        delete [] buff_base64;
        buff_base64 = NULL;
    }
}

void SrsScreenShotConn::do_check_vod_file_status(ClientReqData &clientdata)
{
    std::stringstream videofile;
    videofile << "/var/hls/vod/" << clientdata.stream;//vod file name.
    int ret = is_file_exist(videofile.str().c_str());
    if (0 == ret) //exist
    {
        clientdata.file_status = "1";
    }
    else//not exist
    {
        clientdata.file_status = "0";
    }

    std::stringstream res;
    make_file_status_pack(clientdata, res);

    //send to client.
    ssize_t actually_write = 0;
    int ret2 = ERROR_SUCCESS;
    if (ret2 = skt->write(const_cast<char *>(res.str().c_str()), res.str().length(), &actually_write)) {
        srs_error("do get vod file status error, ret=%d", ret);
    }
}

bool SrsScreenShotConn::parse_json(char *json_data, int len, ClientReqData &res)
{
    srs_trace("parse_json, data:%s", json_data);
    const nx_json* js = nx_json_parse_utf8(json_data);
    if (NULL == js){
        return false;
    }
    const nx_json* js_action = nx_json_get(js, "action");
    if (NULL == js_action) {
        return false;
    }
    res.action = js_action->text_value;

    const nx_json* js_params = nx_json_get(js, "params");
    if (NULL == js_params) {
        return false;
    }

    const nx_json* js_item = nx_json_item(js_params, 0);
    if (NULL == js_item) {
        return false;
    }

    const nx_json* js_app = nx_json_get(js_item, "app");
    if (NULL == js_app) {
        return false;
    }
    res.app = js_app->text_value;

    const nx_json* js_stream = nx_json_get(js_item, "stream");
    if (NULL == js_stream) {
        return false;
    }
    res.stream = js_stream->text_value;

    const nx_json* js_timeoffset = nx_json_get(js_item, "time_offset");
    if (NULL != js_timeoffset) {
        if (NULL != js_timeoffset->text_value)
        {
            res.time_offset = js_timeoffset->text_value;
        }
    }

    if (NULL != js) {
        nx_json_free(js);
    }

    return true;
}

bool SrsScreenShotConn::get_tsfile(const char *stream, std::string &file_name)
{
    std::vector<std::string> res;
    bool ret = ListDirectoryFile("/var/hls/live", res);
    if (!ret || res.size() <= 0) {
        return false;
    }       

    std::vector<std::string> res_tsfiles;
    for (int i = 0; i < res.size(); ++i) {
        if (NULL != strstr(res.at(i).c_str(), stream)) {
            res_tsfiles.push_back(res.at(i));
        }
    }

    if (res_tsfiles.size() <= 0)
    {
        srs_error("res_tsfiles size==0");
        return false;
    }

    int filesize = 0;
    int last_modify = 0;
    int index = 0;
    for (int j = 0; j < res_tsfiles.size(); ++j) {
        int tmp_modify = 0;
        std::stringstream ss;
        ss << "/var/hls/live/" << res.at(j).c_str();
        get_file_size_time(ss.str().c_str(), filesize, tmp_modify);
        if (tmp_modify > last_modify)
        {
            last_modify = tmp_modify;
            index = j;
        }
    }

    if (res_tsfiles.size() >= index)
    {
        file_name = res_tsfiles[index];
    }
    else
    {
        return false;
    }

    return true;
}

bool SrsScreenShotConn::shot_picture(char *video_name, char *jpg_name, char *time_offset)
{
    Execv_ffmpeg ff;
    ff.SetCmd("./ffmpeg/ffmpeg");
    ff.set_params(video_name, jpg_name, time_offset);
    ff.start();
    ff.cycle();
    ff.stop();

    return true;
}

void SrsScreenShotConn::make_screen_shot_pack(const ClientReqData &screenshotdata, char *buff_base64, int len_base64, std::stringstream &res)
{
    std::stringstream datas;

    datas << __SRS_JOBJECT_START
            << __SRS_JFIELD_STR("app", screenshotdata.app.c_str()) << __SRS_JFIELD_CONT
            << __SRS_JFIELD_STR("stream", screenshotdata.stream.c_str()) << __SRS_JFIELD_CONT
            << __SRS_JFIELD_STR("picture", buff_base64)
            << __SRS_JOBJECT_END;

    res << __SRS_JOBJECT_START
            << __SRS_JFIELD_STR("succ", true) << __SRS_JFIELD_CONT
            << __SRS_JFIELD_STR("msg", "get picture result.") << __SRS_JFIELD_CONT
            << __SRS_JFIELD_NAME("data") << __SRS_JARRAY_START
            << datas.str().c_str()
            << __SRS_JARRAY_END
        << __SRS_JOBJECT_END
           ;
}

void SrsScreenShotConn::make_file_status_pack(const ClientReqData &clientdata, std::stringstream &res)
{
    std::stringstream datas;

    datas << __SRS_JOBJECT_START
            << __SRS_JFIELD_STR("app", clientdata.app.c_str()) << __SRS_JFIELD_CONT
            << __SRS_JFIELD_STR("stream", clientdata.stream.c_str()) << __SRS_JFIELD_CONT
            << __SRS_JFIELD_STR("status", clientdata.file_status.c_str())
            << __SRS_JOBJECT_END;

    res << __SRS_JOBJECT_START
            << __SRS_JFIELD_STR("succ", true) << __SRS_JFIELD_CONT
            << __SRS_JFIELD_STR("msg", "get vod file status result.") << __SRS_JFIELD_CONT
            << __SRS_JFIELD_NAME("data") << __SRS_JARRAY_START
            << datas.str().c_str()
            << __SRS_JARRAY_END
        << __SRS_JOBJECT_END
           ;
}

bool ListDirectoryFile( char *path, std::vector<std::string>& vec_files)
{
    DIR *dp ;
    struct dirent *dirp;

    if( (dp = opendir( path )) == NULL )
    {
        return false;
    }

    while( ( dirp = readdir( dp ) ) != NULL)
    {
        if(strcmp(dirp->d_name,".")==0  || strcmp(dirp->d_name,"..")==0)
            continue;

        int size = strlen(dirp->d_name);

        //for ts file, len at least 4.
        if(size < 4)
            continue;

        if(strcmp( ( dirp->d_name + (size - 3) ) , ".ts") != 0)
            continue;

        std::string filename = dirp->d_name;
        vec_files.push_back(filename);
    }

    return true;
}

int is_file_exist(const char *file_path)
{
    if (NULL == file_path)
        return -1;
    if (0 == access(file_path, F_OK))
        return 0;
    return -1;
}

int get_file_size_time (const char *filename, int &filesize, int &last_modify)
{  
    struct stat statbuf;
    if (stat (filename, &statbuf) == -1)
    {
        return (-1);
    }

    if (S_ISDIR (statbuf.st_mode))
        return (1);

    if (S_ISREG (statbuf.st_mode))
    {
        // printf ("%s size：%ld bytes\tmodified at %s",
        // filename, statbuf.st_size, ctime(&statbuf.st_mtime)); 字符串的形式
        //        printf ("%s size：%ld bytes\tmodified at %d\n",
        //                filename, statbuf.st_size, (statbuf.st_mtime));//utc时间形式
        filesize = statbuf.st_size;
        last_modify = (statbuf.st_mtime);
    }

    return (0);
}
