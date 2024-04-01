#ifndef DB_H
#define DB_H
#include <string>
#include <sys/syslog.h>
#include<muduo/base/Logging.h>
#include <mysql/mysql.h>
using namespace std;


// 数据库连接
class MySQL
{
public:
    //初始化数据库连接
    MySQL();
    //释放数据库连接资源
    ~MySQL();
    //连接数据库
    bool connect();
    //更新数据库
    bool update(string sql);
    //查询数据库
    MYSQL_RES* query(string sql);

    //返回连接
    MYSQL* getConn();

private:
    MYSQL* _conn;

};

#endif