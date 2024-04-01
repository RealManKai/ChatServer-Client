#include <cstdio>
#include <sys/socket.h>
#include <iostream>
#include <user.hpp>
#include <vector>
#include <string>
#include <string.h>
#include <functional>
#include <unordered_map>
#include <ctime>
#include <time.h>
#include <chrono>
#include <thread>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "groupuser.hpp"
#include "public.hpp"
#include "group.hpp"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录的用户的好友列表信息
vector<User> g_currentUserFriendlist;
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;

// 读写线程通信
sem_t rwsem;
// 接收线程
void readTaskHandler(int clientfd);

// 控制主菜单页面程序
bool isMainMenuRunning = false;
// 记录登录状态
atomic_bool g_isLoginSuccess{false};

// 显示当前登录成功的用户信息
void showCurrentUserData();
// 获取系统时间
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int);

// 聊天客户端实现，main线程用作发送线程，子线程作为接收线程
int main(int argc, char **argv)
{

    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
    }

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "socket create error!" << endl;
        exit(-1); // -1 是常用的表示程序异常退出的值
    }

    // 填写client需要连接的server信息ip+port
    sockaddr_in server;                      // sockaddr_in 类型的结构体变量。sockaddr_in 结构体用于存储 IPv4 地址和端口号。
    memset(&server, 0, sizeof(sockaddr_in)); // memset 函数作用是将一段内存块的内容全部设置为特定的值

    server.sin_family = AF_INET;            // sin_family 表示地址族（Address Family），AF_INET 表示 IPv4 地址族
    server.sin_port = htons(port);          // htons 函数用于将主机字节序（host byte order）转换为网络字节序
    server.sin_addr.s_addr = inet_addr(ip); // inet_addr 函数将一个点分十进制的 IPv4 地址转换成一个用网络字节序表示的32位无符号整数

    // client和Server连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error!" << endl;
        close(clientfd);
        exit(-1);
    }
    // 初始化读写线程的信号量
    sem_init(&rwsem, 0, 0); // 第一个参数用于初始化信号量rwsem， 第二个参数表示信号量的初始值，第三个参数0，表示只能在当前进程中共享，不是0则可在不同进程间共享

    // 连接服务器成功，启动接收子线程
    std::thread readTask(readTaskHandler, clientfd); // 创建线程
    readTask.detach();                               // 分离线程

    // main线程用于接收用户输入吧，负责发送数据
    for (;;)
    {
        cout << "________________________________" << endl;
        cout << "__________1.login_______________" << endl;
        cout << "__________2.register____________" << endl;
        cout << "__________3.quit________________" << endl;
        cout << "________________________________" << endl;
        cout << "_____Please input your choice___" << endl;
        int choice = 0;
        cin >> choice;
        cin.get(); // 读取缓冲区残留的回车
        switch (choice)
        {
        case 1: // login
        {

            int id = 0;
            char pwd[50] = {0};
            cout << "useid:";
            cin >> id;
            cin.get(); // 读取缓冲区残留的回车
            cout << "password:";
            cin.getline(pwd, 50);


            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            g_isLoginSuccess=false;
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                // 发送消息失败
                cerr << "send login msg error!" << endl;
            }
            sem_wait(&rwsem);
            if(g_isLoginSuccess==true){
                //进入聊天主菜单
                isMainMenuRunning=true;
                mainMenu(clientfd);
            }
        }
        break;
        case 2:
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, 50);
            cout << "userPassword:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error!" << endl;
            }
            //等待信号量，子线程处理完注册消息会通知
            sem_wait(&rwsem);
        }
        break;
        case 3:
        {
            close(clientfd);
            sem_destroy(&rwsem);
            exit(0);
        }
        default:
        {
            cerr << "Input error, please input again!" << endl;
        }
        break;
        }
    }
    return 0;
}

// 显示当前登录成功的用户信息
void showCurrentUserData()
{
    cout << "======================login user======================" << endl;
    cout << "current login user => id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << endl;
    cout << "----------------------friend list---------------------" << endl;
    // 显示好友列表
    if (!g_currentUserFriendlist.empty())
    {
        for (User &user : g_currentUserFriendlist)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "----------------------group list---------------------" << endl;
    // 显示群组列表
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getGroupName() << " " << group.getDesc() << endl;
            for (groupUser &guser : group.getUsers())
            {
                cout << guser.getName() << " " << guser.getState() << " " << guser.getRole() << endl;
            }
        }
    }
    cout << "========================================================" << endl;
}

// 获取系统时间
string getCurrentTime()
{
    auto currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&currentTime);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_yday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}

// "help" command handler
void help(int fd = 0, string str = "");
// "chat" command handler
void chat(int, string);
// "addfriend" command handler
void addfriend(int, string);
// "creategroup" command handler
void creategroup(int, string);
// "addgroup" command handler
void addgroup(int, string);
// "groupchat" command handler
void groupchat(int, string);
// "loginout" command handler
void loginout(int, string);
// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"loginout", "注销，格式loginout"}};

// 注册系统支持的客户端命令处理
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}
    };


// 主聊天页面程序
void mainMenu(int clientfd)
{
    help();
    char buffer[1024] = {0};
    while (isMainMenuRunning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);//拷贝构造
        string command;//存储命令

        int idx = commandbuf.find(":");
        if(idx == -1)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if(it==commandHandlerMap.end())
        {
            cerr<<"invalid input command!"<<endl;
            continue;
        }
        //调用相应的命令处理回调，main对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd, commandbuf.substr(idx+1, commandbuf.size()-idx));
    } 
}

