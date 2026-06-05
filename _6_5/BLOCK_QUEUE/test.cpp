/**
 * @file test.cpp
 * @brief BlockQueue<Task> 多线程功能测试
 *
 * 场景：2 个生产者线程 + 3 个消费者线程，共享一个容量为 3 的阻塞队列。
 * 队列满 → 生产者阻塞；队列空 → 消费者阻塞，验证同步逻辑正确性。
 *
 * 线程安全说明：
 * - 生产者间的互斥由 BlockQueue 内部保证
 * - 全局计数器 _g_produced / _g_consumed 由 _g_counter_mutex 保护
 * - _g_all_produced_cond 用于通知消费者"生产已结束"
 */

#include "BlockQueue.hpp"
#include "Task.hpp"
#include <pthread.h>
#include <iostream>
#include <vector>
#include <unistd.h>      // usleep
#include <cstdlib>       // rand

// ==================== 测试参数 ====================

constexpr int QUEUE_CAPACITY     = 3;   // 队列容量（故意设小以触发阻塞）
constexpr int PRODUCER_COUNT     = 2;   // 生产者线程数
constexpr int CONSUMER_COUNT     = 3;   // 消费者线程数
constexpr int TASKS_PER_PRODUCER = 5;   // 每个生产者生产的任务数
constexpr int TOTAL_TASKS        = PRODUCER_COUNT * TASKS_PER_PRODUCER;

// ==================== 全局共享状态 ====================

BlockQueue<Task>* _g_queue = nullptr;    // 全局阻塞队列指针

int _g_produced = 0;                     // 已生产任务数（受 _g_counter_mutex 保护）
int _g_consumed = 0;                     // 已消费任务数（受 _g_counter_mutex 保护）
bool _g_all_produced = false;            // 是否所有生产者已完成

pthread_mutex_t _g_counter_mutex   = PTHREAD_MUTEX_INITIALIZER;  // 保护计数器
pthread_cond_t  _g_all_produced_cond = PTHREAD_COND_INITIALIZER; // "生产完毕"通知

// ==================== 生产者线程 ====================

/**
 * @brief 生产者线程入口
 *
 * @param arg 指向生产者编号 (int) 的指针，调用者负责管理其生命周期
 *
 * 每个生产者循环 TASKS_PER_PRODUCER 次：
 *   1. 构造一个 Task（加法或乘法交替）
 *   2. 调用 _g_queue->Enqueue() 放入队列（队满时自动阻塞）
 *   3. 更新全局生产计数器并通知可能等待的消费者
 */
void* ProducerThread(void* arg) {
    int producer_id = *reinterpret_cast<int*>(arg);

    for (int i = 0; i < TASKS_PER_PRODUCER; ++i) {
        // 交替选择加法 / 乘法任务
        Task::Type type = (i % 2 == 0) ? Task::Type::ADD : Task::Type::MUL;
        int a = producer_id * 100 + i;
        int b = producer_id * 10  + i * 3;
        Task task(a, b, i + 1, type);   // id = i+1, 每个生产者独立编号

        std::cout << "[Producer " << producer_id
                  << "] 准备入队: " << task.ToString() << std::endl;

        // ★ 核心操作：入队（满则阻塞）
        _g_queue->Enqueue(task);

        // 更新生产计数器（加锁）
        pthread_mutex_lock(&_g_counter_mutex);
        ++_g_produced;
        // 如果所有任务都已生产，广播通知消费者
        if (_g_produced == TOTAL_TASKS) {
            _g_all_produced = true;
            pthread_cond_broadcast(&_g_all_produced_cond);
        }
        pthread_mutex_unlock(&_g_counter_mutex);

        // 短暂随机延迟，模拟"生产耗时"
        usleep((rand() % 50 + 10) * 1000);   // 10~60 ms
    }

    return nullptr;
}

// ==================== 消费者线程 ====================

/**
 * @brief 消费者线程入口
 *
 * @param arg 指向消费者编号 (int) 的指针
 *
 * 消费者循环直到所有任务被消费完：
 *   1. 判断是否应该退出（生产完毕 && 已消费数 == 总数）
 *   2. 调用 _g_queue->Pop() 取出任务（队空时自动阻塞）
 *   3. 执行任务（Run）并更新消费计数器
 */
