
#pragma once
#include <stdio.h>
#include <list>

#include <error.h>
#include <string.h>
#include <iostream>
#include <string>

#include <stdio.h>
#include <hiredis/hiredis.h>
#include <iostream>
#include <string>
#include <mutex>
#include"../log/block_queue.h"
using namespace std;

class redisConnPool
{
public:
	redisContext *GetConnection();				 //获取数据库连接
	bool ReleaseConnection(redisContext *conn); //释放连接
	void DestroyPool();					 //销毁数据库连接池
    

	//单例模式
	static redisConnPool *GetInstance();

	void init(int); 
	//定期清理过多的连接，必要时增加连接
	redisConnPool(){}
	~redisConnPool(){}
private:
	mutex redisMutex;
	BlockQueue<redisContext *> connList; //连接池
    
};

class GetredisContextCon{
public:
	GetredisContextCon(redisContext **con, redisConnPool *connPool);
	~GetredisContextCon();
private:
	redisContext *conn;
	redisConnPool *pool;
};
