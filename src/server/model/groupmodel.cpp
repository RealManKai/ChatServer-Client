#include "groupmodel.hpp"
#include"db.h"
#include "groupuser.hpp"
#include <cstdio>
#include <mysql/mysql.h>
// 创建群组
bool GroupModel::createGroup(Group &group)
{
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup (groupname, groupdesc)values('%s', '%s')"
    , group.getGroupName().c_str(),group.getDesc().c_str() );
    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            group.setGroupId(mysql_insert_id(mysql.getConn()));
            return true;
        }
    }
    return false;
}
// 加入群组
void GroupModel::addGroup(int userid, int groupid, string role)
{
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser values (%d, %d, '%s')"
    ,userid, groupid, role.c_str());
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }


}
// 查询用户所在群组信息
vector<Group> GroupModel::queryGroups(int userid)
{
 //组装sql语句
    char sql[1024] = {0};
    /*
    多表联合查询
    select a.id, a.groupname, a.groupdesc
    from allgroup a
    inner join groupuser b
    on a.id = b.groupid
    where b.userid= %d    
    */
   //1.现根据userid在groupuser表中查询出该用户所属的群组信息
    sprintf(sql, "select a.id, a.groupname, a.groupdesc \
                            from allgroup a \
                            inner join groupuser b \
                            on a.id = b.groupid \
                            where b.userid= %d", userid);
    MySQL mysql;
    vector<Group> groupVec;
    if(mysql.connect()){
        MYSQL_RES* mess = mysql.query(sql);
        if(mess!=nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(mess))!=nullptr){
                Group group;
                group.setGroupId(atoi(row[0]));
                group.setGroupName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            }
            mysql_free_result(mess);
        }
    }
    //2.再根据群组信息查询属于该群组所有用户的userid，并且和user表进行多表联合查询，查出用户的详细信息
    for(Group &group:groupVec){
        /*
        select a.id, a.name, a.state, b.grouprole
        from user a
        inner join groupuser b
        on b.userid = a.id
        where b.groupid = %d
        */
        sprintf(sql,"select a.id, a.name, a.state, b.grouprole \
                                from user a \
                                inner join groupuser b \
                                on b.userid = a.id \
                                where b.groupid = %d", group.getId());
        MYSQL_RES* res = mysql.query(sql);
        if(res!=nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res))!=nullptr){
                groupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }
    }
return groupVec;
}

// 根据指定的groupid查询群组用户id列表，除了userid自己，主要用户群聊业务给群组其他成员发消息
vector<int> GroupModel::queryGroupsUsers(int userid, int groupid)
{
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid = %d and userid = %d"
    , groupid, userid);
    vector<int> idVec;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES* res = mysql.query(sql);
        if(res!=nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res))!= nullptr){
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}
