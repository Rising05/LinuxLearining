// 包含进程池头文件，获得函数声明、全局变量声明、返回码枚举、Channel 和 TaskManger 类型
#include "processpool.hpp"

// ========== 全局变量定义 ==========

// channels 向量定义：存储所有子进程对应的 Channel 对象
// 父进程用它来遍历分发任务（DispatchTask）和回收子进程（CleanProcessPool）
// 子进程在 fork 后会遍历它关闭从父进程继承的管道写端 fd
// 初始为空，由 InitProcessPool 填充
std::vector<Channel> channels;

// processnum 全局变量定义：子进程数量
// 在 main 函数中赋值为期望的子进程数，InitProcessPool 读取它来控制 fork 次数
// 初始值 0，在进入 InitProcessPool 之前必须由调用者设置为正数
int processnum = 0;

// tm 全局 TaskManger 对象定义（单一定义原则）
// 程序启动时自动构造，初始化任务列表并设置随机种子
// 在 taskmanager.hpp 中声明为 extern，此处为唯一定义
TaskManger tm;

// ========== 函数实现 ==========

// Worker 函数：子进程的工作循环
// 子进程已将管道读端通过 dup2 重定向到 fd 0（标准输入）
// 所以从 fd 0 读取等价于从管道读取父进程发来的命令
void Worker()
{
    // 无限循环：持续等待父进程发送的任务命令
    while (true)
    {
        int cmd = 0;  // 用于接收从管道读取到的任务编号（int 类型，4 字节）

        // 从标准输入（实际上是管道读端）读取 sizeof(cmd) 个字节到 cmd 变量
        // ::read 是 POSIX 系统调用，原型 ssize_t read(int fd, void *buf, size_t count)
        // 参数 0   表示文件描述符 0（标准输入），已被 dup2 重定向为管道读端
        // 参数 &cmd 表示数据写入 cmd 变量的内存地址
        // 参数 sizeof(cmd) 表示期望读取的字节数（通常为 4）
        int n = ::read(0, &cmd, sizeof(cmd));

        // 情况 1：读取到的字节数等于 sizeof(cmd)，说明完整收到一个命令
        if (n == sizeof(cmd))
        {
            // 调用全局 TaskManger 对象的 Excute 方法，执行对应编号的任务
            // tm 是在 taskmanager.hpp 中定义的全局对象
            tm.Excute(cmd);
        }
        // 情况 2：read 返回 0，表示管道写端已被关闭（EOF）
        // 当父进程调用 Channel::Close() 关闭管道写端时，子进程的 read 会返回 0
        else if (n == 0)
        {
            // 输出退出信息，显示当前子进程的 PID
            std::cout << "pid: " << getpid() << " quit..." << std::endl;
            // 跳出 while 循环，Worker 函数返回，子进程结束
            break;
        }
        // 情况 3：read 返回其他值（负数表示错误，或小于 sizeof(cmd) 的正数表示部分读取）
        else
        {
            // 空分支：对于错误或部分读取的情况，当前实现不做处理
            // 生产环境中应在此添加错误处理逻辑
        }
    }
    // Worker 返回后，子进程在 InitProcessPool 中调用 ::exit(0) 退出
}

