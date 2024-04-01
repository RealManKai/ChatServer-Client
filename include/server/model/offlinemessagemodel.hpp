#ifndef OFFINEMESSAGEMODEL_H
#define OFFINEMESSAGEMODEL_H
#include <string>
#include <vector>
using namespace std;
// 提供离线消息处理的操作
class offlineMsgModel
{
public:
    //存储用户离线消息
    void insert(int userid, string msg);
    //删除用户的离线消息
    void remove(int userid);
    //查询用户的离线消息
    vector<string> query(int userid);
};

#endif