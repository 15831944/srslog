#pragma once

//json关键字定义
#define JSON_CRLF "\r\n"
#define JSON_ACTION_STR "action"
#define JSON_PARAMS_STR "params"
#define JSON_APP_STR "app"
#define JSON_LIVE_STR "live"
#define JSON_STREAM_STR "stream"
#define JSON_MODE_STR "mode"
#define JSON_USRDATA_STR "usrdata"
#define JSON_USR_NAME_STR "usr_name"
#define JSON_SESSION_STR "session"
#define JSON_MONEY_STR "money"
#define JSON_ERROR_STR "error"
#define JSON_MSG_STR "msg"

#define JS_CHARGE_TYPE "type"
#define JS_CHARGE_USER "user"
#define JS_CHARGE_MONEY "money"
#define JS_CHARGE_SESSIONID "sessionid"
#define JS_CHARGE_RET   "ret"
#define JS_CHARGE_OPENDATE "open_date"
#define JS_CHARGE_MONEY  "money"
#define JS_CHARGE_FLOW "flow"
#define JS_CHARGE_OPENDATE "opendate"
#define JS_CHARGE_FLOWTOTAL  "flowtotal"
#define JS_CHARGE_NFLOWUSED  "nflowused"
#define JS_CHARGE_NMONEYUSED "nmoneyused"
#define JS_CHARGE__PACKUSED "pack_used"
#define JS_CHARGE__ALARMIP "alarm_ip"
#define JS_CHARGE_ALARMPORT "alarm_port"

#define __SRS_TYPE_CHARGE 1000
#define __SRS_TYPE_BUY_PACK 1001
#define __SRS_TYPE_ALARM_INFO 1002
#define __SRS_TYPE_QUERY_INFO 1003
#define __SRS_TYPE_MONEYWARN 1004

#define TOBESTRING(STR) #STR

enum JCC_ERROR_CODE {
    JCC_SUCCESS = 0,
    JCC_CHARGE_FAILED = 1,//充值失败
    JCC_BUY_PACKAGE_FAILED = 2, //购买套餐失败
    JCC_QUERY_USEAGE_FAILED = 3,//查询使用情况失败
    JCC_SET_ALARM_FAILED = 4,//设置报警信息失败
    JCC_CONNECT_SERVER_ERROR = 5,//和服务器通信错误
    JCC_NOT_SET_SERVER = 6,//没有设置服务器信息
    JCC_ERROR,//
};

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
