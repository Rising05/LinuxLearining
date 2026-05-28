#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

void* thread_run(void* arg) {
    pthread_detach(pthread_self());
    printf("%s\n", (char*)arg);
    return NULL;
}

int main() {
    pthread_t tid;
    if((pthread_create(&tid, NULL, thread_run, "Thread1 is running...")) != 0) {
        fprintf(stderr, "Failed to create thread\n");
        return 1;
    }

    int ret = 0;

    sleep(1);

    if(pthread_join(tid, NULL) != 0) {
        fprintf(stderr, "Failed to join thread\n");
        ret = 1;
    }else {
        printf("Thread joined successfully\n");
    }
    return ret;
}