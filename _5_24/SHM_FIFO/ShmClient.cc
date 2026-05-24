/******************************************************************************
 * ShmClient.cc — 共享内存通信示例：客户端（写端）
 *
 * ── 角色定位 ──────────────────────────────────────────────────────────────
 *
 *   本程序作为共享内存通信的「生产者」：
 *     - 获取 server 创建的共享内存段
 *     - 从标准输入（键盘）读取用户输入的文本
 *     - 将文本写入共享内存，然后通过 FIFO 通知 server 来读
 *     - 输入 "quit" 时退出循环
 *
 * ── 运行方式 ──────────────────────────────────────────────────────────────
 *
 *   必须先启动 server（./shm_server），再启动 client（./shm_client）。
 *   因为共享内存由 server 创建，client 只能获取已存在的。
 *
 *   启动后，在 client 终端输入任意文本并回车，
 *   server 终端会立即显示该文本。
 *
 * ── 与 server 的协议 ──────────────────────────────────────────────────────
 *
 *   每次交互的流程：
 *
 *   Client                                Server
 *   ────────────────────────────────────────────────────
 *   1. read(0, shmaddr, ...)              等待中...（阻塞在 Wait(fd)）
 *      （从键盘读入文本到共享内存）
 *   2. Signal(fd) ───────── FIFO ─────→   Wait(fd) 返回
 *      （写入 4 字节唤醒 server）            printf(shmaddr)
 *                                          （读取并打印共享内存）
 *   3. 回到步骤 1                          回到 Wait(fd) 阻塞
 *
 *   特殊退出协议：
 *     - 用户输入 "quit" 时，client 先写入共享内存
 *     - 然后 Signal 通知 server
 *     - server 读到 "quit" 后退出循环 → 删除共享内存
 *     - client 也退出循环
 ******************************************************************************/

#include "Comm.hpp"

Init init;

