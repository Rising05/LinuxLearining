#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

int ticket = 100;

void* route(void* arg) {
    char* id = (char*)arg;
    while (1) {
        if (ticket > 0) {
            usleep(1000);
            printf("%s sell ticket %d\n", id, ticket);
            ticket--;
        } else {
            break;
        }
    }
    return nullptr;
}

int main() {
    pthread_t t1, t2, t3, t4;

    pthread_create(&t1, nullptr, route, (void*)"thread1");
    pthread_create(&t2, nullptr, route, (void*)"thread2");
    pthread_create(&t3, nullptr, route, (void*)"thread3");
    pthread_create(&t4, nullptr, route, (void*)"thread4");

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);
    pthread_join(t3, nullptr);
    pthread_join(t4, nullptr);

    return 0;
}