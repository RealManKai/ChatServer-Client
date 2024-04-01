#include"ConnectionPool.hpp"
#include<iostream>
#include<mysql/mysql.h>
#include<string>
#include<queue>
#include<thread>


ConnectionPool* ConnectionPool::getConnectionPool() {
	//线程安全的懒汉单例函数接口
	static ConnectionPool pool;//对于静态局部变量的初始化，编译器自动进行lock和unlock
	return &pool;
}


bool ConnectionPool::loadConfigFile() 
{
	//从配置文件加载配置项
	FILE* pf = fopen("mysql.ini", "r");
	if (pf == nullptr) {
		LOG("mysql.ini file is not exist!");
		return false;
	} 
	//到文件末尾为止
	while (!feof(pf)) {
		char line[1024] = { 0 };
		//获取文件
		fgets(line, 1024, pf);
		string str = line;
		int idx = str.find("=", 0);
		if (idx == -1) {//无效的配置项
			continue;
		}
		//port=3306
		int endidx = str.find("\n", idx);
		string key = str.substr(0, idx);
		string value = str.substr(idx + 1, endidx - idx - 1);
		/*cout << key << endl;
		cout << value << endl;*/
		if (key == "ip") {
			_ip = value;
		}
		else if (key == "port") {
			_port = atoi(value.c_str());
		}
		else if (key == "username") {
			_username = value;
		}
		else if (key == "password") {
			_password = value;
		}
		else if (key == "dbname") {
			_dbname = value;
		}
		else if (key == "initSize") {
			_initSize = atoi(value.c_str());
		}
		else if (key == "maxSize") {
			_maxSize = atoi(value.c_str());
		}
		else if (key == "maxIdleTime") {
			_maxIdleTime = atoi(value.c_str());
		}
		else if (key == "connectionTimeOut") {
			_connectionTimeout = atoi(value.c_str());
		}
		//cout << endl;
	}
	return true;//加载完毕
}

//连接池的构造
ConnectionPool::ConnectionPool() {
	if (!loadConfigFile()) {
		return;
	}

	//创建初始数量的连接
	for (int i = 0; i < _initSize; ++i) {
		Connection* p = new Connection();
		p->connect(_ip, _port, _username, _password, _dbname);
		//cout << _ip << _port << _username << _password << _dbname << "连接状态" << b << endl;
		p->refreshAlivetime();
		_connectionQue.push(p);
		_connectionCnt++;
	}

	//启动一个新的线程，作为连接的生产者 linux thread => pthread_create
	//使用绑定器给produceConnectionTask成员方法绑定一个当前对象，这样才能使成员方法作为一个线程独立运行
	//因为线程函数都是C接口，C++的方法使用需要依赖一个对象进行绑定
	thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
	produce.detach();//守护线程

	//启动一个新的线程，用来扫描空闲连接
	thread scanner(std::bind(&ConnectionPool::scanConnectionTask, this));
	scanner.detach();//守护线程

}

void ConnectionPool::produceConnectionTask() {
	for (;;) {
		//设置一把锁，生产者拿了这把锁，消费者就没办法调用
		unique_lock<mutex> lock(_queueMutex);
		while (!_connectionQue.empty()) {
			cv.wait(lock);//队列不为空，此时生产者线程进入等待状态，此时对应有连接状态，消费者可以拿到连接，生产者进入等待状态
		}
		if (_connectionCnt < _maxSize) {
			Connection* p = new Connection();
			p->connect(_ip, _port, _username, _password, _dbname);
			p->refreshAlivetime();
			_connectionQue.push(p);
			_connectionCnt++;
		}
		//通知消费者线程可以消费了
		cv.notify_all();
	}
}
shared_ptr<Connection> ConnectionPool::getConnection() {
	unique_lock<mutex> lock(_queueMutex);
	while (_connectionQue.empty()) {
		//意思是如果是超时醒来 仍然为空，那么此时连接超时
		if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeout))) {
			if (_connectionQue.empty()) {
				LOG("连接超时");
				return nullptr;
			}
		}
	}
	//队列不为空创建一个智能指针指向连接队列的头
	shared_ptr<Connection> sp(_connectionQue.front(),
		[&](Connection* pcon) {
			//这里是在服务器应用线程中调用的，所以需要考虑线程安全
			unique_lock<mutex> lock(_queueMutex);
			pcon->refreshAlivetime();
			_connectionQue.push(pcon);
		});
	//shared_ptr智能指针析构时会直接把connection资源detele掉，相当于调用connection的析构函数
	//connection就被close掉了，这里需要自定义share_ptr释放资源方式，把connection直接归还queue中


	//每连接一个，就弹出一个连接
	_connectionQue.pop();
	//if (_connectionQue.empty()) {
	//	//谁消费了最后一个连接，谁告诉生产者继续生产，不加判断的话也可以，因为生产者判断队列为空时，自动生产
	//	cv.notify_all();//告诉所有消费者
	//}
	//消费完连接以后，通知生产者线程检查队列，如果队列为空，赶紧生产
	cv.notify_all();
	return sp;
}

void ConnectionPool::scanConnectionTask() {
	for (;;) {
		//通过睡眠模拟定时效果
		this_thread::sleep_for(chrono::milliseconds(_maxIdleTime));
		//扫描整个队列释放多余连接
		unique_lock<mutex> lock(_queueMutex);
		while (_connectionCnt > _initSize) {
			Connection* p = _connectionQue.front();
			if (p->getAlivetime() >= (_maxIdleTime * 1000)) {
				_connectionQue.pop();
				_connectionCnt--;
				delete p;
			}
			else
			{
				break;//队头连接没有超过最大连接时间，则其他都不会超时
			}
		}
	}

}
