#ifndef _TIMEHEAP_H_
#define _TIMEHEAP_H_

#include <iostream>
#include <netinet/in.h>
#include <time.h>
#include "../log/log.h"
#define TIMESLOTER 5
using namespace std;
class HeapTimer;

// 用户数据，绑定socket和定时器
struct ClientData {
    sockaddr_in address;
    int sockfd;
    HeapTimer* timer;
};

// 定时器类
class HeapTimer {
public: 
    time_t expire;  // 定时器生效的绝对时间
    ClientData* user_data;//定时器管理的资源
    void ( *cb_func ) ( ClientData* );  // 定时器的回调函数

    HeapTimer(ClientData* user_data,void ( *cb_func ) ( ClientData* )){
        this->expire = time( NULL ) + TIMESLOTER;
        this->user_data =user_data;
        this->cb_func =cb_func;

    }
};

class TimeHeap {
private: 
    HeapTimer** array;  // 堆数组
    int capacity;  // 堆数组容量
    int cur_size; // 堆数组当前包含元素个数
public: 
    TimeHeap( int cap =1);  // 构造函数1，初始化大小为cap的空数组
    TimeHeap( HeapTimer** init_array, int size, int cap ); // 构造函数2，根据已有数组初始化堆
    ~TimeHeap();
public:
    void percolate_down( int hole );  // 对堆结点进行下虑
    void add_timer( HeapTimer* timer );
    void del_timer( HeapTimer* timer );
    void pop_timer();
    void tick();

    void resize();
};

TimeHeap::TimeHeap( int cap ) : capacity(cap), cur_size(0) {
    array = new HeapTimer*[ capacity ];
    if( !array ) {
        throw std::exception();
    }

    for( int i = 0; i < capacity; i++ ) {
        array[i] = nullptr;
    }
}

TimeHeap::TimeHeap( HeapTimer** init_array, int size, int cap ) : cur_size(size), capacity(cap) {
    if( capacity < size ) {
        throw std::exception();
    }

    array = new HeapTimer*[ capacity ];
    if( !array ) {
        throw std::exception();
    }

    for( int i = 0; i < size; i++ ) {
        array[i] = init_array[i];
    }

    // 因为会比较当前节点与子节点，所以只从最下层遍历非叶子节点即可
    for( int i = size/2 - 1; i >= 0 ; i-- ) {
        percolate_down( i );
    }
}

TimeHeap::~TimeHeap() {
    for( int i = 0; i < cur_size; i++ ) {
        if( !array[i] ) {
            delete array[i];
        } 
    }
    delete[] array;
}

// 对堆结点进行下滤，确保第hole个节点满足最小堆性质
void TimeHeap::percolate_down( int hole ) {
    HeapTimer* tmp = array[hole];
    int child = 0;
    for( ; hole * 2 + 1 < cur_size; hole = child ) {
        child = hole * 2 + 1;
        if( child < cur_size-1 && array[child]->expire > array[child+1]->expire ) {  // 右子节点较小
            child++;  // 将节点切换到右子节点
        }
        if( tmp->expire > array[child]->expire ) {  // 子树的根节点值大于子节点值
            array[hole] = array[child];
        } else {  // tmp节点的值最小，符合
            break;
        }
    }
    array[hole] = tmp;  // 将最初的节点放到合适的位置
}

// 添加定时器，先放在数组末尾，在进行上滤使其满足最小堆
void TimeHeap::add_timer( HeapTimer* timer ) {
    if( !timer ) {
        return ;
    }

    if( cur_size >= capacity ) {
        resize();  // 空间不足，将堆空间扩大为原来的2倍
    }

    int hole = cur_size++;
    int parent = 0;

    // 由于新结点在最后，因此将其进行上滤，以符合最小堆
    for( ; hole > 0; hole = parent ) {
        parent = ( hole - 1 ) / 2;
        if( array[parent]->expire > timer->expire ) {
            array[hole] = array[parent];
        } else {
            break;
        }
    }
    array[hole] = timer;
}

// 删除指定定时器
void TimeHeap::del_timer( HeapTimer* timer ) {
    if( !timer ) {
        return;
    }
    // 仅仅将回调函数置空，虽然节省删除的开销，但会造成数组膨胀
    timer->cb_func = nullptr;
}

// 删除堆顶定时器
void TimeHeap::pop_timer() {
    if( !cur_size ) {
        return;
    }
    if( array[0] ) {
        delete array[0];
        array[0] = array[--cur_size];
        percolate_down( 0 );  // 对新的根节点进行下滤
    }
}

// 从时间堆中寻找到时间的结点
void TimeHeap::tick() {
    HeapTimer* tmp = array[0];
    time_t cur = time( NULL );
    while( !cur_size ) {
        if( !tmp ) {
            break ;
        }
        if( tmp->expire > cur ) {  // 未到时间
            break;
        }
        if( array[0]->cb_func ) {
            LOG_INFO("delete........");
            Log::get_instance()->flush();
            array[0]->cb_func( array[0]->user_data );
        }
        pop_timer();
        tmp = array[0];
    }
}

// 空间不足时，将空间扩大为原来的2倍
void TimeHeap::resize() {
    HeapTimer** tmp = new HeapTimer*[ capacity * 2 ];
    for( int i = 0; i < 2 * capacity; i++ ) {
        tmp[i] = nullptr;
    }
    if( !tmp ) {
        throw std::exception();
    }
    capacity *= 2;
    for( int i = 0; i < cur_size; i++ ) {
        tmp[i] = array[i];
    }
    delete[] array;
    array = tmp;
}

#endif