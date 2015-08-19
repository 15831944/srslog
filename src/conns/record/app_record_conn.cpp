#include "app_record_conn.h"
#include "app_log.h"
#include "app_error_code.h"
#include <dirent.h>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include "json_process.h"
#include "../conns/redis_process/redis_process.h"

MutexLock g_records_mu;
std::map<std::string , VideoRecord*> g_map_vr;

SrsRecordConn::SrsRecordConn(SrsServer *srs_server, st_netfd_t client_stfd)
    : SrsConnection(srs_server, client_stfd)

{
    skt = new SrsStSocket(client_stfd);
    proxy = new SrsRecordServer(skt);
}

SrsRecordConn::~SrsRecordConn()
{
    srs_freep(skt);
    srs_freep(proxy);
}

void SrsRecordConn::kbps_resample()
{

}

int64_t SrsRecordConn::get_send_bytes_delta()
{
    return 0;
}

int64_t SrsRecordConn::get_recv_bytes_delta()
{
    return 0;
}

int SrsRecordConn::do_cycle()
{
    int ret = ERROR_SUCCESS;

    if (NULL == proxy) {
        ret = ERROR_POINT_NULL;
        return ret;
    }

    proxy->set_recv_timeout(SRS_CONSTS_ACCOUNT_RECV_TIMEOUT_US);
    proxy->set_send_timeout(SRS_CONSTS_ACCOUNT_SEND_TIMEOUT_US);

    std::string recv_str;
    enum {SIZE_TMP_BUFF = 512};
    char recvtmp[SIZE_TMP_BUFF] = {'\0'};

    while (1){
        bzero(recvtmp, SIZE_TMP_BUFF);

        ssize_t nactually = 0;
        if ((ret = skt->read(recvtmp, SIZE_TMP_BUFF, &nactually) != ERROR_SUCCESS)) {
            srs_error("recv client ,sock error[%d]", ret);
            return ret;
        }

        recv_str.append(recvtmp, nactually);
        if (recv_str.find(CRNL) != std::string::npos) {
            break;
        }
    }

    //cut the CRNL
    std::string newstr = recv_str.substr(0, recv_str.length() - 2);
    handle_client_data(newstr);

    return ret;
}

void SrsRecordConn::handle_client_data(const std::string &data)
{
    srs_trace("========recv client:%s", data.c_str());

    std::vector<std::string> keys;
    keys.push_back(JSON_TYPE);
    keys.push_back(JSON_STREAM);
    keys.push_back(JSON_PUBLISHER);
    keys.push_back(JSON_FILENAME);
    keys.push_back(JSON_TIMEOUT);

    std::map<std::string, std::string> jsonvalue;
    parse_json_no_array(data.c_str(), keys, jsonvalue);

    std::string type;
    get_value_from_map(jsonvalue, JSON_TYPE, type);
    std::string stream;
    get_value_from_map(jsonvalue, JSON_STREAM, stream);
    std::string publisher;
    get_value_from_map(jsonvalue, JSON_PUBLISHER, publisher);
    std::string filename;
    get_value_from_map(jsonvalue, JSON_FILENAME, filename);
    std::string timeout;
    get_value_from_map(jsonvalue, JSON_TIMEOUT, timeout);

    std::string res;

    int ret = RET_CODE_SUCCESS;
    switch (atoi(type.c_str())) {
    case TYPE_START_RECORD:
    {
        ret = handle_start_record(stream, publisher, filename, timeout);
        pack_ret_start_record(ret, res);
        send_client(res);
    }
        break;
    case TYPE_STOP_RECORD:
    {
        //in case block mode.
        pack_ret_stop_record(0, res);
        send_client(res);
        ret = handle_stop_record(stream, publisher, filename);        
    }
        break;
    case TYPE_DELETE_RECORD:
    {
        ret = handle_delete_record(stream, publisher, filename);
        pack_ret_delete_record(ret, res);
        send_client(res);
    }
        break;
    default:
        break;
    }

    srs_trace("========send client:%s", res.c_str());
}

