#pragma once
#include <mutex>
#include <iostream>

namespace MutexModule {
    class Mutex {
        public:
            Mutex(const Mutex&) = delete;

            const Mutex& operator=(const Mutex&) = delete;

            Mutex() {
                int n = pthread_mutex_init(&mutex, NULL);
                (void)n;
            }

            void Lock() {
                pthread_mutex_lock(&mutex);
            }

            void Unlock() {
                pthread_mutex_unlock(&mutex);
            }

            pthread_mutex_t* GetMutex() {
                return &mutex;
            }

            ~Mutex() {
                int n = pthread_mutex_destroy(&mutex);
                (void)n;
            }
        
        private:
            pthread_mutex_t mutex;
    };

    //使用 RAII 机制实现自动加锁和解锁
    class LockGuard {
        public:
            LockGuard(Mutex& m) : mutex(m) {
                mutex.Lock();
            }

            ~LockGuard() {
                mutex.Unlock();
            }

        private:
            Mutex& mutex;
    };
}