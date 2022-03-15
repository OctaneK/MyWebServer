#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include "redis_connection.h"

using namespace std;



redisConnPool *redisConnPool::GetInstance(){
	static redisConnPool connPool;
	return &connPool;
}

//构造初始化
void redisConnPool::init(int maxConn){
    
	for (int i = 0; i < maxConn; i++){
		redisContext* conn = redisConnect("127.0.0.1",6379);
        if (conn == nullptr){
			cout << "redis error" <<endl;
			exit(1);
		}
		connList.PushBack(conn);
	}
}


//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
redisContext *redisConnPool::GetConnection(){
	redisContext *con = NULL;

	connList.PopFront(con);
    return con;
}

//释放当前使用的连接
bool redisConnPool::ReleaseConnection(redisContext *con){
	if (NULL == con)
		return false;

	connList.PushBack(con);
	return true;
}
GetredisContextCon::GetredisContextCon(redisContext **redis, redisConnPool *connPool){
	*redis = connPool->GetConnection();
	
	conn = *redis;
	pool = connPool;
}
GetredisContextCon::~GetredisContextCon(){
	pool->ReleaseConnection(conn);
}

