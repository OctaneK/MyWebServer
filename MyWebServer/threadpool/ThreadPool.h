#include<mutex>
#include<queue>
#include<functional>
#include<thread>
#include<utility>
#include<vector>
#include<future>
#include<iostream>
#include <random>
#include<unistd.h>
#include "../log/log.h"
using  namespace std;
//简单实现的线程安全队列
template <typename T>
class SafeQueue{
private:
    queue<T> safeQueue;
    mutex queueMutex;
public:
    SafeQueue(){}
    //默认就是浅拷贝,因此无需额外写任何内容
    SafeQueue(SafeQueue &&){}
    bool empty(){
        unique_lock<mutex> guard(queueMutex);
        return safeQueue.empty();
    }
    bool dequeue(T &t){
        unique_lock<mutex> guard(queueMutex);
        //这里请求的是普通队列的成员函数
        if(safeQueue.empty()){
            return false;
        }
        //可以确保，该队头元素不会再去使用，因此可以使用右值引用
        t = move(safeQueue.front());
        safeQueue.pop();
        return true;
    }
    bool size(){
        unique_lock<mutex> guard(queueMutex);
        return safeQueue.size();
    }
    void enqueue(T &t){
        unique_lock<mutex> guard(queueMutex);
        safeQueue.emplace(t);
    }
};
class ThreadPool{
public:
    SafeQueue<function<void()>> safeQueue;//任务队列
    vector<thread> threads;//启动线程池
    condition_variable condVariable;//用于同步
    mutex poolMutex;//保护资源
    bool closePool;//关闭线程池标志位
class ThreadPoolWorker{
private:
    ThreadPool *pool;
    int workerId;
public:
    ThreadPoolWorker(ThreadPool* pool,int id):pool(pool),workerId(id){}
    //为了与thread适配，需要重载括号运算符
    void operator()()
    {
        function<void()> func;

        bool sign;
        while(!pool->closePool){
            {
                //从任务队列中获取一个可调用对象
                unique_lock<mutex> guard(pool->poolMutex);
                if((pool->safeQueue).empty()){
                    (pool->condVariable).wait(guard);
                }
                sign = (pool->safeQueue).dequeue(func);
                if(!sign)continue;
                
                
            }
            //调用该对象
            func();
        }
    }
};
    ThreadPool(const int maxThread=10):threads(vector<thread>(maxThread)){
        closePool =false;
    }
    void init(){
        for(int i=0;i<threads.size();i++){
            //每启动一个worker，都将调用重载括号函数
            threads[i] =thread(ThreadPoolWorker(this,i));
            //将线程分离，结束任务将自动销毁
            threads[i].detach();
        }
    }
    ~ThreadPool(){
        closePool =true;
        //唤醒所有线程
        condVariable.notify_all();
    }
    template<typename F,typename...Args>//f(args...)是一个函数表达式，对其进行一次decltype将推导出该可调用对象类型
    auto submit(F &&f,Args &&...args)->future<decltype(f(args...))>{
        //返回future是因为可能将任务交送给线程池后需要其执行结果，如果不需要结果其实无需这一步

        //将可调用对象进行适配，以便能使用（），从而能在后面正常使用
        auto func = bind(forward<F>(f),forward<Args>(args)...);
        //将该对象通过智能指针进行管理；获得的异步任务类型，返回值为推导结果，参数为空
        //decltype(f(args...)):可调用对象返回类型 ():可调用对象参数列表为空
        auto taskPtr = make_shared<packaged_task<decltype(f(args...))()>>(func);
        //将该对象交给任务队列，注意到这里通过智能指针调用该对象，封装成一个lambda可调用对象
        function<void()> task =[taskPtr]()
        {
            (*taskPtr)();
        };
        //队列安全，无需多余加锁
        safeQueue.enqueue(task);
        //唤醒一个工作者线程
        condVariable.notify_one();
        //返回一个未来，供同步使用
        return taskPtr->get_future();
    }
};