int SrsRecordConn::handle_start_record(std::string stream, std::string publisher,
                                       std::string file, std::string timeout)
{
    if (stream.length() <= 0 || publisher.length() <= 0 || file.length() <= 0) {
        srs_error("handle_start_record: input param illigal.");
        return RET_INPUTPARAMS_ERROR;
    }

    if (timeout.length() <= 0) {
        timeout = "172800"; //2 days.
    }

    int ret = RET_CODE_SUCCESS;

    std::string key;
    int npos = file.find(".mp4");
    if (npos == std::string::npos) {
        return RET_FILE_FORMAT_NOT_MP4;
    }
    key = file.substr(0, npos);

    //asd if recording
    if (ask_if_recording(key)){
        srs_trace("client ask start record, has recording now, key=%s", key.c_str());
        return RET_CODE_SUCCESS;
    }

    //insert redis, and start new record.
    if (!insert_record_redis(key, stream, publisher, timeout)) {
        return RET_CODE_INSERT_REDIS_FAILED;
    }

    //start record.
    ret = do_start_record(key, stream, publisher, file);

    return ret;
}

int SrsRecordConn::handle_stop_record(std::string stream, std::string publisher, std::string file)
{
    if (stream.length() <=0 || publisher.length() <= 0 || file.length() <= 0) {
        srs_error("handle_stop_record: input params error.");
        return RET_INPUTPARAMS_ERROR;
    }

    std::string key;
    int npos = file.find(".mp4");
    if (npos == std::string::npos) {
        srs_error("handle_stop_record: file format error.");
        return RET_FILE_FORMAT_NOT_MP4;
    }
    key = file.substr(0, npos);

    //del from redis data.
    {
        std::stringstream cmd;
        cmd << "DEL " << key.c_str();

        g_mu_redis.Lock();
        g_redis->delete_record_key(DB_REDIS_RECORDINFO, cmd.str().c_str());
        g_mu_redis.UnLock();
    }

    //do delete and stop job.
    srs_trace("handle stop record, recv key=%s", key.c_str());

    //get point of vr.
    VideoRecord* vr = NULL;
    {
        g_records_mu.Lock();
        for (std::map<std::string, VideoRecord*>::iterator iter = g_map_vr.begin();
             iter != g_map_vr.end();
             ++iter) {
            if (iter->first == key) {
                vr = iter->second;
                g_map_vr.erase(iter);
                break;
            }
        }
        g_records_mu.UnLock();
    }

    if (NULL == vr) {
        srs_error("do_stop_rec, NULL == vr, may be has stoped, recv key=%s.", key.c_str());
        return RET_CODE_SUCCESS;
    }

    vr->stop_record();
    srs_trace("after stop, in VideoRecord, vr->key==%s, recved key=%s",
              vr->key_.c_str(), key.c_str());

    if (NULL != vr) {
        delete vr;
        vr = NULL;
    }

    return RET_CODE_SUCCESS;
}

int SrsRecordConn::handle_delete_record(std::string stream, std::string publisher, std::string file)
{
    int ret = RET_CODE_SUCCESS;

    //in case client skip stop.
    handle_stop_record(stream, publisher, file);

    std::string hlspath = g_config->get_hls_path();

    std::stringstream mp4_full_path;
    mp4_full_path << hlspath.c_str() << "/mp4/" << publisher.c_str() << "/" <<file.c_str();

    std::stringstream del_mp4_cmd;
    del_mp4_cmd << "rm -r " << mp4_full_path.str().c_str();
    system(del_mp4_cmd.str().c_str());

    return ret;
}

bool SrsRecordConn::ask_if_recording(const std::string &key)
{
    if (NULL == g_redis) {
        srs_error("NULL==g_redis, ask if recording.");
        return false;
    }

    std::stringstream cmd;
    cmd << "EXISTS " << key;
    int res = 0;

    {
        g_mu_redis.Lock();
        g_redis->record_key_exist(res, DB_REDIS_RECORDINFO, cmd.str().c_str());
        g_mu_redis.UnLock();
    }


    return (1 == res);
}

