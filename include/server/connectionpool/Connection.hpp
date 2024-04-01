#ifndef CONNECTION_H
#define CONNECTION_H
#include<mysql/mysql.h>
#include<string>
#include<ctime>
#include"public.h"
#include<iostream>

using namespace std;

//数据库增删改查实现
class Connection {
public:
	//初始化数据库连接
	Connection();
	//释放数据库连接资源
	~Connection();
	//连接数据库
	bool connect(string ip, unsigned short port, string user,string password ,string dbname);
	//更新操作
	bool update(string sql);
	//查询
	MYSQL_RES* query(string sql);
	//刷新连接的起始空闲时间点
	void refreshAlivetime() {
		_alivetime = clock();
	};
	//返回存活时间
	clock_t getAlivetime()const {
		return clock() - _alivetime;
	}
	
private:
	MYSQL * _conn;//表示mysql的一条连接
	clock_t _alivetime;  //进入空闲后的起始存活时间

};



#endif

