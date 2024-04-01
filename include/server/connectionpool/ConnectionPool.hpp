#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H
#include<iostream>
#include<string>
#include<queue>
#include<mutex>//互斥锁
#include<atomic> //原子类型
#include<memory>//智能指针
#include<functional>//绑定器在函数接口里
#include<condition_variable>//条件变量
#include<mysql/mysql.h>
#include<thread>
#include"Connection.hpp"
//实现连接池
//单例模式
class ConnectionPool {
public:
	//获取连接池实例
	static ConnectionPool* getConnectionPool();
	//给外部提供一个接口，从连接池中获取一个可用的空闲连接
	shared_ptr<Connection> getConnection();//智能指针自动调用析构函数释放连接资源

private:
	ConnectionPool();//单例1：构造函数私有化

	bool loadConfigFile();//加载配置文件

	//运行在独立的线程中，专门负责生产新连接
	//线程函数作为成员对象的好处就是，能够方便的调用成员变量
	void produceConnectionTask();

	//扫描超过maxIdleTime时间的空闲连接，进行回收
	void scanConnectionTask();
	string _ip;//mysql的ip地址
	unsigned short _port;
	string _username;
	string _password;
	string _dbname;//数据库名称
	int _initSize;//初始连接量
	int _maxSize;//最大连接量
	int _maxIdleTime;//最大空闲时间
	int _connectionTimeout;//最大超时时间

	queue<Connection*> _connectionQue; //存储mysql的连接队列
	mutex _queueMutex; //维护连接队列的线程安全互斥锁
	atomic_int _connectionCnt;//记录连接所创建的connection连接的总数量
	condition_variable cv;//设置条件变量，用于连接生产者线程和消费者线程的通信
};

#endif
