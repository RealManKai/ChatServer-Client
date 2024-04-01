#include"offlinemessagemodel.hpp"
#include"db.h"
#include <mysql/mysql.h>


//存储用户离线消息
void offlineMsgModel::insert(int userid, string msg){
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values(%d, '%s')", userid, msg.c_str());
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}
//删除用户的离线消息
void offlineMsgModel::remove(int userid){
        //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid = %d",userid);
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}
//查询用户的离线消息
vector<string> offlineMsgModel::query(int userid){
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid = %d", userid);
    MySQL mysql;
    vector<string> v;
    if(mysql.connect()){
        MYSQL_RES* mess = mysql.query(sql);
        if(mess!=nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(mess))!=nullptr){
                v.push_back(row[0]);
            }
            mysql_free_result(mess);
            return v;
        }
    }
    return v;
}