/******************************************************************************
 * comm.c — System V 共享内存通信模块（实现）
 *
 * 本文件实现了 comm.h 中声明的三个对外接口：
 *   createShm()  /  getShm()  /  destroyShm()
 *
 * 并提供一个模块内部使用的静态函数 commShm() 来消除重复代码。
 ******************************************************************************/

#include "comm.h"

/**
 * commShm - 共享内存创建/获取的通用内部函数
 * @size:  共享内存大小（字节）
 * @flags: 传给 shmget() 的标志位（IPC_CREAT | IPC_EXCL | 权限 等）
 * 返回值: 成功返回 shmid，失败返回负数错误码。
 *
 * 流程：
 *   1. ftok() 生成 System V IPC key
 *   2. shmget() 根据 key + size + flags 创建或获取共享内存
 *   3. 错误时通过 perror() 打印原因并返回负数
 *
 * 设计为 static 函数：只在本 .c 文件内可见，外部通过 createShm/getShm 调用，
 * 不暴露内部实现细节。
 */
static int commShm(int size, int flags)
{
    /*
     * ftok() — File TO Key
     *   将 PATHNAME 的 inode 号 + PROJ_ID 的低 8 位 合成为一个 key_t。
     *   这样只要路径不变、proj_id 不变，每次运行都能拿到相同的 key，
     *   从而操作同一块共享内存。
     */
    key_t key = ftok(PATHNAME, PROJ_ID);
    if (key < 0) {
        perror("ftok");     /* 常见失败原因：PATHNAME 路径不存在 */
        return -1;
    }

    /*
     * shmget() — Shared Memory GET
     *   根据 key 创建或获取共享内存段。
     *   @key:   IPC 键值
     *   @size:  共享内存大小。创建时指定实际大小；
     *           获取已存在的段时 size 可以 ≤ 实际大小（但不能为 0）。
     *   @flags: 标志位组合
     *     IPC_CREAT   — 不存在则创建
     *     IPC_EXCL    — 配合 IPC_CREAT，如果已存在则报错（排他创建）
     *     0666        — 权限位（八进制）：owner/group/other 均可读写
     * 返回值: 成功返回 shmid（非负整数），失败返回 -1 并设置 errno。
     */
    int shmid = 0;
    if ((shmid = shmget(key, size, flags)) < 0) {
        perror("shmget");   /* 常见失败：权限不够、size 不合理、已存在(EXCL) */
        return -2;
    }

    return shmid;
}

/**
 * createShm - 排他创建共享内存
 *
 * 使用 IPC_CREAT | IPC_EXCL 组合：
 *   保证创建的一定是全新的共享内存段。
 *   如果 key 对应的段已经存在，shmget() 直接报 EEXIST 错误。
 *
 * 典型场景：server 进程用这个接口创建共享内存。
 */
int createShm(int size)
{
    /* 0666: 八进制权限，owner/group/other 可读写 */
    return commShm(size, IPC_CREAT | IPC_EXCL | 0666);
}

/**
 * getShm - 获取（或创建）共享内存
 *
 * 仅使用 IPC_CREAT（不带 EXCL）：
 *   段存在 → 返回已有段的 shmid
 *   段不存在 → 创建一个新段并返回 shmid
 *
 * 典型场景：client 进程用这个接口拿到 server 创建的共享内存。
 * 即使 client 先于 server 启动，也会自动创建，server 再用 getShm 也能拿到同一块。
 */
int getShm(int size)
{
    return commShm(size, IPC_CREAT);
}

/**
 * destroyShm - 删除共享内存段
 *
 * 调用 shmctl() 发送 IPC_RMID（Remove ID）命令。
 * 内核会标记该段为「待删除」：
 *   - 之后不再允许新的 shmat() 挂接
 *   - 当最后一个进程 shmdt() 卸载后，内核真正回收物理内存
 *
 * 注意：
 *   如果忘了调用 destroyShm()，共享内存会一直残留在系统中，
 *   可以用 `ipcs -m` 查看，用 `ipcrm -m <shmid>` 手动删除。
 */
int destroyShm(int shmid)
{
    if (shmctl(shmid, IPC_RMID, NULL) < 0) {
        perror("shmctl");
        return -1;
    }
    return 0;
}
