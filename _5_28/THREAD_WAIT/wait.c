#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

// 1.通过 return 退出线程
void* thread1(void* arg) {
    printf("Thread 1 is running\n");
    int* p = (int*)malloc(sizeof(int));
    *p = 10;
    return (void*)p;
}

// 2.通过 pthread_exit 退出线程
void* thread2(void* arg) {
    printf("Thread 2 is running\n");
    int* p = (int*)malloc(sizeof(int));
    *p = 20;
    pthread_exit((void*)p);
}

// 3.通过 pthread_cancel 取消线程
void* thread3(void* arg) {
    while(1) {
        // 模拟线程工作
        printf("Thread 3 is running\n");
        sleep(1);
    }
    return NULL;
}
int main() {
    pthread_t tid;
    void* ret;

    // 创建线程1
    pthread_create(&tid, NULL, thread1, NULL);
    pthread_join(tid, &ret);
    printf("Thread 1 returned: %d\n", *(int*)ret);
    free(ret);

    // 创建线程2
    pthread_create(&tid, NULL, thread2, NULL);
    pthread_join(tid, &ret);
    printf("Thread 2 returned: %d\n", *(int*)ret);
    free(ret);

    // 创建线程3
    pthread_create(&tid, NULL, thread3, NULL);
    sleep(5); // 让线程3运行5秒
    pthread_cancel(tid);
    pthread_join(tid, &ret);
    
    if(ret == PTHREAD_CANCELED) {
        printf("Thread 3 was canceled\n");
    } else {
        printf("Thread 3 returned: %s\n", (char*)ret);
    }

    return 0;
}