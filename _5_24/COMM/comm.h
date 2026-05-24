/******************************************************************************
 * comm.h — System V 共享内存通信模块（头文件）
 *
 * 本模块封装了 System V 共享内存的创建、获取与销毁操作。
 *
 * 核心概念：
 *   共享内存(Shared Memory) 是 Linux 下最快的 IPC 方式。
 *   多个进程将同一块物理内存映射到各自的虚拟地址空间，
 *   之后就能像访问普通内存一样直接读写，无需系统调用中转。
 *
 *   System V IPC 使用「键(key)」来标识资源：
 *   - ftok()   将路径名 + 项目ID 合成一个 key_t 键值
 *   - shmget() 根据 key 创建或获取共享内存段，返回 shmid
 *   - shmat()  将共享内存段挂接(attach)到进程地址空间，返回指针
 *   - shmdt()  将共享内存段从进程地址空间卸载(detach)
 *   - shmctl() 控制共享内存段（如删除）
 ******************************************************************************/

#ifndef _COMM_H_
#define _COMM_H_

#include <stdio.h>       /* perror(), printf() 等标准 I/O */
#include <sys/types.h>   /* key_t, pid_t 等类型定义 */
#include <sys/ipc.h>     /* ftok(), IPC_CREAT, IPC_EXCL, IPC_RMID 等 */
#include <sys/shm.h>     /* shmget(), shmat(), shmdt(), shmctl() */

/*
 * PATHNAME: ftok() 用的路径名。
 *   此处使用 "." 表示当前目录 —— 只要 server 和 client 在同一目录下运行即可。
 *   路径必须真实存在且可访问，否则 ftok() 会失败。
 */
#define PATHNAME "."

/*
 * PROJ_ID: ftok() 用的项目 ID（低 8 位有效）。
 *   0x6666 只是一个习惯性魔数，实际只用低 8 位（即 0x66）。
 *   server 与 client 必须使用相同的 PATHNAME + PROJ_ID，
 *   才能拿到同一个 key，从而操作同一块共享内存。
 */
#define PROJ_ID  0x6666

/**
 * createShm - 创建一块全新的共享内存
 * @size:  共享内存大小（字节）。建议按页对齐（getpagesize() 的整数倍）。
 * 返回值: 成功返回共享内存标识符 shmid（>= 0），失败返回 -1。
 *
 * 底层使用 IPC_CREAT | IPC_EXCL 标志：
 *   - IPC_CREAT: 如果不存在则创建
 *   - IPC_EXCL:  如果已存在则报错（保证拿到的一定是刚刚创建的新段）
 *   - 0666:      权限位，允许所有用户读写
 */
int createShm(int size);

/**
 * getShm - 获取一块已存在（或新建）的共享内存
 * @size:  共享内存大小（字节）。
 * 返回值: 成功返回共享内存标识符 shmid（>= 0），失败返回 -1。
 *
 * 底层仅用 IPC_CREAT（不带 EXCL）：
 *   - 如果共享内存已存在，直接返回其 shmid
 *   - 如果不存在，则创建
 * 这样 client 端无论先启动还是后启动都能拿到同一块内存。
 */
int getShm(int size);

/**
 * destroyShm - 删除共享内存段
 * @shmid: 共享内存标识符。
 * 返回值: 成功返回 0，失败返回 -1。
 *
 * 调用 shmctl() 并传入 IPC_RMID 命令来标记删除。
 * 只有当所有进程都 shmdt() 卸载之后，内核才会真正释放这块内存。
 */
int destroyShm(int shmid);

#endif  /* _COMM_H_ */
