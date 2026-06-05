#pragma once 
#include "Lock.hpp"
#include <pthread.h>
#include <iostream>
#include <string>

namespace CondModule {
    using namespace MutexModule;
    class Cond {
        public:
            Cond(const Cond&) = delete;

            const Cond& operator=(const Cond&) = delete;

            Cond() {
                int n = pthread_cond_init(&cond, NULL);
                (void)n;
            }

            void Wait(Mutex& mutex) {
                pthread_cond_wait(&cond, mutex.GetMutex());
            }

            void Signal() {
                pthread_cond_signal(&cond);
            }

            void Broadcast() {
                pthread_cond_broadcast(&cond);
            }

            ~Cond() {
                int n = pthread_cond_destroy(&cond);
                (void)n;
            }
        
        private:
            pthread_cond_t cond;
    };
}


