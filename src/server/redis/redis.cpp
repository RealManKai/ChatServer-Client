#include "redis.hpp"
#include <hiredis/hiredis.h>
#include <hiredis/read.h>
#include <iostream>

using namespace std;

Redis::Redis()
{
    this->_subscribe_context = nullptr;
    this->_publish_context = nullptr;
}
Redis::~Redis(){
    if(this->_subscribe_context!=nullptr)
    {
        redisFree(this->_subscribe_context);
    }
    if(this->_publish_context!=nullptr)
    {
        redisFree(this->_publish_context);
    }
}

//连接redis服务器
bool Redis::connect()
{
    //负责polish发布消息的上下文连接
    this->_publish_context = redisConnect("127.0.0.1", 6379);
    if(this->_publish_context==nullptr)
    {
        cerr<<"connect redis error!"<<endl;
        return false;
    }
    //负责subscribe的上下文连接
    this->_subscribe_context = redisConnect("127.0.0.1", 6379);
    if(this->_subscribe_context==nullptr)
    {
        cerr<<"connect redis error!"<<endl;
        return false;
    }
    //在单独的线程中，监听通道的事件，有消息就向业务层汇报
    thread t([&](){
        observer_channel_message();
    });
    t.detach();

    cerr<<"connect redis success!"<<endl;
    return true;
}
//向redis指定通道channel发布消息
bool Redis::publish(int channel, string message)
{
    redisReply *reply = (redisReply *)redisCommand(_publish_context,"PUBLISH %d %s", channel, message.c_str());
    if(reply == nullptr)
    {
        cerr<<"publish command error!"<<endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}
//向redis指定通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
    //subscribe命令本身会导致线程阻塞等待通道里边发生消息，因此这欧里只进行订阅通道，不接收通道消息
    //通道消息的接收专门在observerc_channel_message函数中的独立线程中进行
    //只负责发送命令，不阻塞接收redis server 响应的消息，否则和notifyMsg线程抢占相应资源
    if(REDIS_ERR==redisAppendCommand(this->_subscribe_context,"SUBSCRIBE %d", channel))
    {
        cerr<<"subscribe command error!"<<endl;
        return false;
    }
    //redisbufferWrite 可惜循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done =0;
    while(!done)
    {
        if(REDIS_ERR==redisBufferWrite(this->_subscribe_context, &done))
        {        
            cerr<<"subscribe command error!"<<endl;
            return false;
        }
    }
    //redisGetReply
    return true;
}

//向redis指定通道subscribe取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if(REDIS_ERR==redisAppendCommand(this->_subscribe_context,"UNSUBSCRIBE %d", channel))
    {
        cerr<<"unsubscribe command error!"<<endl;
        return false;
    } //redisbufferWrite 可惜循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done =0;
    while(!done)
    {
        if(REDIS_ERR==redisBufferWrite(this->_subscribe_context, &done))
        {        
            cerr<<"unsubscribe command error!"<<endl;
            return false;
        }
    }

    return true;
}

//在独立线程中接收订阅通道中的消息    
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    while(REDIS_OK==redisGetReply(this->_subscribe_context, (void **)&reply))
    {
        //订阅收到的消息是一个带三元素的数组
        if(reply != nullptr&& reply->element[2] != nullptr && reply->element[2]->str !=nullptr)
        {
            //给业务层上报通道上发生的消息
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr<<"++++++++++++observer_channel_message quit!"<<endl;
}

//初始化向业务层上报通道详细的回调对象
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler = fn;

}




