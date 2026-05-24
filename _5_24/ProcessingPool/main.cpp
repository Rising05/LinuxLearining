// 包含进程池头文件，获取 InitProcessPool / DispatchTask / CleanProcessPool 函数声明
// 以及全局变量 processnum 和 channels 的声明
#include "processpool.hpp"

// main 函数：程序入口点
// argc 是命令行参数个数，argv 是命令行参数数组
// 父进程在此完成进程池的初始化、任务分发和资源清理
int main(int argc, char *argv[])
{
    // 设置子进程数量为 4
    // processnum 是 processpool.cpp 中定义的全局变量
    // InitProcessPool 会读取此值来决定 fork 多少个子进程
    processnum = 4;

    // 步骤 1：初始化进程池
    // 内部循环 processnum 次，每次 pipe + fork 创建一个子进程
    // 子进程进入 Worker 循环等待命令，父进程收集 Channel 对象
    // 返回值是 int 类型的错误码
    int ret = InitProcessPool();

    // 检查初始化是否成功
    if (ret != OK)  // OK 定义为 0，表示成功
    {
        // 初始化失败时输出错误信息
        std::cerr << "InitProcessPool failed, ret = " << ret << std::endl;
        // 返回非零值表示程序异常退出
        return ret;
    }

    // 步骤 2：分发任务
    // 以轮询方式向 4 个子进程分发 20 个随机任务
    // 每个任务间隔 1 秒
    DispatchTask();

    // 步骤 3：清理进程池
    // 关闭所有管道写端，使子进程收到 EOF 退出 Worker 循环
    // 然后 waitpid 回收每个子进程，避免僵尸进程
    CleanProcessPool();

    // 程序正常退出，返回 0
    return 0;
}
