
#pragma once
#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <pthread.h>
#include "block_queue.h"
#include "../lock/locker.h"
using namespace std;

class Log{
public:
    //C++11以后,使用局部变量懒汉不用加锁
    static Log *get_instance(){
        static Log instance;
        return &instance;
    }
    //用于开辟一个线程，专门用于向磁盘写入日志
    static void *flush_log_thread(void *args){
        Log::get_instance()->async_write_log();
    }
    //日志文件名、日志缓冲区大小、最大行数以及最长日志条队列
    bool init(const char *file_name, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);
    //将日志封装，并按照格式送入阻塞队列
    void write_log(int level, const char *format, ...);

    void flush(void);

private:
    Log();
    virtual ~Log();
    void *async_write_log(){
        string single_log;
        //从阻塞队列中取出一个日志string，写入文件
        while (m_log_queue->PopFront(single_log)){
            LockGuard guard(&lock);
            //string.c_str()：将字符串转化成一个C格式的字符串
            fputs(single_log.c_str(), m_fp);
        }
    }

private:
    char dir_name[128]; //路径名
    char log_name[128]; //log文件名
    int m_split_lines;  //日志最大行数
    int m_log_buf_size; //日志缓冲区大小
    long long m_count;  //日志行数记录
    int m_today;        //记录当前时间是哪一天
    FILE *m_fp;         //打开log的文件指针
    char *m_buf;        //日志缓冲区
    BlockQueue<string> *m_log_queue; //阻塞队列
    locker lock;
};


#define LOG_DEBUG(format, ...) Log::get_instance()->write_log(0, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) Log::get_instance()->write_log(1, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) Log::get_instance()->write_log(2, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) Log::get_instance()->write_log(3, format, ##__VA_ARGS__)


