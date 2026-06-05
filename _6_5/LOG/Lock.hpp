#pragma once
#include <pthread.h>

namespace LockModule {
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

    // RAII 自动加锁/解锁
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
