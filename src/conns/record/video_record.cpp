#include "video_record.h"
#include "app_log.h"
#include "../conns/redis_process/redis_process.h"

VideoRecord::VideoRecord()
{
    has_init_ = false;
    stop_ = false;
    rc_pid_ = -1;

}

VideoRecord::~VideoRecord()
{

}
bool VideoRecord::init(std::string key, const char *hls_path, const char *tmp_ts_path, const char *mp4_path, const char* ffmpeg_cmd)
{
    if (0 == hls_path || 0 == hls_path || 0 == mp4_path || 0 == ffmpeg_cmd) {
        has_init_ = false;

        return false;
    } else {
        key_ = key;
        root_hls_ = hls_path;
        root_tmpts_ = tmp_ts_path;
        root_mp4_ = mp4_path;
        ffmpeg_cmd_ = ffmpeg_cmd;

        has_init_ = true;

        return true;
    }
}

bool VideoRecord::start_record(const char *stream_name, const char *publisher, const char *mp4filename)
{
    if (0 == stream_name || 0 == publisher || 0 == mp4filename) {
        return false;
    } else {
        stream_name_ = stream_name;
        publisher_ = publisher;
        mp4filename_ = mp4filename;
    }

    if (!has_init_) {
        return false;
    }

    if (!create_ts_folder()) {
        return false;
    }

    if (!create_mp4_folder()) {
        return false;
    }

    int ret = pthread_create(&rc_pid_, NULL, copy_ts, this);
    if (0 != ret) {
        srs_error("create start record thread failed.");
        return false;
    }

    return true;
}

bool VideoRecord::stop_record()
{
    if (!has_init_) {
        srs_warn("VideoRecord::stop_record, has not init.");
        return true;
    }

    {
        mu_.Lock();
        stop_ = true;
        mu_.UnLock();
    }

    pthread_join(rc_pid_, NULL);

    std::vector<std::string> tsfiles;
    get_all_ts(tsfiles);

    if (tsfiles.size() <= 0) {
        return true;
    }

    std::stringstream concat_ts;
    concat_ts << "concat:\"";
    int i = 0;
    for (i = 0; i < tsfiles.size() - 1; ++i) {
        std::stringstream ts_tmp;
        ts_tmp << ts_full_path_ << "/" << tsfiles[i];
        concat_ts << ts_tmp.str().c_str() << "|";
    }
    concat_ts << ts_full_path_ << "/" << tsfiles[i] << "\"";

    std::stringstream mp4;
    mp4 << mp4_full_path_ << "/" << mp4filename_;

    //convert the moov data to the head of mp4, so that flash player can support time transfer.

    std::stringstream transcode_cmd;
    transcode_cmd << ffmpeg_cmd_ << " "
                  << "-i "
                  << concat_ts.str().c_str() << " "
                  << "-vcodec "
                  << "copy "
                  << "-v "
                  << "quiet "
                  << "-y "
                  << mp4.str().c_str()
                  << " && "
                  << "./qt-faststart "
                  << mp4.str().c_str()
                  << " "
                  << mp4.str().c_str() << "_bak"
                  << " && "
                  << "mv -f "
                  << mp4.str().c_str() << "_bak"
                  << " "
                  << mp4.str().c_str();
    system(transcode_cmd.str().c_str());

    srs_trace("after transcode cmd, record file=%s", mp4.str().c_str());

    int cnt = 0;
    while (cnt++ < 172800) {
        if (0 == access(mp4.str().c_str(), F_OK)) {
            srs_trace("record file done. %s", mp4.str().c_str());
            break;
        }

        st_sleep(1);
    }

    if (172800 == cnt) {
        srs_error("record file not done.%s", mp4.str().c_str());
    }

    //delete all ts tmp files.
    std::stringstream del_ts_cmd;
    del_ts_cmd << "rm -rf " << ts_full_path_.c_str();
    system(del_ts_cmd.str().c_str());

    return true;
}