void* ConsumerThread(void* arg) {
    int consumer_id = *reinterpret_cast<int*>(arg);

    while (true) {
        // ---- 临界区：检查退出条件 & 取出任务 ----
        pthread_mutex_lock(&_g_counter_mutex);

        // 退出条件：所有任务已生产 且 已全部消费
        while (_g_consumed < TOTAL_TASKS &&
               !(_g_all_produced && _g_consumed >= TOTAL_TASKS)) {
            // 暂时还有任务可消费，跳出内层 while 去 Pop
            break;
        }

        if (_g_all_produced && _g_consumed >= TOTAL_TASKS) {
            // 没有更多任务了，退出
            pthread_mutex_unlock(&_g_counter_mutex);
            break;
        }

        pthread_mutex_unlock(&_g_counter_mutex);
        // ---- 临界区结束 ----

        Task task;

        // ★ 核心操作：出队（空则阻塞）
        _g_queue->Pop(task);

        std::cout << "[Consumer " << consumer_id
                  << "] 取出: " << task.ToString() << " → ";

        // 执行任务
        task.Run();

        // 更新消费计数器
        pthread_mutex_lock(&_g_counter_mutex);
        ++_g_consumed;
        pthread_mutex_unlock(&_g_counter_mutex);

        // 短暂随机延迟，模拟"处理耗时"
        usleep((rand() % 100 + 20) * 1000);  // 20~120 ms
    }

    return nullptr;
}

// ==================== 主函数 ====================

int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    // 1. 创建阻塞队列
    BlockQueue<Task> queue(QUEUE_CAPACITY);
    _g_queue = &queue;

    std::cout << "========== BlockQueue 多线程测试 ==========" << std::endl;
    std::cout << "队列容量: " << QUEUE_CAPACITY << std::endl;
    std::cout << "生产者数: " << PRODUCER_COUNT
              << " (各生产 " << TASKS_PER_PRODUCER << " 个任务)" << std::endl;
    std::cout << "消费者数: " << CONSUMER_COUNT << std::endl;
    std::cout << "总任务数: " << TOTAL_TASKS << std::endl;
    std::cout << "============================================"
              << std::endl << std::endl;

    // 2. 启动生产者线程
    std::vector<pthread_t> producers(PRODUCER_COUNT);
    std::vector<int>       prod_ids(PRODUCER_COUNT);
    for (int i = 0; i < PRODUCER_COUNT; ++i) {
        prod_ids[i] = i + 1;
        pthread_create(&producers[i], nullptr, ProducerThread, &prod_ids[i]);
    }

    // 3. 启动消费者线程
    std::vector<pthread_t> consumers(CONSUMER_COUNT);
    std::vector<int>       cons_ids(CONSUMER_COUNT);
    for (int i = 0; i < CONSUMER_COUNT; ++i) {
        cons_ids[i] = i + 1;
        pthread_create(&consumers[i], nullptr, ConsumerThread, &cons_ids[i]);
    }

    // 4. 等待所有生产者完成
    for (int i = 0; i < PRODUCER_COUNT; ++i) {
        pthread_join(producers[i], nullptr);
    }
    std::cout << "\n>>> 所有生产者已结束" << std::endl;

    // 5. 等待所有消费者完成
    for (int i = 0; i < CONSUMER_COUNT; ++i) {
        pthread_join(consumers[i], nullptr);
    }

    std::cout << "\n============================================" << std::endl;
    std::cout << "测试结束" << std::endl;
    std::cout << "生产总数: " << _g_produced << std::endl;
    std::cout << "消费总数: " << _g_consumed << std::endl;
    std::cout << "队列残留: " << queue.Size() << std::endl;

    // 6. 验证结果
    if (_g_produced == TOTAL_TASKS && _g_consumed == TOTAL_TASKS && queue.Size() == 0) {
        std::cout << "✅ 测试通过：所有任务已正确生产并消费" << std::endl;
    } else {
        std::cout << "❌ 测试失败：生产/消费数量不匹配" << std::endl;
        return 1;
    }

    return 0;
}