void help(int, string){
    cout<<"show command list:"<<endl;
    for(auto &p:commandMap){
        cout<<p.first<<":"<<p.second<<endl;
    }
    cout<<endl;    
}

// "chat" command handler
void chat(int clientfd, string str)
{
    int idx = str.find(":");
    if(idx==-1)
    {
        cerr<<"chat command invalid!"<<endl;
        return;
    }
    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx+1, str.size()-idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] =  message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();
    
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(len==-1)
    {
        cerr<<"send loginout msg error!"<<buffer<<endl;
    }

}
// "addfriend" command handler
void addfriend(int clientfd, string str)
{
    //
    int friendid = atoi(str.c_str());

    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(len==-1)
    {
        cerr<<"send addfriend msg error!"<<buffer<<endl;
    }

}
// "creategroup" command handler
void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if(idx==-1)
    {
        cerr<<"chat command invalid!"<<endl;
        return;
    }

    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx+1, str.size()-idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(len==-1)
    {
        cerr<<"send creategroup msg error!"<<buffer<<endl;
    }
}
// "addgroup" command handler
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(len==-1)
    {
        cerr<<"send addgroup msg error!"<<buffer<<endl;
    }
}
// "groupchat" command handler
void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if(idx==-1)
    {
        cerr<<"group chat command invalid!"<<endl;
    }

    int groupid = atoi(str.substr(0,idx).c_str());
    string message = str.substr(idx, str.size()-idx);
    json js;
    js["msgid"] = GROUP_CHAT_MAG;
    js["id"] = g_currentUser.getId();
    js["name"]=g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(len==-1)
    {
        cerr<<"send loginout msg error!"<<buffer<<endl;
    }

}
// "loginout" command handler
void loginout(int clientfd, string str)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(len==-1)
    {
        cerr<<"send loginout msg error!"<<buffer<<endl;
    }
    else
    {
        isMainMenuRunning=false;
    }
}

// 处理登录业务
void doLoginResponse(json &responsejs)
{
    // 登录失败
    if (responsejs["errno"].get<int>() == 1)
    {
        // 打印错误信息
        cerr << "login error" << responsejs["errmsg"] << endl;
        // 重置登录状态
        g_isLoginSuccess = false;
    }
    else
    {
        // 记录当前系统登录的用户信息
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);
        // 查询好友
        if (responsejs.contains("friends"))
        {
            // 初始化好友列表
            g_currentUserFriendlist.clear();

            vector<string> fvec = responsejs["friends"];
            for (string &fstr : fvec)
            {
                json js = json::parse(fstr);
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendlist.push_back(user);
            }
        }
        // 查询群组
        if (responsejs.contains("groups"))
        {
            g_currentUserGroupList.clear();

            vector<string> gvec = responsejs["groups"];
            for (string &gstr : gvec)
            {
                json gjs = json::parse(gstr);
                Group group;
                group.setGroupId(gjs["groupid"].get<int>());
                group.setGroupName(gjs["groupname"]);
                group.setDesc(gjs["groupdesc"]);

                vector<string> gustr = gjs["users"];
                for (string ustr : gustr)
                {
                    groupUser guser;
                    json gujs = json::parse(ustr);
                    guser.setId(gujs["id"].get<int>());
                    guser.setName(gujs["name"]);
                    guser.setState(gujs["state"]);
                    guser.setRole(gujs["role"]);
                    group.getUsers().push_back(guser);
                }
                g_currentUserGroupList.push_back(group);
            }
        }
        // 显示登录用户的基本信息
        showCurrentUserData();

        // 显示当前用户的离线消息
        if (responsejs.contains("offlinemsg"))
        {
            vector<string> offlinemsg = responsejs["offlinemsg"];
            for (string offstr : offlinemsg)
            {
                json offjs = json::parse(offstr);
                if (ONE_CHAT_MSG == offjs["msgid"].get<int>())
                {
                    cout << "============FriendMessage:===============";
                    cout << "[Time" << offjs["time"].get<string>() << "][FriendID:" << offjs["id"].get<int>() << "][name:" << offjs["name"].get<string>() << "][Message:" << offjs["msg"].get<string>() << "]" << endl;
                }
                else
                {
                    cout << "============GroupMessage:===============";
                    cout << "[Time" << offjs["time"].get<string>() << "][GroupID:" << offjs["groupid"].get<int>() << "][name:" << offjs["name"].get<string>() << "][Message:" << offjs["msg"].get<string>() << "]" << endl;
                }
            }
        }
        g_isLoginSuccess = true;
    }
}
// 处理注册业务
void doRegResponse(json &responsejs)
{
    if (responsejs["errno"].get<int>() == 1)
    {
        // 注册失败
        cerr << "name is already exist, register error!" << endl;
    }
    else
    { // 注册成功
        cout << "name register success, userid is " << responsejs["id"]
             << ", do not forget it!" << endl;
    }
}

// 接收线程
void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        // 出现阻塞
        if (len == -1 || len == 0)
        {
            cerr << "recv login response error!" << endl;
            close(clientfd);
            exit(-1);
        }
        // 接收ChatServer转发数据，反序列化成json数据
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (msgtype == ONE_CHAT_MSG)
        {
            cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        if (msgtype == GROUP_CHAT_MAG)
        {
            cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        if (msgtype == LOGIN_MSG_ACK)
        {
            doLoginResponse(js);
            sem_post(&rwsem);
            continue;
        }
        if (msgtype == REG_MSG_ACK)
        {
            doRegResponse(js);
            sem_post(&rwsem);
            continue;
        }
    }
}
