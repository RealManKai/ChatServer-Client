#include "usermodel.hpp"
#include "db.h"
#include "user.hpp"
#include <cstdio>
#include <iostream>
#include <mysql/mysql.h>

using namespace std;

bool UserModel::insert(User &user)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            user.setId(mysql_insert_id(mysql.getConn()));
            return true;
        }
    }
    return false;
}
// user表的查询
User UserModel::query(int id)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *mess = mysql.query(sql);
        if (mess != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(mess);
            if (row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                mysql_free_result(mess);
                return user;
            }
        }
    }
    return User();
}

// 更新user表
bool UserModel::updateState(User user)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d",
            user.getState().c_str(), user.getId());
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {

            return true;
        }
    }
    return false;
}

void UserModel::resetState()
{
    char sql[1024] = {"update user set state ='offline' where state = 'online'"};
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}