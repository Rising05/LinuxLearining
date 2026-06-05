#ifndef TASK_HPP
#define TASK_HPP

#include <iostream>
#include <string>

/**
 * @brief 计算任务 —— 阻塞队列的负载类型
 *
 * 每个 Task 携带一个编号和两个操作数，
 * 支持加法或乘法两种运算，模拟线程池中待处理的工作单元。
 */
class Task {
public:
    /**
     * @brief 任务类型枚举
     */
    enum class Type {
        ADD,   // 加法
        MUL    // 乘法
    };

    /**
     * @brief 默认构造函数
     */
    Task();

    /**
     * @brief 参数化构造函数
     *
     * @param id   任务编号
     * @param a    左操作数
     * @param b    右操作数
     * @param type 运算类型（ADD 或 MUL）
     */
    Task(int id, int a, int b, Type type);

    /**
     * @brief 执行任务 —— 根据 type 计算并输出结果
     *
     * @return 计算结果
     */
    int Run() const;

    /**
     * @brief 获取任务的字符串描述（用于日志 / 调试）
     */
    std::string ToString() const;

private:
    int  _id;    // 任务编号
    int  _a;     // 左操作数
    int  _b;     // 右操作数
    Type _type;  // 运算类型
};

#endif // TASK_HPP
