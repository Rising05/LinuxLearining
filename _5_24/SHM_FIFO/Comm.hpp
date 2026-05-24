/******************************************************************************
 * Comm.hpp — 共享内存 + FIFO 同步通信模块（头文件）
 *
 * 本模块为「共享内存数据传输 + FIFO 通知同步」架构提供基础设施。
 *
 * ── 整体设计思路 ──────────────────────────────────────────────────────────
 *
 *   Server（读端）                         Client（写端）
 *   ┌──────────────────┐                  ┌──────────────────┐
 *   │ 1. 创建共享内存    │                  │ 1. 获取共享内存    │
 *   │ 2. 挂接共享内存    │                  │ 2. 挂接共享内存    │
 *   │ 3. 打开 FIFO 读端  │                  │ 3. 打开 FIFO 写端  │
 *   │ 4. Wait(fd) 阻塞   │ ←── FIFO ────→ │ 4. read stdin      │
 *   │ 5. 读共享内存内容   │                  │ 5. 写入共享内存    │
 *   │    (printf 输出)   │                  │ 6. Signal(fd) 唤醒 │
 *   │                    │                  │                    │
 *   └──────────────────┘                  └──────────────────┘
 *
 *   共享内存只负责「数据的搬运」—— 快，但没有同步能力。
 *   FIFO  只负责「控制信号的传递」—— 慢，但天然支持阻塞/唤醒。
 *   二者配合，取长补短。
 *
 * ── Wait / Signal 的设计 ───────────────────────────────────────────────────
 *
 *   Wait(fd):
 *     - 从 FIFO 的读端阻塞读取一个 uint32_t。
 *     - 如果 FIFO 中还没有数据，read() 系统调用会阻塞当前进程，
 *       直到对端写入数据。这实现了「等待通知」的效果。
 *
 *   Signal(fd):
 *     - 向 FIFO 的写端写入一个 uint32_t。
 *     - 写入后，对端阻塞在 Wait() 上的 read() 会立即返回。
 *       这实现了「唤醒对端」的效果。
 *
 *   这个设计本质上是一种「自制的二元信号量」：
 *     - FIFO 充当信号量载体
 *     - write(1) ≈ sem_post()   — 释放信号量，唤醒等待者
 *     - read(1)  ≈ sem_wait()   — 获取信号量，没有则阻塞
 *
 * ── Init 类的 RAII 设计 ────────────────────────────────────────────────────
 *
 *   Init 是一个全局对象，利用 C++ 的构造/析构自动管理 FIFO 文件的
 *   生命周期：
 *     - 构造函数：调用 mkfifo() 创建命名管道文件
 *     - 析构函数：调用 unlink() 删除命名管道文件
 *   这样无论程序正常退出还是异常退出，FIFO 文件都会被清理。
 ******************************************************************************/

#pragma once

/*
 * ── 系统头文件 ──────────────────────────────────────────────────────────
 *
 * fcntl.h    — open() 系统调用、O_RDONLY / O_WRONLY 标志
 * sys/ipc.h  — ftok() 函数（路径名→IPC key 转换）
 * sys/shm.h  — shmget()、shmat()、shmdt()、shmctl()（System V 共享内存）
 * sys/stat.h — umask()、mkfifo()、文件权限宏
 * sys/types.h— key_t、ssize_t 等基本类型
 * unistd.h   — close()、read()、write()、unlink()
 * cassert    — assert() 宏
 * cstdio     — perror()、printf()、snprintf()
 * ctime      — time()（用于日志时间戳）
 * cstring    — strcmp()
 * iostream   — std::cout（日志输出）
 */
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <cstring>
#include <iostream>

using namespace std;

/******************************************************************************
 * 日志系统
 *
 * 定义四个日志级别，Log() 函数统一格式化输出：
 *
 *   | <Unix时间戳> | <级别> | <消息>
 *
 * 用法示例：
 *   Log("something happened", Warning) << " extra info" << "\n";
 ******************************************************************************/

#define Debug   0    // 调试信息：详细的内部状态
#define Notice  1    // 通知信息：正常的流程节点
#define Warning 2    // 警告信息：非致命异常
#define Error   3    // 错误信息：致命错误

/// msg[] — 日志级别对应的字符串，和上面的宏一一对应
const std::string msg[] = {
    "Debug",
    "Notice",
    "Warning",
    "Error"
};

/**
 * Log - 输出一条带时间戳和级别的日志（返回 ostream 引用，支持链式追加）
 * @param message  日志主体内容
 * @param level    日志级别（Debug / Notice / Warning / Error）
 * @return         std::cout 的引用，方便继续 << 追加内容
 */
std::ostream &Log(std::string message, int level)
{
    std::cout << " | " << (unsigned)time(nullptr)   // Unix 时间戳
              << " | " << msg[level]                 // 级别标签
              << " | " << message;                   // 日志内容
    return std::cout;                               // 返回引用以便链式调用
}

