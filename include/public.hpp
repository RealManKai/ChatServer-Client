#ifndef PUBLIC_H
#define PUBLIC_H

enum EnMsgType
{
    LOGIN_MSG = 1,//login
    LOGIN_MSG_ACK,//登录返回消息
    LOGINOUT_MSG,//注销消息

    REG_MSG,  //register
    REG_MSG_ACK,//注册返回消息

    ONE_CHAT_MSG,//聊天消息
    ADD_FRIEND_MSG,//添加好友消息

    CREATE_GROUP_MSG,//创建群聊消息
    ADD_GROUP_MSG,//加入群聊消息
    GROUP_CHAT_MAG,//群聊天
};

#endif