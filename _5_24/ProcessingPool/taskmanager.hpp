// 头文件保护宏：防止 taskmanager.hpp 被多次包含时产生重复定义
// #pragma once 是另一种常见的头文件保护方式，效果等价于 #ifndef / #define / #endif
#pragma once

// 包含标准输入输出流库，用于任务执行时的日志输出
#include <iostream>
// 包含标准 vector 容器，用于存储任务函数对象数组
#include <vector>
// 包含 std::function，用于将可调用对象（lambda）统一存储为 task_t 类型
#include <functional>
// 包含 C 标准时间库，提供 time() 函数用于初始化随机数种子
#include <ctime>
// 包含 POSIX 标准头文件，提供 getpid() 函数获取当前进程 ID
#include <sys/types.h>
// 包含 POSIX 标准头文件，提供 getpid() 的另一种声明来源
#include <unistd.h>

// 定义 task_t 类型别名：一个无参数、无返回值的可调用对象类型
// std::function<void()> 可以包装函数指针、lambda 表达式、函数对象等
using task_t = std::function<void()>;

// TaskManger 类：任务管理器
// 维护一个任务列表，支持随机选择任务和执行任务
// 注意：类名 TaskManger 是原文拼写，保持与原代码一致
class TaskManger
{
public:
    // 默认构造函数：初始化任务列表并使用当前时间设置随机种子
    TaskManger()
    {
        // 使用当前时间作为随机数种子，使每次程序运行产生的随机序列不同
        // time(nullptr) 返回自 Unix 纪元以来的秒数
        srand(time(nullptr));

        // 向任务列表添加第 0 号任务：模拟访问数据库
        // lambda 表达式 []() { ... } 创建一个匿名可调用对象
        // getpid() 获取当前进程 ID，用于区分是哪个子进程在执行任务
        tasks.push_back([]() {
            // 输出：子进程[进程ID] 执行访问数据库的任务
            std::cout << "sub process[" << getpid()
                      << " ] 执行访问数据库的任务\n"
                      << std::endl;
        });

        // 向任务列表添加第 1 号任务：模拟 URL 解析
        tasks.push_back([]() {
            // 输出：子进程[进程ID] 执行url解析
            std::cout << "sub process[" << getpid()
                      << " ] 执行url解析\n"
                      << std::endl;
        });

        // 向任务列表添加第 2 号任务：模拟加密任务
        tasks.push_back([]() {
            // 输出：子进程[进程ID] 执行加密任务
            std::cout << "sub process[" << getpid()
                      << " ] 执行加密任务\n"
                      << std::endl;
        });

        // 向任务列表添加第 3 号任务：模拟数据持久化任务
        tasks.push_back([]() {
            // 输出：子进程[进程ID] 执行数据持久化任务
            std::cout << "sub process[" << getpid()
                      << " ] 执行数据持久化任务\n"
                      << std::endl;
        });
    }

    // SelectTask 方法：随机选择一个任务编号并返回
    // 返回值范围：[0, tasks.size() - 1]
    int SelectTask()
    {
        // rand() 产生一个随机整数
        // % tasks.size() 取模运算将随机数映射到 [0, tasks.size()-1] 范围
        return rand() % tasks.size();
    }

    // Excute 方法：执行指定编号的任务
    // number 参数：要执行的任务在 tasks 向量中的下标
    // 注意：方法名 Excute 是原文拼写（少了一个 e），保持与原代码一致
    void Excute(unsigned long number)
    {
        // 边界检查：如果编号超出任务列表范围，直接返回，不执行任何操作
        if (number >= tasks.size())
            return;

        // 调用指定下标处的可调用对象（lambda），执行对应的任务逻辑
        tasks[number]();
    }

    // 析构函数：当前为空实现，tasks 向量会自动释放其元素
    ~TaskManger()
    {
        // 空析构：std::vector 及其元素由编译器自动回收
    }

private:
    // tasks 成员变量：存储所有任务的可调用对象数组
    // std::vector<task_t> 动态数组，每个元素是一个 std::function<void()>
    std::vector<task_t> tasks;
};

// 全局 TaskManger 对象的 extern 声明
// 告知编译器 tm 是一个 TaskManger 类型的外部全局变量
// 实际定义在 processpool.cpp 中（单一定义原则）
// 父进程用它来 SelectTask（随机选任务），子进程用它来 Excute（执行任务）
extern TaskManger tm;
