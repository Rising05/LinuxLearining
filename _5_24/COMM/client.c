/******************************************************************************
 * client.c — 共享内存通信示例：客户端（写者）
 *
 * 功能：
 *   1. 获取共享内存段（getShm）
 *   2. 将共享内存挂接到进程地址空间（shmat）
 *   3. 循环向共享内存写入字符 'A' ~ 'Z'
 *   4. 卸载共享内存（shmdt）
 *
 * 运行时序：
 *   server 启动后本程序再启动（但 getShm 使用 IPC_CREAT 不带 EXCL，
 *   即使 client 先启动也能自己创建，只是 server 应使用 getShm 而非 createShm）。
 *
 * 写入模式：
 *   每秒写入一个字母，共 26 个（'A' ~ 'Z'）。
 *   每次写入新字符后紧跟一个 '\0'，使共享内存成为一个合法的 C 字符串，
 *   方便 server 端用 printf("%s") 直接打印。
 *
 * 编译 & 运行：
 *   $ make
 *   $ ./server &    # 后台运行 server
 *   $ ./client      # 前台运行 client
 ******************************************************************************/

#include "comm.h"    /* getShm */
#include <unistd.h>  /* sleep() */

int main(void)
{
    /*
     * 第 1 步：获取共享内存
     *   如果 server 已经创建了共享内存段，这里直接拿到它的 shmid；
     *   如果 server 还没启动，这里会自己创建一个。
     *   （注意：server 端如果用 createShm(IPC_EXCL) 而此时 client 先创建了，
     *    server 就会报"已存在"错误。生产环境中双方应用 getShm 并额外约定创建者。）
     */
    int shmid = getShm(4096);

    /*
     * 短暂休眠 1 秒，等待 server 端完成 shmat 挂接。
     * （和 server 端一样，这是粗糙的同步手段，仅用于演示。）
     */
    sleep(1);

    /*
     * 第 2 步：挂接共享内存
     *   拿到 shmid 后将其映射到本进程的地址空间。
     *   此时 server 和 client 的 addr 指针指向的是同一块物理内存！
     *   一个进程写入，另一个进程立刻就能看到——这就是共享内存的核心优势。
     */
    char *addr = shmat(shmid, NULL, 0);

    /*
     * 再休眠 2 秒，确保 server 也已进入读取循环。
     */
    sleep(2);

    /*
     * 第 3 步：循环写入字母 'A' ~ 'Z'
     *
     *   每次循环：
     *     addr[i] = 'A' + i;   // 写入字符
     *     addr[i+1] = '\0';     // 追加字符串终止符
     *
     *   例如 i=0 时：addr 内容为 "A\0"
     *       i=1 时：addr 内容为 "AB\0"
     *       ...
     *       i=25 时：addr 内容为 "ABCDEFGHIJKLMNOPQRSTUVWXYZ\0"
     *
     *   server 端每秒打印一次，能看到字符串逐渐变长。
     *
     *   ⚠️ 并发警告：
     *     这里没有任何锁保护。可能发生的情况：
     *     - server 读到 "AB" 时，client 刚好在写 'C'，读到 "ABC" 或 "AB" 不确定
     *     - 更糟：如果数据不是单字节，可能出现写了一半的"撕裂"数据
     *     正式项目中务必使用 POSIX 信号量 (semaphore) 或互斥锁 (mutex)
     *     来保护共享内存的并发访问。
     */
    int i = 0;
    while (i < 26) {
        addr[i] = 'A' + i;   /* 写入第 i 个字母 */
        i++;
        addr[i] = 0;         /* '\0' 字符串结尾，覆盖下一字节 */
        sleep(1);            /* 每秒写入一个字符 */
    }

    /*
     * 第 4 步：卸载共享内存
     *   从本进程的地址空间移除该映射。
     *   注意：这里不删除共享内存段（删除由 server 负责），
     *   只做 detach，段本身依然存在，server 还能继续使用。
     */
    shmdt(addr);

    /*
     * 休眠 2 秒让 server 有机会做最后一轮读取和清理。
     */
    sleep(2);

    return 0;
}
