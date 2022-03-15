#pragma once


#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include "../lock/locker.h"

using namespace std;

class MysqlConPool
{
public:
	MYSQL *GetConnection();				 //获取数据库连接
	bool ReleaseConnection(MYSQL *conn); //释放连接
	void DestroyPool();					 //销毁数据库连接池

	//单例模式
	static MysqlConPool *GetInstance();

	void init(string url, string User, string PassWord, string DataBaseName, int Port, unsigned int MaxConn); 
	//定期清理过多的连接，必要时增加连接
	
	static void *DynamicAdjust(void *args){
        MysqlConPool::GetInstance()->dynamicAdjust();
    }

	MysqlConPool();
	~MysqlConPool();

private:
	unsigned int MaxConn;  //最大连接数
	unsigned int BusyConn;  //当前已使用的连接数
	unsigned int FreeConn; //当前空闲的连接数
	//动态调整数据库池大小
	void *dynamicAdjust();
private:
	locker lock;
	list<MYSQL *> connList; //连接池
	sem reserve;

private:
	string url;			 //主机地址
	int Port;		 //数据库端口号
	string User;		 //登陆数据库用户名
	string PassWord;	 //登陆数据库密码
	string DatabaseName; //使用数据库名
};

class GetMysqlCon{

public:
	GetMysqlCon(MYSQL **con, MysqlConPool *connPool);
	~GetMysqlCon();
	
private:
	MYSQL *conn;
	MysqlConPool *pool;
};
