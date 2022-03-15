#pragma once
#include <mutex>
#include <deque>
#include <condition_variable>

using namespace std;
template<typename T>
class BlockQueue {
public:
    explicit BlockQueue (int MaxSize = 1000);
    ~BlockQueue();

    bool Empty();
    bool Full();
    int Size();
    int Capacity();

    T Front();
    T Back();
    void PushFront(const T &item);
    void PushBack(const T &item);
    bool PopFront(T &item);
    bool PopBack(T &item);
    void Flush();
    void Close();

private:
    bool m_isClose;
    int m_maxSize;
    deque<T> m_deque;
    mutex m_mutex;
    condition_variable waitUtilNoFull;
    condition_variable waitUtilNoEmpty;
};

template<typename T>
BlockQueue<T>::BlockQueue(int MaxSize) :
    m_isClose(false),
    m_maxSize(MaxSize){
}

template<typename T>
BlockQueue<T>::~BlockQueue() {
    Close();
}

template<typename T>
void BlockQueue<T>::Close() {
    {
        unique_lock<mutex> locker(m_mutex);
        m_deque.clear();
        m_isClose = true;
    }
    //pthread_cond_broadcast() 函数可以解除等待队列中所有线程的“被阻塞”状态。
    waitUtilNoFull.notify_all();
    waitUtilNoEmpty.notify_all();
}

template<typename T>
bool BlockQueue<T>::Empty() {
    unique_lock<mutex> locker(m_mutex);
    return m_deque.empty();
}

template<typename T>
bool BlockQueue<T>::Full() {
    unique_lock<mutex> locker(m_mutex);
    return m_deque.empty() >= m_maxSize;
}


template<typename T>
int BlockQueue<T>::Size() {
    unique_lock<mutex> locker(m_mutex);
    return m_deque.size();
}

template<typename T>
int BlockQueue<T>::Capacity() {
    unique_lock<mutex> locker(m_mutex);
    return m_maxSize;
}

template<typename T>
T BlockQueue<T>::Front() {
    unique_lock<mutex> locker(m_mutex);
    return m_deque.front();
}

template<typename T>
T BlockQueue<T>::Back() {
    unique_lock<mutex> locker(m_mutex);
    return m_deque.back();
}

template<typename T>
void BlockQueue<T>::PushFront(const T &item) {
    unique_lock<mutex> locker(m_mutex);
    while (m_deque.size() >= m_maxSize) {
        waitUtilNoFull.wait(locker);
    }
    m_deque.emplace_front(item);
    //pthread_cond_signal() 函数至少解除一个线程的“被阻塞”状态，如果等待队列中包含多个线程，优先解除哪个线程将由操作系统的线程调度程序决定
    waitUtilNoEmpty.notify_one();
}

template<typename T>
void BlockQueue<T>::PushBack(const T &item) {
    unique_lock<mutex> locker(m_mutex);
    while (m_deque.size() >= m_maxSize) {
        waitUtilNoFull.wait(locker);
    }
    m_deque.emplace_back(item);
    waitUtilNoEmpty.notify_one();
}

template<typename T>
bool BlockQueue<T>::PopFront(T &item) {
    unique_lock<mutex> locker(m_mutex);
    while (m_deque.empty()) {
        waitUtilNoEmpty.wait(locker);
        if (m_isClose) return false;
    }

    item = m_deque.front();
    m_deque.pop_front();
    waitUtilNoFull.notify_one();
    return true;
}

template<typename T>
bool BlockQueue<T>::PopBack(T &item) {
    unique_lock<mutex> locker(m_mutex);
    while (m_deque.empty()) {
        waitUtilNoEmpty.wait(locker);
        if (m_isClose) return false;
    }

    item = m_deque.back();
    m_deque.pop_back();
    waitUtilNoFull.notify_one();
    return true;
}

template<typename T>
void BlockQueue<T>::Flush() {
    unique_lock<mutex> locker(m_mutex);
    waitUtilNoEmpty.notify_one();
}