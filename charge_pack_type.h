#pragma once

//json关键字定义
#define JSON_CRLF "\r\n"
#define JSON_ACTION_STR "action"
#define JSON_PARAMS_STR "params"
#define JSON_APP_STR "app"
#define JSON_LIVE_STR "live"
#define JSON_STREAM_STR "stream"
#define JSON_MODE_STR "mode"
#define JSON_FLOW_STR "flow"
#define JSON_USRDATA_STR "usrdata"
#define JSON_USR_NAME_STR "usr_name"
#define JSON_SESSION_STR "session"
#define JSON_MONEY_STR "money"
#define JSON_ERROR_STR "error"
#define JSON_MSG_STR "msg"

//各服务器和计费服务器之间包类型
typedef enum JOPACKTYPE
{
    JOPACKTYPE_UNKNOWN = 0x01,          //未定义

    JOPACKTYPE_TOP_UP,                  //用户充值

    JOPACKTYPE_STREAMMEDIA_HEARTBEAT,   //流媒体服务器和计费服务器之间心跳包
    JOPACKTYPE_STREAMMEDIA_FLOWDATA,   //流媒体服务器向计费服务器发送的流量数据包

    JOPACKTYPE_CLOUD_STORE_HEARTBEAT,  //云存储和计费服务器之间心跳包
    JOPACKTYPE_CLOUD_STORE_FLOWDATA,   //云存储向计费服务器发送端流量数据包

}e_JOPACKTYPE;

//流媒体流量数据连接信息中的连接类型：
typedef enum JOSTREAMMEDIA{    
 JOSTREAMMEDIA_FLOWDATA_LINKMODE_UNKNOWN = 0x01, //未定义

 JOSTREAMMEDIA_FLOWDATA_LINKMODE_PLAY, //play播放模式
 JOSTREAMMEDIA_FLOWDATA_LINKMODE_PUSH,  //push推流模式

}e_JOSTREAMMEDIA;

typedef enum CHARGE_ERROR_CODE
{
    CHARGE_ERROR_SUCCESS = 0x0,

    CHARGE_ERROR_TOPUP_FAILED,
    CHARGE_ERROR_BALANCE_NOT_ENOUTH,

}e_CHARGE_ERROR_CODE;