int main()
{
    /**************************************************************************
     * 步骤 1：生成 IPC 键值（PATH_NAME + PROJ_ID 必须和 server 完全一致）
     *
     * 如果 server 还没有运行（共享内存尚不存在），这里的 ftok() 仍会成功
     * （key 是基于文件 inode 生成的，不依赖共享内存是否已创建），
     * 但后续的 shmget() 会失败。
     **************************************************************************/
    key_t k = ftok(PATH_NAME, PROJ_ID);
    if (k < 0) {
        Log("create key failed", Error)
            << " client key : " << TransToHex(k) << endl;
        exit(1);
    }
    Log("create key done", Debug)
        << " client key : " << TransToHex(k) << endl;

    /**************************************************************************
     * 步骤 2：获取共享内存段（注意：是"获取"不是"创建"）
     *
     * shmget(k, SHM_SIZE, 0) 参数说明：
     *   - k        : IPC 键值（和 server 相同）
     *   - SHM_SIZE : 4096 字节
     *   - 0        : 只有这个 flag！表示"仅获取已存在的段，不创建"
     *
     * 对比 server 端：server 用的是 IPC_CREAT | IPC_EXCL（创建新段），
     * client 用的是 0（获取已存在的段）。这是 IPC 所有权模型的关键区别。
     *
     * 如果 server 还没启动，shmget() 会返回 -1 并设置 errno = ENOENT。
     **************************************************************************/
    int shmid = shmget(k, SHM_SIZE, 0);
    if (shmid < 0) {
        Log("create shm failed", Error)
            << " client key : " << TransToHex(k) << endl;
        exit(2);
    }
    Log("create shm success", Error)
        << " client key : " << TransToHex(k) << endl;

    /**************************************************************************
     * 步骤 3：挂接共享内存到当前进程的地址空间
     *
     * shmat() 返回的指针 shmaddr 指向共享内存在本进程中的映射地址。
     * 虽然 server 和 client 各自调用 shmat() 拿到的虚拟地址不同，
     * 但内核的页表将它们映射到了同一块物理内存，所以：
     *   - client 通过 shmaddr 写入的数据
     *   - server 通过自己的 shmaddr 就能读到
     *
     * 这就是共享内存 IPC 的本质：绕过内核的数据拷贝，直接在物理内存层面共享。
     **************************************************************************/
    char* shmaddr = (char*)shmat(shmid, nullptr, 0);
    if (shmaddr == nullptr) {
        Log("attach shm failed", Error)
            << " client key : " << TransToHex(k) << endl;
        exit(3);
    }
    Log("attach shm success", Error)
        << " client key : " << TransToHex(k) << endl;

    /**************************************************************************
     * 步骤 4：打开 FIFO 写端
     *
     * 以 O_WRONLY 打开 FIFO。此时如果 server 还没打开读端，
     * open() 会阻塞等到 server 就绪。
     *
     * 所以启动顺序是：
     *   - 先启动 server：server 阻塞在 OpenFIFO(READ) 等 client 的写端
     *   - 再启动 client：client 阻塞在 OpenFIFO(WRITE) 等 server 的读端
     *   → 两个 open() 同时完成（FIFO 的"会合"语义）
     **************************************************************************/
    int fd = OpenFIFO(FIFO_NAME, WRITE);

    /**************************************************************************
     * 步骤 5：主循环 —— 读取键盘 → 写入共享内存 → 通知 server
     *
     * read(0, shmaddr, SHM_SIZE - 1)：
     *   - 0            : 标准输入的文件描述符（即键盘输入）
     *   - shmaddr      : 目标缓冲区（直接写入共享内存！）
     *   - SHM_SIZE - 1 : 最大读取字节数，预留 1 字节给 '\0'
     *
     * 写入流程：
     *   1. 从 stdin 读取用户输入到共享内存
     *   2. 将最后一个字符（通常是 '\n'）替换为 '\0'，形成 C 字符串
     *   3. 通过 Signal(fd) 通知 server "数据已就绪，可以读了"
     *   4. 如果输入的是 "quit"，跳出循环（server 也会读到了 quit 并退出）
     *
     * 注意：
     *   - 共享内存写入和 Signal 的顺序不能反：
     *     必须先写数据再发信号，否则 server 可能读到未写完的数据（竞态条件）
     *   - 这里没有对共享内存加锁，正确性完全依赖 FIFO 的同步协议
     **************************************************************************/
    while (true) {
        // 从键盘读取用户输入，直接写入共享内存
        // ssize_t 是有符号版本，read() 返回实际读取的字节数
        ssize_t s = read(0, shmaddr, SHM_SIZE - 1);

        if (s > 0) {
            // s - 1 的位置是 '\n'（用户按回车），替换为 '\0' 形成 C 字符串
            // 这样 server 端的 printf("%s") 和 strcmp() 才能正常工作
            shmaddr[s - 1] = 0;

            // 唤醒 server：数据已就绪，可以读了
            Signal(fd);

            // 检查退出条件：用户输入了 "quit"
            if (strcmp(shmaddr, "quit") == 0)
                break;
        }
        // 如果 read() 返回 0（EOF，如 Ctrl+D）或负数（错误），
        // 不发送 Signal，继续循环。实际生产环境中应处理这些情况。
    }

    /**************************************************************************
     * 步骤 6：清理资源
     *
     * client 只卸载共享内存（shmdt），不删除（IPC_RMID）。
     * 删除共享内存是 server 的职责——server 是所有者。
     *
     * 关闭 FIFO 写端后，server 端的 read() 会读到 EOF（返回 0），
     * 但在本设计中 server 已经通过 "quit" 协议先退出了。
     **************************************************************************/
    CloseFifo(fd);

    int n = shmdt(shmaddr);                             // 卸载共享内存
    assert(n != -1);
    Log("detach shm success", Error)
        << " client key : " << TransToHex(k) << endl;

    return 0;
}