bool VideoRecord::create_ts_folder()
{
    if (!has_init_) {
        return false;
    }

    std::stringstream path;
    path << root_tmpts_ << "/" << key_.c_str();
    ts_full_path_ = path.str();
    if (0 == access(path.str().c_str(), F_OK)) {
        return true;
    }

    std::stringstream create_cmd;
    create_cmd << "mkdir -p " << path.str().c_str();

    const char* cmd = create_cmd.str().c_str();
    system(cmd);

    if (0 == access(path.str().c_str(), F_OK)) {
        return true;
    } else {
        return false;
    }
}

bool VideoRecord::create_mp4_folder()
{
    if (!has_init_) {
        return false;
    }

    std::stringstream path;
    path << root_mp4_ << "/" << publisher_;
    mp4_full_path_ = path.str();
    if (0 == access(path.str().c_str(), F_OK)) {
        return true;
    }

    std::stringstream create_cmd;
    create_cmd << "mkdir -p " << path.str().c_str();

    const char* cmd = create_cmd.str().c_str();
    system(cmd);

    if (0 == access(path.str().c_str(), F_OK)) {
        return true;
    } else {
        return false;
    }
}

void* copy_ts(void* data)
{
    VideoRecord *vr = (VideoRecord*)(data);
    if (NULL == vr) {
        return NULL;
    }

    std::string key = vr->key_;

    std::stringstream cmd;
    cmd << "EXISTS " << key.c_str();

    std::vector<std::string> last_tsfiles;//保存最新的ts文件列表

    while (!vr->stop_) {
        {
            vr->mu_.Lock();
            if (vr->stop_) {
                vr->mu_.UnLock();
                break;
            }
            vr->mu_.UnLock();
        }

        int res = 0;
        {
            g_mu_redis.Lock();
            g_redis->record_key_exist(res, DB_REDIS_RECORDINFO, cmd.str().c_str());
            g_mu_redis.UnLock();
        }

        if (0 == res) {
            srs_trace("when copy ts, no key in redis, key=%s", key.c_str());
            break;
        }

        vr->do_copy_job(last_tsfiles);
        sleep(10);
    }

    pthread_exit(NULL);
}

void VideoRecord::do_copy_job(std::vector<std::string> &last_tsfiles)
{
    //找到m3u8文件
    std::stringstream m3u8file;
    m3u8file << root_hls_ << "/live/" << stream_name_ << ".m3u8";
    const char* m3u8file_str = m3u8file.str().c_str();
    if (0 != access(m3u8file_str, F_OK)) {
        return;
    }

    //解析m3u8
    FILE* fm3u8 = fopen(m3u8file_str, "rb");
    if (0 == fm3u8) {
        return;
    }

    std::string str_tsinfo;
    enum {TSFILE_BUFF_SIZE = 512};
    char tsinfo[TSFILE_BUFF_SIZE] = {0};

    int readsize = 0;
    while (1) {
        readsize = fread(tsinfo, 1, TSFILE_BUFF_SIZE, fm3u8);
        str_tsinfo.append(tsinfo, readsize);
        bzero(tsinfo, TSFILE_BUFF_SIZE);
        if (readsize <=0) {
            break;
        }
    }

    if (0 != fm3u8){
        fclose(fm3u8);
    }

    std::vector<std::string> tsfiles;
    while (str_tsinfo.length() > 0) {
        int npos = str_tsinfo.find("\n");
        if (npos == std::string::npos) {
            break;
        }
        std::string ts;
        ts.assign(str_tsinfo.substr(0, npos));
        if(ts.find(".ts") != std::string::npos) {
            tsfiles.push_back(ts);
        }
        str_tsinfo.erase(0, npos + 1);
    }

    for (std::vector<std::string>::iterator iter = tsfiles.begin(); iter != tsfiles.end(); ++iter) {
        std::string ts = *iter;

        std::vector<std::string>::iterator find_iter ;
        for (find_iter = last_tsfiles.begin();
             find_iter != last_tsfiles.end(); ++find_iter) {
            if (*find_iter == ts) {
                break;
            }
        }

        if (find_iter != last_tsfiles.end()) { //yes, has find. do nothing.
            continue;
        }

        //not find ,means new file. do copy job.
        last_tsfiles.push_back(ts);

        std::stringstream ts_dst;
        ts_dst << ts_full_path_ << "/" << ts;
        std::stringstream ts_src;
        ts_src << root_hls_ << "/live/" << ts;

        time_t srcfile_time = 0;
        get_file_last_modify_time(ts_src.str(), srcfile_time);
        if (time(NULL) - srcfile_time > 300) {//more than 300
            continue;
        }

        if (0 == access(ts_dst.str().c_str(), F_OK)) {//ts临时目录中已经有了这个文件,
            //get the time of the two files, if  time interval > 30s, then copy it to tstmp directory.
            time_t dstfile_time = 0;
            get_file_last_modify_time(ts_dst.str(), dstfile_time);

            if (srcfile_time - dstfile_time > 30) { //more than 30s, should copy to the dst directory.
                std::stringstream copy_cmd;
                copy_cmd << "cp -f " << ts_src.str().c_str() << " " << ts_full_path_ << "/"
                         << ts << "_" << time(NULL) << ".ts";
                system(copy_cmd.str().c_str());
            }
        } else {
            std::stringstream copy_cmd;
            //judge the folder exist or not.
            if (0 != access(ts_full_path_.c_str(), F_OK)) {
                std::stringstream cmd;
                cmd << "mkdir -p " << ts_full_path_.c_str();
                system(cmd.str().c_str());
            }
            copy_cmd << "cp -f " << ts_src.str().c_str() << " " << ts_full_path_ << "/";
            system(copy_cmd.str().c_str());
        }
    }
}

