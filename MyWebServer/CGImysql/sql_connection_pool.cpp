
#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include "sql_connection_pool.h"

using namespace std;

MysqlConPool::MysqlConPool(){
	this->BusyConn = 0;
	this->FreeConn = 0;
}

MysqlConPool *MysqlConPool::GetInstance(){
	static MysqlConPool connPool;
	return &connPool;
}

//构造初始化
void MysqlConPool::init(string url, string User, string PassWord, string DBName, int Port, unsigned int MaxConn){
	this->url = url;
	this->Port = Port;
	this->User = User;
	this->PassWord = PassWord;
	this->DatabaseName = DBName;

	lock.lock();
	pthread_t tid;
    //	启动动态扩容缩容的函数
    pthread_create(&tid, NULL, DynamicAdjust, NULL);

	for (int i = 0; i < MaxConn; i++){
		MYSQL *con = NULL;
		con = mysql_init(con);

		if (con == NULL){
			cout << "Error:" << mysql_error(con);
			exit(1);
		}
		con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DBName.c_str(), Port, NULL, 0);

		if (con == NULL){
			cout << "Error: " << mysql_error(con);
			exit(1);
		}
		connList.push_back(con);
		++FreeConn;
	}

	reserve = sem(FreeConn);

	this->MaxConn = FreeConn;
	
	lock.unlock();
}


//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *MysqlConPool::GetConnection(){
	MYSQL *con = NULL;

	if (0 == connList.size())
		return NULL;

	reserve.wait();
	
	lock.lock();

	con = connList.front();
	connList.pop_front();

	--FreeConn;
	++BusyConn;

	lock.unlock();
	return con;
}

//释放当前使用的连接
bool MysqlConPool::ReleaseConnection(MYSQL *con){
	if (NULL == con)
		return false;

	lock.lock();

	connList.push_back(con);
	++FreeConn;
	--BusyConn;

	lock.unlock();

	reserve.post();
	return true;
}

//销毁数据库连接池
void MysqlConPool::DestroyPool(){

	lock.lock();
	if (connList.size() > 0){
		list<MYSQL *>::iterator it;
		for (it = connList.begin(); it != connList.end(); ++it){
			MYSQL *con = *it;
			mysql_close(con);
		}
		BusyConn = 0;
		FreeConn = 0;
		connList.clear();

		lock.unlock();
	}

	lock.unlock();
}
//根据数据库连接池使用情况，动态的调整最大允许连接数
void *MysqlConPool::dynamicAdjust(){
	
	while(true){
		if(FreeConn>3*BusyConn&&MaxConn>4){
			LockGuard guard(&lock);
			int deleteNode =MaxConn/2;
			for (auto it = connList.begin(); 
					deleteNode>0; deleteNode--,it++){
				MYSQL *con = *it;
				mysql_close(con);
				FreeConn--;
			}
			MaxConn =MaxConn/2;
		}else if(BusyConn>FreeConn*3&&MaxConn<128){
			LockGuard guard(&lock);
			int AddNode =MaxConn;
			MaxConn *=2;
			while(AddNode-->0){
				MYSQL *con = NULL;
				con = mysql_init(con);

				if (con == NULL){
					cout << "Error:" << mysql_error(con);
					exit(1);
				}
				con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DatabaseName.c_str(), Port, NULL, 0);

				if (con == NULL){
					cout << "Error: " << mysql_error(con);
					exit(1);
				}
				connList.push_back(con);
				++FreeConn;
			}
		}else{
			//也许这里改成信号更好
			sleep(60);

		}
	}
}



MysqlConPool::~MysqlConPool(){
	DestroyPool();
}

GetMysqlCon::GetMysqlCon(MYSQL **SQL, MysqlConPool *connPool){
	*SQL = connPool->GetConnection();
	
	conn = *SQL;
	pool = connPool;
}

GetMysqlCon::~GetMysqlCon(){
	pool->ReleaseConnection(conn);
}
