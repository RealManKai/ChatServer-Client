#ifndef GROUP_H
#define GROUP_H
#include "groupuser.hpp"
#include<string>
#include<vector>
using namespace std;

class Group
{
public:
Group(int id=-1, string groupname="",string desc=""){
    this->groupId = id;
    this->groupName = groupname;
    this->desc = desc;
}

void setGroupId(int gid){this->groupId = gid;}
void setGroupName(string gname){this->groupName = gname;}
void setDesc(string desc){this->desc = desc;}

int getId(){return this->groupId;}
string getGroupName(){return this->groupName;}
string getDesc(){return this->desc;}
vector<groupUser> &getUsers(){return this->gusers;}

private:
    int groupId;
    string groupName;
    string desc;
    vector<groupUser> gusers;

};

#endif