void VideoRecord::get_all_ts(std::vector<std::string> &tsfiles)
{
    std::stringstream tslistfile;
    tslistfile << ts_full_path_ << "/" << "ts_file_list";

    std::stringstream cmd;
    cmd << "ls " << ts_full_path_ << " -lthr | awk '{print $NF}'" << " >> " << tslistfile.str().c_str();
    system(cmd.str().c_str());

    int cnt = 0;
    while (cnt++ < 3) {
        if (0 == access(tslistfile.str().c_str(), F_OK)) {
            break;
        }
        sleep(1);
    }

    std::string str_tsinfo;
    enum {TSFILE_BUFF_SIZE = 512};
    char tsbuffer[TSFILE_BUFF_SIZE] = {0};

    FILE* fts = fopen(tslistfile.str().c_str(), "rb");
    if (0 == fts) {
        return;
    }

    int readsize = 0;
    while (1) {
        readsize = fread(tsbuffer, 1, TSFILE_BUFF_SIZE, fts);
        str_tsinfo.append(tsbuffer, readsize);
        bzero(tsbuffer, TSFILE_BUFF_SIZE);
        if (readsize <= 0) {
            break;
        }
    }

    if (0 != fts) {
        fclose(fts);
    }

    std::stringstream del_tmp_tslist_cmd;
    del_tmp_tslist_cmd << "rm -f " << tslistfile.str().c_str();
    const char* del_cmd = del_tmp_tslist_cmd.str().c_str();
    system(del_cmd);

    while (str_tsinfo.length() > 0) {
        int npos = str_tsinfo.find("\n");
        if (npos == std::string::npos) {
            break;
        }
        std::string ts;
        ts.assign(str_tsinfo.substr(0, npos));
        if (ts.find(".ts") != std::string::npos)
        {
            tsfiles.push_back(ts);
        }
        str_tsinfo.erase(0, npos + 1);
    }
}

void VideoRecord::get_file_last_modify_time(std::string filename, time_t &modify_time)
{
    FILE * fp = 0;
    int fd = 0;
    struct stat buf;
    fp = fopen(filename.c_str(), "r");
    if (NULL == fp) {
        return;
    }

    fd = fileno(fp);
    fstat(fd, &buf);
//    int size = buf.st_size; //get file size (byte)
    modify_time = buf.st_mtime; //latest modification time (seconds passed from 01/01/00:00:00 1970 UTC)

//    printf("file size=%d\n",size);
//    printf("file last modify time=%d\n",modify_time);

    if (NULL != fp){
        fclose(fp);
    }
}
