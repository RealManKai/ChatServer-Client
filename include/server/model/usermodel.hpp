#ifndef USERMODEL_H
#define USERMODEL_H
#include "user.hpp"
class UserModel
{
public:
//user表的插入
    bool insert(User &user);
//user表的查询
    User query(int id);
// 更新user表
    bool updateState(User user);
//重置状态信息
    void resetState();

};

#endif