// 头文件保护宏：防止 processpool.hpp 被多次包含时产生重复定义
#ifndef __PROCESSPOOL_HPP__
#define __PROCESSPOOL_HPP__

// 包含 Channel 类定义，用于 channels 向量的元素类型
#include "channel.hpp"
// 包含 TaskManger 类及全局 tm 对象，用于任务选择和执行的声明
#include "taskmanager.hpp"
// 包含标准 vector 容器，用于存储 Channel 对象的动态数组
#include <vector>
// 包含 POSIX 标准头文件，提供 pipe / fork / dup2 / read / write / close / waitpid 等系统调用声明
#include <unistd.h>
// 包含 sys/wait.h，提供 waitpid 函数声明及 WNOHANG 等宏
#include <sys/wait.h>
// 包含标准输入输出流库，用于日志输出
#include <iostream>
// 包含标准字符串库（某些编译器下 vector 间接依赖，显式包含避免隐式依赖问题）
#include <string>

// 返回码枚举：使用匿名 enum 定义进程池操作的返回值
// 避免使用宏定义，使返回码具有类型安全性
enum
{
    OK = 0,          // 操作成功
    PipeError = -1,  // pipe 系统调用失败
    ForkError = -2   // fork 系统调用失败
};

// 全局变量声明
// ----------------------------------------------------------------

// channels 向量：存储所有子进程对应的 Channel 对象
// Channel 对象持有管道写端 fd 和子进程 PID
// 父进程通过遍历 channels 来分发任务和回收子进程
extern std::vector<Channel> channels;

// processnum 全局变量：指定进程池中子进程的数量
// 在 main 函数中初始化，控制 fork 子进程的个数
extern int processnum;

// 函数声明
// ----------------------------------------------------------------

// Worker 函数：子进程的工作循环
// 子进程调用 dup2 将管道读端重定向到标准输入（fd 0）后调用此函数
// 循环从标准输入读取整数命令，执行对应任务，直到管道关闭
void Worker();

// InitProcessPool 函数：初始化进程池
// 循环 processnum 次，每次创建一对 pipe 并 fork 一个子进程
// 子进程中关闭不需要的 fd，将管道读端重定向到 stdin，然后进入 Worker 循环
// 父进程中关闭管道读端，将写端封装为 Channel 对象存入 channels 向量
// 返回码：成功返回 OK，pipe 失败返回 PipeError，fork 失败返回 ForkError
int InitProcessPool();

// DispatchTask 函数：分发任务给子进程
// 以轮询（round-robin）方式遍历 channels 向量
// 每次随机选择一个任务编号，通过 Channel::Send() 发送给当前轮到的子进程
// 共分发 20 个任务，每个任务之间间隔 1 秒
void DispatchTask();

// CleanProcessPool 函数：清理进程池
// 遍历所有 Channel 对象，关闭管道写端（子进程 read 返回 0 从而退出）
// 然后调用 waitpid 等待每个子进程结束，回收其资源（避免僵尸进程）
void CleanProcessPool();

// 头文件保护宏结束
#endif
