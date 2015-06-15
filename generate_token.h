#pragma once

#include <stdint.h>
#include <string>


//版本信息
#define VERSION_INFO_LIB_ANTI_STREAL_LINK_TOKEN "1.0.2.20150612"

//token类型
typedef enum e_AntiStrealLinkTokenType {
    AntiStrealLinkTokenTypeUnknown = 0x0,

    ShareWebsiteAntiStrealLinkToken = 0x1,//分享到我的网站
    VideoSquare, //视频广场
}e_AntiStrealLinkTokenType;

//输入参数结构体
typedef struct t_in_AntiStrealLinkTokenData {
    std::string devguid_;           //设备guid
    uint32_t channel_;               //设备通道号
    std::string usr_name_;          //用户名
    uint32_t time_out_;              //超时时间。必须赋值。 0：不考虑超时,一直有效。非0：token在此时间间隔内有效
    e_AntiStrealLinkTokenType type_;//token类型

    t_in_AntiStrealLinkTokenData () {
        channel_ = 0;
        time_out_ = 0;
        type_ = AntiStrealLinkTokenTypeUnknown;
    }
}t_in_AntiStrealLinkTokenData;

//输出参数结构体
typedef struct t_out_AntiStrealLinkTokenData{
    std::string devguid_;           //设备guid
    uint32_t channel_;
    std::string usr_name_;          //用户名
    int64_t time_;                 //生成token的时间戳
    uint32_t time_out_;              //超时时间
    e_AntiStrealLinkTokenType type_;//token类型

    t_out_AntiStrealLinkTokenData() {
        channel_ = 0;
        time_ = 0;
        time_out_ = 0;
        type_ = AntiStrealLinkTokenTypeUnknown;
    }
}t_out_AntiStrealLinkTokenData;

/**
 * @brief JPLTGetVersion 获取库版本号
 * @return
 */
const char* JPLTGetVersion();

/**
 * @brief JPLTGenerateToken
 * @param out_token 输出token，以字符串形式
 * @param token_buff 输出token空间长度
 * @param in_data 输入结构体，请参看其定义赋值
 * @return 0 成功。 -1 失败。
 */
int JPLTGenerateToken(uint8_t *out_token,
                      const uint32_t token_buff,
                      t_in_AntiStrealLinkTokenData *in);

/**
 * @brief JPLTDecodeToken
 * @param out 输出结构体
 * @param in_token 输入token
 * @return 0 成功。 -1 失败。
 */
int JPLTDecodeToken(
        t_out_AntiStrealLinkTokenData *out,
        uint8_t* in_token);
