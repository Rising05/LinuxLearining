#ifndef BLOCK_QUEUE_HPP
#define BLOCK_QUEUE_HPP

#include <queue>
#include <pthread.h>
#include <iostream>
#include <string>

/**
 * @brief 基于 pthread 的线程安全有界阻塞队列
 *
 * 生产者-消费者模型的核心数据结构：
 * - 队列满时，生产者线程在 _product_cond 上阻塞等待
 * - 队列空时，消费者线程在 _consum_cond  上阻塞等待
 * - 通过 _product_wait_num / _consum_wait_num 精确记录等待线程数，
 *   避免无效的 pthread_cond_signal 调用
 *
 * @tparam T 队列中存放的元素类型
 */
template <typename T>
class BlockQueue {
public:
    /**
     * @brief 构造函数：初始化互斥锁和两个条件变量
     * @param cap 队列最大容量
     */
    explicit BlockQueue(int cap)
        :_cap(cap),
         _product_wait_num(0),
         _consum_wait_num(0)
    {
        // 初始化 pthread 同步原语，第二个参数为 nullptr 表示使用默认属性
        pthread_mutex_init(&_mutex, nullptr);
        pthread_cond_init(&_product_cond, nullptr);
        pthread_cond_init(&_consum_cond, nullptr);
    }

    /**
     * @brief 入队操作（生产者调用）
     *
     * 如果队列已满，当前线程阻塞直到有消费者取出元素。
     * 入队成功后，若有消费者正在等待，则唤醒一个消费者。
     *
     * @param data 要入队的元素（const 引用，避免拷贝）
     */
    void Enqueue(const T& data) {
        // 上锁 —— 所有对共享数据（队列、计数器）的访问都必须在锁内
        pthread_mutex_lock(&_mutex);

        // while 而非 if：被唤醒后需要重新检查条件，
        // 防止"虚假唤醒"（spurious wakeup）或 多生产者竞争导致队列再次满
        while (IsFull()) {
            ++_product_wait_num;                     // 登记：又有一个生产者进入等待
            pthread_cond_wait(&_product_cond, &_mutex); // 原子操作：解锁 → 睡眠 → 被唤醒后重新加锁
            --_product_wait_num;                     // 被唤醒后，从等待集合中移除
        }

        _block_queue.push(data);

        // 入队后，如果有消费者正在等待，唤醒其中一个
        if (_consum_wait_num > 0) {
            pthread_cond_signal(&_consum_cond);
        }

        pthread_mutex_unlock(&_mutex);
    }

    /**
     * @brief 出队操作（消费者调用）
     *
     * 如果队列为空，当前线程阻塞直到有生产者放入元素。
     * 出队成功后，若有生产者正在等待，则唤醒一个生产者。
     *
     * @param out [out] 用于接收出队元素的引用
     */
    void Pop(T& out) {
        pthread_mutex_lock(&_mutex);

        // while 而非 if：防止虚假唤醒 或 多消费者竞争导致队列再次空
        while (IsEmpty()) {
            ++_consum_wait_num;
            pthread_cond_wait(&_consum_cond, &_mutex);
            --_consum_wait_num;
        }

        out = _block_queue.front();   // 取出队首元素
        _block_queue.pop();           // 移除队首

        // 出队后，如果有生产者正在等待，唤醒其中一个
        if (_product_wait_num > 0) {
            pthread_cond_signal(&_product_cond);
        }

        pthread_mutex_unlock(&_mutex);
    }

    /**
     * @brief 获取当前队列中的元素个数
     *
     * 注意：返回值仅在调用瞬间有效，外部拿到值后可能已变化。
     * 如需原子地"检查大小 + 做决策"，请在外部加锁或封装成组合操作。
     */
    int Size() {
        pthread_mutex_lock(&_mutex);
        int size = static_cast<int>(_block_queue.size());
        pthread_mutex_unlock(&_mutex);
        return size;
    }

    /**
     * @brief 析构函数：销毁 pthread 同步原语
     *
     * 注意：析构前应确保没有线程仍在等待条件变量，
     * 否则 pthread_cond_destroy 可能返回 EBUSY。
     * 生产环境中通常需要提供 Stop() 方法广播唤醒所有等待线程。
     */
    ~BlockQueue() {
        pthread_mutex_destroy(&_mutex);
        pthread_cond_destroy(&_product_cond);
        pthread_cond_destroy(&_consum_cond);
    }

private:
    /**
     * @brief 判断队列是否已满
     *
     * 调用者必须已持有 _mutex 锁
     */
    bool IsFull() {
        return static_cast<int>(_block_queue.size()) == _cap;
    }

    /**
     * @brief 判断队列是否为空
     *
     * 调用者必须已持有 _mutex 锁
     */
    bool IsEmpty() {
        return _block_queue.empty();
    }

    // ==================== 数据成员 ====================

    std::queue<T>       _block_queue;       // 底层容器：标准库队列
    int                 _cap;               // 队列最大容量

    // 同步原语
    pthread_mutex_t     _mutex;             // 互斥锁：保护队列和计数器的并发访问
    pthread_cond_t      _product_cond;      // 生产者条件变量：队列满时生产者在此等待
    pthread_cond_t      _consum_cond;       // 消费者条件变量：队列空时消费者在此等待

    // 等待计数器：精确记录当前有多少线程阻塞在条件变量上
    int                 _product_wait_num;  // 正在等待的生产者数量
    int                 _consum_wait_num;   // 正在等待的消费者数量
};

#endif // BLOCK_QUEUE_HPP
