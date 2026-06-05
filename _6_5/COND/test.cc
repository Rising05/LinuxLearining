#include <iostream>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

// 静态初始化条件变量
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// 静态初始化互斥量
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *active(void *arg)
{
    std::string name = static_cast<const char*>(arg);

    while (true)
    {
        // 等待条件变量之前先加锁
        pthread_mutex_lock(&mutex);

        // 当前线程在 cond 条件变量上等待
        // pthread_cond_wait 会自动释放 mutex
        // 被唤醒后会重新竞争 mutex
        pthread_cond_wait(&cond, &mutex);

        std::cout << name << " 活动..." << std::endl;

        // 执行完临界区逻辑后解锁
        pthread_mutex_unlock(&mutex);
    }

    return nullptr;
}

int main(void)
{
    pthread_t t1, t2;

    pthread_create(&t1, NULL, active, (void*)"thread-1");
    pthread_create(&t2, NULL, active, (void*)"thread-2");

    // 确保两个线程已经运行起来并进入等待状态
    sleep(3);

    while (true)
    {
        // 唤醒一个等待线程
        // pthread_cond_signal(&cond);

        // 唤醒所有等待线程
        pthread_cond_broadcast(&cond);

        sleep(1);
    }

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}