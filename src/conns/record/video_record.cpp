#include "video_record.h"
#include "app_log.h"
#include "../conns/redis_process/redis_process.h"

VideoRecord::VideoRecord()
{
    has_init_ = false;
    stop_ = false;
    rc_pid_ = -1;
    has_write_m3u8_header_ = false;
    first_copy_ts_ = true;
}

VideoRecord::~VideoRecord()
{

}
bool VideoRecord::init(std::string key,
                       const char *hls_path,
                       const char *m3u8_path,
                       const char *mp4_path,
                       const char* ffmpeg_cmd)
{
    if (0 == key.length() || 0 == hls_path ||
            0 == hls_path || 0 == mp4_path ||
            0 == ffmpeg_cmd) {
        has_init_ = false;
        return false;
    } else {
        key_ = key;

        root_hls_ = hls_path;
        root_m3u8_ = m3u8_path;
        root_mp4_ = mp4_path;

        ffmpeg_cmd_ = ffmpeg_cmd;
        has_init_ = true;

        return true;
    }
}

bool VideoRecord::start_record(const char *stream_name, const char *publisher)
{
    if (0 == stream_name || 0 == publisher) {
        return false;
    } else {
        stream_name_ = stream_name;
        publisher_ = publisher;
    }

    if (!has_init_) {
        return false;
    }

    if (!create_m3u8_folder()) {
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

    //add endlist to m3u8 file
    std::stringstream addendlist;
    addendlist << "echo " << "\"#EXT-X-ENDLIST\"" << " >> " << root_m3u8_ << "/" << key_.c_str() << ".m3u8";
    system(addendlist.str().c_str());

    std::stringstream concat_ts;
    concat_ts << "concat:\"";
    int i = 0;
    for (i = 0; i < tsfiles.size() - 1; ++i) {
        std::stringstream ts_tmp;
        ts_tmp << root_m3u8_.c_str() << "/" << tsfiles[i];
        concat_ts << ts_tmp.str().c_str() << "|";
    }
    concat_ts << root_m3u8_.c_str() << "/" << tsfiles[i] << "\"";

    std::stringstream mp4;
    mp4 << root_mp4_.c_str() << "/" << key_.c_str() << ".mp4";

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

    return true;
}

bool VideoRecord::create_m3u8_folder()
{
    if (!has_init_) {
        return false;
    }

    if (0 == access(root_m3u8_.c_str(), F_OK)) {
        return true;
    }

    std::stringstream create_cmd;
    create_cmd << "mkdir -p " << root_m3u8_.c_str();

    const char* cmd = create_cmd.str().c_str();
    system(cmd);

    if (0 == access(root_m3u8_.c_str(), F_OK)) {
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

    if (0 == access(root_mp4_.c_str(), F_OK)) {
        return true;
    }

    std::stringstream create_cmd;
    create_cmd << "mkdir -p " << root_mp4_.c_str();

    const char* cmd = create_cmd.str().c_str();
    system(cmd);

    if (0 == access(root_mp4_.c_str(), F_OK)) {
        return true;
    } else {
        return false;
    }
}

void* copy_ts(void* data)
{
    VideoRecord *vr = (VideoRecord*)(data);
    if (NULL == vr) {
        srs_error("copy_ts: NULL == data.");
        return NULL;
    }

    std::string key = vr->key_;

    std::stringstream cmd;
    cmd << "EXISTS " << key.c_str();

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

        vr->do_copy_job();

        sleep(10);
    }

    pthread_exit(NULL);
}

void VideoRecord::do_copy_job()
{
    std::stringstream m3u8file;
    m3u8file << root_hls_.c_str() << "/live/" << stream_name_.c_str() << ".m3u8";
    if (0 != access(m3u8file.str().c_str(), F_OK)) {
        srs_error("do_copy_job: can not find m3u8 file, make sure living ?");
        return;
    }

    FILE* fm3u8 = fopen(m3u8file.str().c_str(), "rb");
    if (0 == fm3u8) {
        srs_error("do_copy_job: open m3u8 file failed.");
        return;
    }

    std::string str_tsinfo;
    enum {TSFILE_BUFF_SIZE = 1024};
    char tsinfo_buff[TSFILE_BUFF_SIZE] = {0};

    int readsize = 0;
    while (1) {
        readsize = fread(tsinfo_buff, 1, TSFILE_BUFF_SIZE, fm3u8);
        str_tsinfo.append(tsinfo_buff, readsize);
        bzero(tsinfo_buff, TSFILE_BUFF_SIZE);
        if (readsize <= 0) {
            break;
        }
    }

    if (0 != fm3u8){
        fclose(fm3u8);
    }

    std::vector<std::string> tsinfo_single;
    while (str_tsinfo.length() > 0) {
        int npos = str_tsinfo.find("\n");
        if (npos == std::string::npos) {
            break;
        }

        std::string single;
        single.assign(str_tsinfo.substr(0, npos));
        tsinfo_single.push_back(single);

        str_tsinfo.erase(0, npos + 1);
    }
    //m3u8 file parse done.

    //write m3u8 header.
    int i = 0;
    enum {M3U8_HEADER_LINE = 5};
    if (!has_write_m3u8_header_) {
        if (tsinfo_single.size() < M3U8_HEADER_LINE) {
            srs_error("tsinfo_single.size() < M3U8_HEADER_LINE");
            return;
        }

        for (;i < M3U8_HEADER_LINE;++i) {
            std::string single = tsinfo_single.at(i);
            std::stringstream info;
            info << "echo " << "\"" << single.c_str() << "\"" <<
                    " >> " << root_m3u8_ << "/" << key_.c_str() << ".m3u8";
            system(info.str().c_str());
        }

        has_write_m3u8_header_ = true;
    }

    std::string extinf;
    std::string tsfile;
    for (;i < tsinfo_single.size(); ++i) {
        std::string single = tsinfo_single.at(i);

        //m3u8 header.
        if (single.find("EXTINF") != std::string::npos) {
            extinf = single;
            continue;
        }

        if (single.find(".ts") != std::string::npos) {
            tsfile = single;
        }

        //handle ts file.
        std::stringstream ts_src;
        ts_src << root_hls_.c_str() << "/live/" << single;
        std::stringstream ts_dst;
        ts_dst << root_m3u8_.c_str() << "/" << single;

        //judge if the first time.
        int timeinterval = 150;
        if (first_copy_ts_){
            timeinterval = 35;
            first_copy_ts_ = false;
        }

        time_t srcfile_time = 0;
        get_file_last_modify_time(ts_src.str(), srcfile_time);
        if (time(NULL) - srcfile_time > timeinterval) {//in case more time on begin. judge on src file.
            continue;
        }

        if (0 == access(ts_dst.str().c_str(), F_OK)) {
            time_t dstfile_time = 0;
            get_file_last_modify_time(ts_dst.str(), dstfile_time);

            if (srcfile_time - dstfile_time > 150) {//judge on dst file.
                std::stringstream new_ts;//rename.
                int npos = single.find(".ts");
                if (npos != std::string::npos) {
                    new_ts << single.substr(0, npos).c_str() << "_" << time(NULL) << ".ts";
                }

                std::stringstream copy_cmd;
                copy_cmd << "cp -f " << ts_src.str().c_str() << " " << root_m3u8_.c_str() << "/"
                         << new_ts.str().c_str();
                system(copy_cmd.str().c_str());

                std::stringstream write_exinfo;
                write_exinfo << "echo " << "\"" << extinf.c_str() << "\"" <<
                                " >> " << root_m3u8_ << "/" << key_.c_str() << ".m3u8";
                std::stringstream write_ts;
                write_ts << "echo " << "\"" << tsfile.c_str() << "\"" <<
                            " >> " << root_m3u8_ << "/" << key_.c_str() << ".m3u8";
                system(write_exinfo.str().c_str());
                system(write_ts.str().c_str());
            }

        } else {
            std::stringstream copy_ts;
            copy_ts << "cp -f " << ts_src.str().c_str() << " " << root_m3u8_ << "/";
            system(copy_ts.str().c_str());

            std::stringstream write_exinfo;
            write_exinfo << "echo " << "\"" << extinf.c_str() << "\"" << " >> " << root_m3u8_ << "/" << key_.c_str() << ".m3u8";
            std::stringstream write_ts;
            write_ts << "echo " << "\"" << tsfile.c_str() << "\"" << " >> " << root_m3u8_ << "/" << key_.c_str() << ".m3u8";
            system(write_exinfo.str().c_str());
            system(write_ts.str().c_str());
        }
    }
}

void VideoRecord::get_all_ts(std::vector<std::string> &tsfiles)
{
    std::stringstream tslistfile;
    tslistfile << root_m3u8_.c_str() << "/" << "ts_file_list";

    std::stringstream cmd;
    cmd << "ls " << root_m3u8_.c_str() << " -lthr | awk '{print $NF}'" << " >> " << tslistfile.str().c_str();
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