bool SrsRecordConn::insert_record_redis(std::string key,
                                        std::string stream,
                                        std::string publisher,
                                        std::string timeout)
{
    std::stringstream set_stream;
    set_stream << "hset " << key.c_str() << " stream " << stream;
    std::stringstream set_publisher;
    set_publisher << "hset " << key.c_str() << " publisher " << publisher;
    std::stringstream expire_cmd;
    expire_cmd << "EXPIRE " << key.c_str() << " " << timeout.c_str();

    if (NULL == g_redis) {
        srs_error("NULL == g_redis, insert_record_redis");
        return false;
    }

    {
        g_mu_redis.Lock();

        g_redis->select_db(DB_REDIS_RECORDINFO);
        g_redis->start_multi();
        g_redis->multi_quene_cmd(set_stream.str().c_str());
        g_redis->multi_quene_cmd(set_publisher.str().c_str());
        g_redis->multi_quene_cmd(expire_cmd.str().c_str());
        g_redis->exe_multi();

        g_mu_redis.UnLock();
    }

    return true;
}

int SrsRecordConn::do_start_record(std::string key, std::string stream, std::string publisher, std::string file)
{
    int ret = RET_CODE_SUCCESS;

    VideoRecord *vr = new VideoRecord;
    if (0 == vr) {
        return RET_CODE_PONIT_NULL;
    }

    std::stringstream hls_path;
    hls_path << g_config->get_hls_path().c_str();
    std::stringstream tmp_ts_path;
    tmp_ts_path << g_config->get_hls_path().c_str() << "/tstmp";
    std::stringstream mp4_path;
    mp4_path << g_config->get_hls_path().c_str() << "/mp4";

    if ( !vr->init(key, hls_path.str().c_str(),
                   tmp_ts_path.str().c_str(),
                   mp4_path.str().c_str(),
                   "./ffmpeg") )
    {
        return RET_CODE_INIT_RECORD_FAILED;
    }

    if (!vr->start_record(stream.c_str(), publisher.c_str(), file.c_str())) {
        return RET_CODE_INIT_RECORD_FAILED;
    }

    {
        g_records_mu.Lock();
        g_map_vr.insert(std::make_pair(key, vr));
        g_records_mu.UnLock();
    }
    srs_trace("do_start_record: insert into map.key=%s", key.c_str());

    return ret;
}

int SrsRecordConn::send_client(const std::string &res)
{
    int ret = ERROR_SUCCESS;

    ssize_t nsize = 0;
    if ((ret = skt->write((char *)res.c_str(), res.length(), &nsize) != ERROR_SUCCESS)) {
        srs_error("send client ,sock error[%d]", ret);
        return ret;
    }

    return ret;
}


void pack_ret_start_record(int ret, std::string &res)
{
    std::stringstream ss;
//    ss << "{"
//       << "\"" << JSON_TYPE << "\":" << "\"" << TYPE_START_RECORD << "\"" << ","
//       << "\"" << JSON_RET << "\":"  << "\"" << ret << "\""
//       << "}" << CRNL;

    ss << ret;
    res = ss.str();
}


void pack_ret_stop_record(int ret, std::string &res)
{
    std::stringstream ss;
//    ss << "{"
//       << "\"" << JSON_TYPE << "\":" << "\"" << TYPE_STOP_RECORD << "\"" << ","
//       << "\"" << JSON_RET << "\":"  << "\"" << ret << "\""
//       << "}" << CRNL;

    ss << ret;
    res = ss.str();
}


void pack_ret_delete_record(int ret, std::string &res)
{
    std::stringstream ss;
//    ss << "{"
//       << "\"" << JSON_TYPE << "\":" << "\"" << TYPE_DELETE_RECORD << "\"" << ","
//       << "\"" << JSON_RET << "\":"  << "\"" << ret << "\""
//       << "}" << CRNL;

    ss << ret;
    res = ss.str();
}
