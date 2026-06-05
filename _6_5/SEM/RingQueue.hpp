#pragma once

#include "Sem.hpp"
#include <vector>

template <typename T>
class RingQueue {
private:
    void Lock(thread_mutex_t& mutex) {
        pthread_mutex_lock(mutex);
    }
    void UnLock(thread_mutex_t& mutex) {
        pthread_mutex_unlock(mutex);
    }

public:
    RingQueue(int cap) : _cap(cap), _prod_pos(0), _cons_pos(0), _room_sem(cap), _data_sem(0) {
        pthread_mutex_init(&_prod_mutex, nullptr);
        pthread_mutex_init(&_cons_mutex, nullptr);
        _ring_queue.resize(cap);
    }

    ~RingQueue() {
        pthread_mutex_destroy(&_prod_mutex);
        pthread_mutex_destroy(&_cons_mutex);
    }

    void Push(const T& data) {
        _room_sem.P();
        Lock(_prod_mutex);

        _ring_queue[_prod_pos] = data;
        _prod_pos = (_prod_pos + 1) % _cap;

        UnLock(_prod_mutex);
        _data_sem.V();
    }

    void Pop(T* data) {
        _data_sem.P();
        Lock(_cons_mutex);

        *data = _ring_queue[_cons_pos];
        _cons_pos = (_cons_pos + 1) % _cap;

        UnLock(_cons_mutex);
        _room_sem.V();
    }
private:
    vector<T> _ring_queue;
    int _cap;

    int _prod_pos;
    int _cons_pos;

    Sem _room_sem;;
    Sem _data_sem;

    pthread_mutex_t _prod_mutex;
    pthread_mutex_t _cons_mutex;
};