// InitProcessPool 函数：初始化进程池
// 核心逻辑：
//   1. 循环 processnum 次
//   2. 每次创建一个管道（pipe）
//   3. fork 一个子进程
//   4. 子进程：关闭不需要的 fd → dup2 重定向 stdin → 进入 Worker 循环
//   5. 父进程：关闭管道的读端 → 将写端封装为 Channel → 存入 channels 向量
int InitProcessPool()
{
    // 循环 processnum 次，每次创建一个子进程和一个管道
    // processnum 是全局变量，在 main 中设置
    for (int i = 0; i < processnum; i++)
    {
        int pipefd[2] = {0};  // 管道文件描述符数组，初始化为 0
                               // pipefd[0] 是读端（子进程用）
                               // pipefd[1] 是写端（父进程用）

        // 创建管道：pipefd[0] 用于读，pipefd[1] 用于写
        // pipe 成功返回 0，失败返回 -1
        int n = pipe(pipefd);
        if (n < 0)               // 检查 pipe 是否失败
            return PipeError;    // pipe 失败，返回 PipeError 错误码

        // fork 创建子进程
        // fork 调用一次，返回两次：
        //   - 在父进程中返回子进程 PID（> 0）
        //   - 在子进程中返回 0
        //   - 失败时返回 -1
        pid_t id = fork();
        if (id < 0)              // 检查 fork 是否失败
            return ForkError;    // fork 失败，返回 ForkError 错误码

        // ========== 子进程分支：id == 0 ==========
        if (id == 0)
        {
            // 输出调试信息：当前子进程 PID，准备关闭从父进程继承的历史 fd
            std::cout << getpid() << ", child close history fd: ";

            // 遍历 channels 向量，关闭父进程之前创建的所有 Channel 的管道写端
            // 原因：fork 后子进程继承了父进程的整个文件描述符表
            // 子进程不需要其他兄弟进程的管道写端，必须关闭以节省资源并保证 EOF 正确传递
            // 注意：此时 channels 已在之前循环迭代中由父进程填充了部分 Channel
            for (auto &c : channels)
            {
                // 输出被关闭的 fd 编号，用于调试
                std::cout << c.wFd() << " ";
                // 调用 Channel::Close() 关闭该管道写端文件描述符
                c.Close();
            }
            // 历史 fd 关闭完毕的标记
            std::cout << " over" << std::endl;

            // 子进程关闭自己管道的写端 pipefd[1]
            // 子进程只需要读端 pipefd[0] 来接收父进程命令
            // 关闭写端确保子进程不会意外写入管道
            // 也确保当父进程关闭其写端后，子进程的 read 能正确收到 EOF
            ::close(pipefd[1]);

            // 调试输出：显示将要重定向到 stdin 的管道读端 fd
            std::cout << "debug: " << pipefd[0] << std::endl;

            // dup2(pipefd[0], 0)：将 pipefd[0] 复制到文件描述符 0（标准输入）
            // 效果：原本指向终端的 fd 0 现在指向管道读端
            // 之后子进程从 fd 0 读取（如 Worker 中的 ::read(0, ...)）
            // 实际上就是从管道读取父进程发送的命令
            dup2(pipefd[0], 0);

            // 调用 Worker 函数，进入子进程工作循环
            // Worker 内循环 ::read(0, ...) 等待父进程发送任务命令
            // 当父进程关闭所有写端时，read 返回 0，Worker 退出
            Worker();

            // Worker 返回后，子进程正常退出，返回码 0 表示成功
            // ::exit 是系统调用，会刷新缓冲区并终止进程
            ::exit(0);
        }

        // ========== 父进程分支：id > 0 ==========
        // 父进程关闭管道读端 pipefd[0]
        // 父进程只需要写端 pipefd[1] 来向子进程发送命令
        // 关闭读端节省文件描述符资源
        ::close(pipefd[0]);

        // 将管道写端 pipefd[1] 和子进程 PID id 封装为 Channel 对象
        // emplace_back 直接在 vector 末尾原地构造 Channel 对象，避免拷贝
        // 之后通过 channels[who] 可以获取该 Channel 引用，调用 Send(cmd) 发送任务
        channels.emplace_back(pipefd[1], id);
    }

    // 所有子进程创建完毕，返回 OK 表示成功
    return OK;
}

// DispatchTask 函数：分发任务给子进程
// 以轮询（round-robin）方式将随机任务分配给各子进程
void DispatchTask()
{
    int who = 0;     // 轮询索引：指向当前要接收任务的子进程在 channels 中的下标
    int num = 20;    // 任务总数：共分发 20 个任务

    // 循环分发 num 个任务
    while (num--)
    {
        // 调用全局 TaskManger 对象的 SelectTask 方法，随机选择一个任务编号
        int task = tm.SelectTask();

        // 获取当前轮到的子进程对应的 Channel 引用
        // channels[who] 返回第 who 个 Channel 对象的引用
        Channel &curr = channels[who++];

        // who 自增后取模，实现环形轮询
        // 例如 channels.size() = 4 时，who 序列为 0,1,2,3,0,1,2,3...
        who %= channels.size();

        // 输出日志分隔线和分发信息
        std::cout << "######################" << std::endl;
        // 输出：发送任务编号 task 到通道 curr.Name()，剩余任务数 num
        std::cout << "send " << task << " to "
                  << curr.Name()                             // 如 "Channel-4-12345"
                  << ", 任务还剩: " << num << std::endl;
        std::cout << "######################" << std::endl;

        // 通过 Channel::Send 方法向子进程发送任务命令
        // 内部调用 ::write(curr._wfd, &task, sizeof(task))
        curr.Send(task);

        // 休眠 1 秒，模拟任务分发的间隔
        // sleep 是 POSIX 函数，使当前进程挂起指定秒数
        sleep(1);
    }
}

// CleanProcessPool 函数：清理进程池
// 关闭所有管道写端，等待所有子进程结束并回收资源
void CleanProcessPool()
{
    // 遍历 channels 向量中的每个 Channel 对象
    for (auto &c : channels)
    {
        // 关闭管道写端
        // 子进程的 Worker 循环中 ::read(0, ...) 会因此返回 0（EOF）
        // 子进程检测到 EOF 后输出 quit 信息并 break 退出循环
        c.Close();

        // waitpid 等待指定子进程结束
        // 参数说明：
        //   c.Id()     要等待的子进程 PID
        //   nullptr    不关心子进程退出状态（不需要获取退出码）
        //   0          阻塞等待，直到子进程终止
        // 返回值 rid：
        //   > 0    成功回收子进程，返回值为子进程 PID
        //   -1     出错（如子进程不存在或已被回收）
        pid_t rid = ::waitpid(c.Id(), nullptr, 0);

        // 判断是否成功回收
        if (rid > 0)
        {
            // 输出回收成功信息
            std::cout << "child " << rid
                      << " wait ... success" << std::endl;
        }
        // 注意：rid <= 0 的情况（出错）在此未做处理
    }

    // 清理 channels 向量（可选，但可以帮助析构函数正确释放资源）
    // std::vector 在程序退出时会自动调用其析构函数清理元素
}
