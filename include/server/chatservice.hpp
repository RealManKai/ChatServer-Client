#ifndef CHATSERVICE_H
#define CHATSERVICE_H
#include<functional>
#include <muduo/base/Timestamp.h>
#include<unordered_map>
#include<muduo/net/TcpConnection.h>
#include "offlinemessagemodel.hpp"
#include"usermodel.hpp"
#include<mutex>
#include "friendmodel.hpp"
#include"groupmodel.hpp"
#include "redis.hpp"
using namespace std;
using namespace muduo;
using namespace muduo::net;

#include"json.hpp"
using json = nlohmann::json;


using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

//聊天服务器业务类
class Chatservice{
    public:
    //获取单例对象的接口函数
    static Chatservice* instance();

    //处理登录业务
    void login(const TcpConnectionPtr &conn,json &js, Timestamp time);
    //处理注销业务
    void loginout(const TcpConnectionPtr &conn,json &js, Timestamp time);
    //处理注册业务
    void reg(const TcpConnectionPtr &conn,json &js, Timestamp time);
    //处理一对一聊天
    void oneChat(const TcpConnectionPtr &conn,json &js, Timestamp time);
    //处理加好友
    void addfriend(const TcpConnectionPtr &conn,json &js, Timestamp time);
    //创建群聊
    void createGroup(const TcpConnectionPtr &conn,json &js, Timestamp time);
    //加入群聊
    void addGroup(const TcpConnectionPtr &conn,json &js, Timestamp time);
    //加入群聊
    void groupChat(const TcpConnectionPtr &conn,json &js, Timestamp time);

    //从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int userid, string msg);

    //获取消息处理器
    MsgHandler getHandler(int msgid);
    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    //重置状态操作
    void reset();

    private:
    Chatservice();

    //存储消息id和其他业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap;

    //存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnectionMap;
    //互斥锁保证_userConnection的线程安全
    mutex _connMutex;

//数据操作
    //创建user
    UserModel _usermodel;

    //离线消息处理
    offlineMsgModel _offlineMsgModel;

    //处理好友信息

    friendModel _friendModel;
    //处理群组消息
    GroupModel _groupModel;

    //redis
    Redis _redis;
    

};

#endif