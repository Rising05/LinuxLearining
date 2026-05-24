// 头文件保护宏：防止 channel.hpp 被多次包含时产生重复定义
#ifndef __CHANNEL_HPP__
#define __CHANNEL_HPP__

// 包含标准输入输出流库，用于日志打印
#include <iostream>
// 包含标准字符串库，用于构造 Channel 名称
#include <string>
// 包含 POSIX 标准头文件，提供 write / close 等系统调用声明
#include <unistd.h>

// Channel 类：封装一个管道写端文件描述符及其对应的子进程信息
// 父进程通过 Channel 对象向指定子进程发送任务命令
class Channel
{
public:
    // 构造函数：接收管道写端文件描述符 wfd 和子进程 PID who
    // wfd 是 pipe 的写端 fd，父进程通过它向子进程写入命令
    // who  是 fork 返回的子进程 PID，用于标识该 channel 对应的子进程
    Channel(int wfd, pid_t who)
        : _wfd(wfd)   // 初始化成员变量 _wfd，保存管道写端文件描述符
        , _who(who)   // 初始化成员变量 _who，保存子进程 PID
    {
        // 构造 Channel 的名称字符串，格式为 "Channel-<wfd>-<who>"
        // 用于日志输出时区分不同的 Channel 对象
        _name = "Channel-" + std::to_string(wfd) + "-" + std::to_string(who);
    }

    // Name 方法：返回 Channel 的名称字符串
    // 用于调试和日志输出，格式如 "Channel-4-12345"
    std::string Name()
    {
        return _name;  // 返回构造时生成的名称
    }

    // Send 方法：向子进程发送一个整数命令
    // cmd 是要发送的任务编号（由 TaskManger::SelectTask() 产生）
    // 内部调用系统调用 ::write，将 cmd 的二进制表示写入管道
    void Send(int cmd)
    {
        // 调用 POSIX write 系统调用，向 _wfd 写入 sizeof(cmd) 个字节
        // &cmd 取 cmd 变量的地址，作为要写入数据的起始地址
        // sizeof(cmd) 计算 cmd 的字节数（通常 int 为 4 字节）
        ::write(_wfd, &cmd, sizeof(cmd));
    }

    // Close 方法：关闭管道的写端文件描述符
    // 父进程调用此方法关闭写端后，子进程的 read 会返回 0（EOF），
    // 子进程即可检测到管道关闭并退出循环
    void Close()
    {
        // 调用 POSIX close 系统调用，关闭 _wfd 文件描述符
        ::close(_wfd);
    }

    // Id 方法：返回该 Channel 对应的子进程 PID
    pid_t Id()
    {
        return _who;  // 返回构造时保存的子进程 PID
    }

    // wFd 方法：返回管道写端文件描述符
    // 在 InitProcessPool 中，子进程遍历 channels 关闭历史 fd 时需要此方法
    int wFd()
    {
        return _wfd;  // 返回管道写端文件描述符
    }

    // 析构函数：当前为空实现，资源释放由 Close() 显式管理
    ~Channel()
    {
        // 空析构：管道的关闭由 CleanProcessPool 统一处理
    }

private:
    int _wfd;              // 管道写端文件描述符
    std::string _name;     // Channel 名称，用于日志标识
    pid_t _who;            // 子进程 PID，用于 waitpid 回收
};

// 头文件保护宏结束
#endif
