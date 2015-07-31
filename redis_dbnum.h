#pragma once

//redis各数据库编号说明，格式key_value
typedef enum RedisDBNum
{
    DB_USER_INFO = 0, //用户信息表

    DB_SESSIONID_USRNAME = 1, //会话id_用户名
    DB_USRNAME_MONEYLEFT = 15,//用户名_余额
    DB_USRDEVCHANNEL_SHAREWEBSITEFLAG = 16,//用户名_是否分享到我的网站标志
    DB_SETS_USRDEVCHANNEL_USERS = 17,//SET: 设备名：通道_用户集合
}e_RedisDBNum;
