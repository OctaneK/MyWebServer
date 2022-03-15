server: main.c  ./http/http_conn.cpp ./http/http_conn.h ./lock/locker.h ./log/log.cpp ./log/log.h ./log/block_queue.h ./CGImysql/sql_connection_pool.cpp ./CGImysql/sql_connection_pool.h ./threadpool/ThreadPool.h ./timer/heap_timer.h ./CGImysql/redis_connection.h
	g++ -g -o  server main.c  ./http/http_conn.cpp ./http/http_conn.h ./lock/locker.h ./log/log.cpp ./log/log.h ./CGImysql/sql_connection_pool.cpp ./CGImysql/sql_connection_pool.h ./threadpool/ThreadPool.h ./timer/heap_timer.h ./CGImysql/redis_connection.cpp -I./deps/hiredis/ -L./deps/hiredis/ -lhiredis -lpthread -lmysqlclient


clean:
	rm  -r server
