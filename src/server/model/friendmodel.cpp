#include "friendmodel.hpp"
#include"db.h"

// 添加好友信息
void friendModel::insert(int userid, int friendid)
{
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d, %d)", userid, friendid);
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}
// 返回用户好友列表
vector<User> friendModel::query(int userid)
{
    //组装sql语句
    char sql[1024] = {0};
    /*
    多表联合查询
    select a.id, a.name, a.state
    from user a
    inner join friend b
    on b.friendid = a.userid
    where b.userid = %d
    select a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where b.userid=%d
    */
    sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on b.friendid = a.id where b.userid = %d", userid);
    MySQL mysql;
    vector<User> v;
    if(mysql.connect()){
        MYSQL_RES* mess = mysql.query(sql);
        if(mess!=nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(mess))!=nullptr){
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                v.push_back(user);
            }
            mysql_free_result(mess);
            return v;
        }
    }
    return v;
}