#include "chatserver.hpp"
#include "chatservice.hpp"
#include <functional>
#include <muduo/net/Callbacks.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <functional>
#include <iostream>

using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

using namespace std;
using namespace placeholders;

#include"json.hpp"
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg),
      _loop(loop)
{
    // 注册链接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    // 注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    // 设置线程数量
    _server.setThreadNum(4);
}
// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 回调链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if(!conn->connected()){
        Chatservice::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}
// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    string buf = buffer->retrieveAllAsString();
    json js = json::parse(buf);//主要逻辑是通过js["magid"]对应Maghander,然后获取conn js time
    auto msghandler = Chatservice::instance()->getHandler(js["msgid"].get<int>());
    msghandler(conn, js, time);
}
