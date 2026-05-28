#include <iostream>
#include <unistd.h>
#include "thread.hpp"
#include <pthread.h>
#include <functional>

void hello1() {
    char buffer[64];

    pthread_getname_np(pthread_self(), buffer, sizeof(buffer) - 1);

    while (1) {
        std::cout << "Hello from " << buffer << std::endl;
        sleep(1);
    }
}

void hello2() {
    char buffer[64];

    pthread_getname_np(pthread_self(), buffer, sizeof(buffer) - 1);

    while (1) {
        std::cout << "Hello from " << buffer << std::endl;
        sleep(1);
    }
}

int main() {
    pthread_setname_np(pthread_self(), "MainThread");

    ThreadModule::Thread<void> t1(hello1);
    t1.Start();

    ThreadModule::Thread<void> t2(std::bind(hello2));
    t2.Start();

    t1.Join();
    t2.Join();
    return 0;
}
