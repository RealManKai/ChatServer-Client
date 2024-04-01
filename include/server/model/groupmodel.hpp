#ifndef GROUPMODEL_H
#define GROUPMODEL_H
#include"group.hpp"
#include<vector>
using namespace std;
// 维护群组信息的操作接口
class GroupModel
{
public:
    //创建群组
    bool createGroup(Group &group);
    //加入群组
    void addGroup(int userid, int groupid, string role);
    //查询用户所在群组
    vector<Group> queryGroups(int userid);
    //根据指定的groupid查询群组用户id列表，除了userid自己，主要用户群聊业务给群组其他成员发消息
    vector<int> queryGroupsUsers(int userid, int groupid);

};

#endif