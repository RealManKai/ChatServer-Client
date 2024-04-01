#include "chatservice.hpp"
#include "chatserver.hpp"
#include "groupuser.hpp"
#include "public.hpp"
#include <functional>
#include <muduo/base/Logging.h>
#include "user.hpp"
#include "usermodel.hpp"
#include <mutex>

using namespace muduo;
// 获取单例对象的接口函数
Chatservice *Chatservice::instance()
{
    static Chatservice service;
    return &service;
}

// 注册消息以及对应的handler回调操作
Chatservice::Chatservice()
{
    //用户业务事件回调
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&Chatservice::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&Chatservice::loginout, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&Chatservice::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&Chatservice::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&Chatservice::addfriend, this, _1, _2, _3)});
    //群组业务事件回调
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&Chatservice::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&Chatservice::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MAG, std::bind(&Chatservice::groupChat, this, _1, _2, _3)});

    //设置redis服务器
    if(_redis.connect()){
        //设置回调
        _redis.init_notify_handler(std::bind(&Chatservice::handleRedisSubscribeMessage,this, _1, _2));
    }
    
}

// 处理登录业务
void Chatservice::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _usermodel.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        { // 重复登陆，登录失败
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 1;
            response["errmsg"] = "重复登陆，登录失败!";
            conn->send(response.dump());
        }
        else
        { // 登录成功
            {
                // 登录成功就记录用户的连接信息
                lock_guard<mutex> lock(_connMutex);
                _userConnectionMap.insert({id, conn});
            }
            //登录成功后，就向redis订阅channel
            _redis.subscribe(id);

            // 更新用户状态
            user.setState("online");
            _usermodel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                // 读取完消息对离线消息进行删除
                _offlineMsgModel.remove(id);
            }

            // 查询该用户的好友信息并返回
            vector<User> uservec = _friendModel.query(id);
            if (!uservec.empty())
            {
                vector<string> vec2;
                for (User &user : uservec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            //查询该用户的群组消息并返回
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if(!groupuserVec.empty()){
                vector<string> gvec;
                for(Group &group:groupuserVec)
                {
                    json gjs;
                    gjs["groupid"] = group.getId();
                    gjs["groupname"] = group.getGroupName();
                    gjs["groupdesc"] = group.getDesc();
                    vector<string> gustr;
                    for(groupUser &guser: group.getUsers())
                    {
                        json gujs;
                        gujs["id"] = guser.getId();
                        gujs["name"] = guser.getName();
                        gujs["state"] = guser.getState();
                        gujs["role"] = guser.getRole();
                        gustr.push_back(gujs.dump());
                    }
                    gjs["users"] = gustr;
                    gvec.push_back(gjs.dump());
                }
                response["groups"] = gvec;
            }

            conn->send(response.dump());
        }
    }
    else
    { // 登录失败（用户不存在 或者 用户存在密码错误）
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户不存在!";
        conn->send(response.dump());
    }
}
// 处理注册业务
void Chatservice::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);

    bool state = _usermodel.insert(user);
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}
// 获取消息处理器
MsgHandler Chatservice::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    // 记录错误的日志，msgid没有对应的事件处理回调
    if (it == _msgHandlerMap.end())
    {
        // LOG_INFO << "msgid" << msgid << "can not find handler !";
        // 返回空处理器，并打印默认值
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << "msgid" << msgid << "can not find handler !";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

void Chatservice::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnectionMap.begin(); it != _userConnectionMap.end(); ++it)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                _userConnectionMap.erase(it);
                break;
            }
        }
    }

    //用户下线
    _redis.unsubscribe(user.getId());
    if (user.getId() != -1)
    {
        user.setState("offline");
        _usermodel.updateState(user);
    }
}
// 处理一对一聊天
void Chatservice::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    /*
    msgid
    id
    from
    message
    */
    int toid = js["toid"].get<int>();

    // 查找用户
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnectionMap.find(toid);
        if (it != _userConnectionMap.end())
        {
            // 用户在线，服务器推送消息
            it->second->send(js.dump());
            return;
        }
    }
    //查询用户是否在线
    User user = _usermodel.query(toid);
    if(user.getState()=="online")
    {
        _redis.publish(toid, js.dump());
        return;
    }

    // 用户不在线,存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}
// 重置状态操作
void Chatservice::reset()
{
    _usermodel.resetState();
}
// 添加好友信息
void Chatservice::addfriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 读取json
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    _friendModel.insert(userid, friendid);
}
// 创建群组消息
void Chatservice::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}
//添加群组
void Chatservice::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}
//群组聊天
void Chatservice::groupChat(const TcpConnectionPtr &conn,json &js, Timestamp time){
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useidVec = _groupModel.queryGroupsUsers(userid, groupid);
    lock_guard<mutex> lock(_connMutex);
    for(int id: useidVec){
        auto it = _userConnectionMap.find(id);
        if(it!=_userConnectionMap.end()){
            //转发消息
            it->second->send(js.dump());
        }
        else{
            User user = _usermodel.query(userid);
            if(user.getState()=="online")
            {
                _redis.publish(userid,js.dump());
            }
            else
            {
                //存储离线消息
            _offlineMsgModel.insert(userid, js.dump());
            }
        }
    }
}

//处理注销业务
void Chatservice::loginout(const TcpConnectionPtr &conn,json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnectionMap.find(userid);
        if(it != _userConnectionMap.end())
        {
            _userConnectionMap.erase(it);
        }
    }
    _redis.unsubscribe(userid);

    //更新用户状态
    User user(userid, "", "", "offline");
    _usermodel.updateState(user);

}

void Chatservice::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnectionMap.find(userid);
    if(it!=_userConnectionMap.end())
    {
        it->second->send(msg);
        return;
    }
    //存储离线消息
    _offlineMsgModel.insert(userid, msg);
}

/*
ORM Object Relation Map 对象关系映射 业务层只操作对象
DAO Data Access Object 对数据存储的访问接口
*/