/******************************************************************************
 * 共享内存相关常量
 *
 * ftok(pathname, proj_id) 使用规则：
 *   - pathname 必须是一个存在且可访问的文件路径
 *   - proj_id  的低 8 位有效（0x66 即十进制 102）
 *   - 相同的 (pathname, proj_id) 会生成相同的 key
 ******************************************************************************/

/// PATH_NAME — ftok() 用的路径名（这里用 /home/hyb，可改为 "." 或其他存在的路径）
#define PATH_NAME "/home/hyb"

/// PROJ_ID — ftok() 用的项目 ID，低 8 位有效
#define PROJ_ID  0x66

/// SHM_SIZE — 共享内存段的大小（字节），4KB 足够一般的文本传输
#define SHM_SIZE 4096

/// FIFO_NAME — 命名管道的文件路径（相对路径，在当前目录下创建）
#define FIFO_NAME "./fifo"

/******************************************************************************
 * Init 类 — FIFO 生命周期的 RAII 管理
 *
 * 利用 C++ 全局对象的构造/析构顺序：
 *   - 程序启动时自动调用构造函数 → mkfifo() 创建 FIFO
 *   - 程序结束时自动调用析构函数  → unlink() 删除 FIFO
 *
 * 注意：umask(0) 确保 mkfifo() 创建的权限就是传入的 0666，
 *       不受进程 umask 的掩码影响。
 ******************************************************************************/
class Init
{
public:
    Init()
    {
        umask(0);                                    // 清零文件模式掩码
        int n = mkfifo(FIFO_NAME, 0666);             // 创建命名管道，权限 rw-rw-rw-
        assert(n == 0);                              // 断言创建成功
        (void)n;                                     // 抑制 release 模式下的 unused 警告
        Log("create fifo success", Notice) << "\n";
    }

    ~Init()
    {
        unlink(FIFO_NAME);                           // 从文件系统中删除 FIFO 节点
        Log("remove fifo success", Notice) << "\n";
    }
};

/*
 * 将 O_RDONLY / O_WRONLY 重命名为 READ / WRITE，
 * 使代码在 OpenFIFO() 调用处意图更清晰。
 */
#define READ  O_RDONLY    // 以只读方式打开 FIFO（server 侧）
#define WRITE O_WRONLY    // 以只写方式打开 FIFO（client 侧）

/**
 * OpenFIFO - 打开命名管道文件
 * @param pathname  FIFO 文件路径
 * @param flags     打开标志（READ 或 WRITE，即 O_RDONLY 或 O_WRONLY）
 * @return          文件描述符 fd（>= 0）
 *
 * 注意：以只读方式打开一个 FIFO 会阻塞，直到有进程以只写方式打开同一 FIFO，
 *       反之亦然。因此 server 和 client 的启动顺序会影响谁先阻塞在 open() 上。
 */
int OpenFIFO(std::string pathname, int flags)
{
    int fd = open(pathname.c_str(), flags);
    assert(fd >= 0);        // 断言打开成功
    return fd;
}

/// CloseFifo - 关闭 FIFO 文件描述符
void CloseFifo(int fd)
{
    close(fd);
}

/**
 * Wait - 阻塞等待对端发来的唤醒信号（从 FIFO 读 4 字节）
 * @param fd  FIFO 的读端文件描述符
 *
 * 行为类似 sem_wait()：
 *   - 如果 FIFO 为空，read() 阻塞当前进程
 *   - 如果 FIFO 中有数据（对端调用了 Signal），read() 读取并返回
 *   - 读取的数据本身没有实际意义，仅用于"消费"信号
 */
void Wait(int fd)
{
    Log("等待中....", Notice) << "\n";
    uint32_t temp = 0;
    ssize_t s = read(fd, &temp, sizeof(uint32_t));   // 阻塞读取 4 字节
    assert(s == sizeof(uint32_t));                   // 确保读到完整数据
    (void)s;
}

/**
 * Signal - 向对端发送唤醒信号（向 FIFO 写 4 字节）
 * @param fd  FIFO 的写端文件描述符
 *
 * 行为类似 sem_post()：
 *   - 向 FIFO 写入 4 字节
 *   - 对端阻塞在 Wait() 中的 read() 收到数据后返回
 */
void Signal(int fd)
{
    uint32_t temp = 1;
    ssize_t s = write(fd, &temp, sizeof(uint32_t));  // 写入 4 字节
    assert(s == sizeof(uint32_t));                   // 确保写入完整
    (void)s;
    Log("唤醒中....", Notice) << "\n";
}

/**
 * TransToHex - 将 key_t 键值格式化为十六进制字符串
 * @param k  IPC 键值
 * @return   如 "0x66010204" 的十六进制字符串
 */
string TransToHex(key_t k)
{
    char buffer[32];
    snprintf(buffer, sizeof buffer, "0x%x", k);
    return buffer;
}
