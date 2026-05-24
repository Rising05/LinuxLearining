/******************************************************************************
 * minishell.c — 简易 Shell（支持管道）
 *
 * ── 功能概述 ───────────────────────────────────────────────────────────────
 *
 *   实现一个最简 shell，支持：
 *     - 基本命令执行（如 ls、cat、echo）
 *     - 管道（|）连接多个命令，如：ls | grep c | wc -l
 *     - 输入重定向（<）和输出重定向（>、>>）
 *
 * ── 架构：五个函数，一个主循环 ─────────────────────────────────────────────
 *
 *   main()
 *     │
 *     ├─► do_face()      读取一行命令
 *     ├─► do_command()   按 | 拆分管道命令
 *     ├─► do_redirect()  处理 < > >> 重定向（在子进程中调用）
 *     ├─► do_parse()     按空白字符拆分为 argv
 *     └─► do_pipe()      创建管道、fork、exec 执行命令
 *
 * ── 管道实现原理 ───────────────────────────────────────────────────────────
 *
 *   对于 "cmd1 | cmd2 | cmd3"（pipe_num = 2）：
 *
 *               pipefd[1]              pipefd[2]
 *   cmd1 ──┬────[1]──────[0]────┬── cmd2 ──┬────[1]──────[0]────┬── cmd3
 *          │                     │          │                     │
 *       stdout                stdin      stdout                stdin
 *
 *   每个命令的 stdout 写入下一个 pipe 的写端，stdin 从上一个 pipe 的读端读取。
 *
 * ── 已知限制 ───────────────────────────────────────────────────────────────
 *
 *   1. 不支持引号（ls "a b" 会被错误拆分）
 *   2. 不支持后台运行（&）
 *   3. 父进程顺序 waitpid，管道命令并非真正并行
 *   4. 不支持内建命令（cd、exit 需 exec 外部程序）
 *   5. 重定向符号必须被空格包围（如 "cat < file" 而非 "cat<file"）
 ******************************************************************************/

#include <stdio.h>       // printf, fflush, perror
#include <stdlib.h>      // exit
#include <string.h>      // memset, strcmp
#include <unistd.h>      // fork, execvp, pipe, dup2, close, getcwd, chdir
#include <sys/wait.h>    // waitpid
#include <ctype.h>       // isspace
#include <fcntl.h>       // open, O_RDONLY, O_WRONLY, O_CREAT, O_APPEND
#include <sys/stat.h>    // 文件权限宏

/******************************************************************************
 * 常量定义
 ******************************************************************************/

/// MAX_CMD — 单行命令的最大长度（含 '\0'）
#define MAX_CMD 1024

/// MAX_PIPE — 管道命令的最大数量
#define MAX_PIPE 32

/// MAX_ARGV — 单个命令的最大参数数量
#define MAX_ARGV 32

/******************************************************************************
 * 全局变量
 *
 * command[]       — 存放 do_face() 读到的整行命令
 * pipe_command[]  — 存放 do_command() 按 | 切分后的各段命令字符串
 ******************************************************************************/

char  command[MAX_CMD];                  // 原始命令行缓冲区
char *pipe_command[MAX_PIPE + 1];        // 各段管道命令指针数组（NULL 结尾）

/******************************************************************************
 * do_face — 打印提示符并读取一行命令
 *
 * @return  0: 成功读取   -1: 空行
 *
 * 细节：
 *   scanf("%[^\n]%*c", ...) 读取一整行（不含 '\n'），然后丢弃换行符。
 *   如果用户直接按回车（空行），scanf 返回 0，需要 getchar() 消费 '\n'。
 ******************************************************************************/
int do_face()
{
    memset(command, 0x00, MAX_CMD);              // 清空缓冲区
    printf("minishell$ ");                       // 打印提示符
    fflush(stdout);                              // 立即刷新，确保提示符显示

    // %[^\n]  — 匹配除 '\n' 外的所有字符
    // %*c     — 丢弃下一个字符（即 '\n'）
    int ret = scanf("%[^\n]%*c", command);
    if (ret == EOF) {
        // Ctrl+D 或管道关闭 → 优雅退出
        printf("\n");
        exit(0);
    }
    if (ret == 0) {
        getchar();                               // 消费残留的 '\n'
        return -1;                               // 空行，返回 -1
    }
    return 0;
}

/******************************************************************************
 * do_parse — 将命令字符串按空白字符拆分为 argv 数组
 *
 * @param buff  待拆分的命令字符串（会被原地修改！'\0' 会插入其中）
 * @return      指向静态 argv 数组的指针（以 NULL 结尾）
 *
 * 拆分规则：
 *   - 连续的非空白字符组成一个参数
 *   - 遇到空白字符时，将其替换为 '\0' 来终止前一个参数
 *
 * 示例：
 *   输入: "ls  -l  /tmp"
 *                 ↓ 原地修改
 *   buff:  "ls\0-l\0/tmp"
 *   argv:  ["ls", "-l", "/tmp", NULL]
 *
 * 注意：argv 是 static 的，保证返回后数组仍在有效生命期内。
 ******************************************************************************/
char **do_parse(char *buff)
{
    int argc = 0;
    static char *argv[MAX_ARGV];                 // static：返回到调用者后仍有效
    char *ptr = buff;

    while (*ptr != '\0') {
        if (!isspace((unsigned char)*ptr)) {
            // 非空白字符 → 新参数的开始
            argv[argc++] = ptr;

            // 扫描到下一个空白或字符串末尾
            while ((!isspace((unsigned char)*ptr)) && (*ptr) != '\0') {
                ptr++;
            }
            continue;                            // 继续外层循环处理剩余部分
        }

        // 空白字符 → 用 '\0' 截断前一个参数
        *ptr = '\0';
        ptr++;
    }

    argv[argc] = NULL;                           // execvp 需要的 NULL 结尾
    return argv;
}

/******************************************************************************
 * do_command — 按管道符 '|' 将一整行命令拆分为多个命令段
 *
 * @param buff  原始命令行字符串（会被原地修改）
 * @return      管道数量（0 表示没有管道，N 表示有 N 个 '|'）
 *
 * 拆分策略："原地切分"。遇到 '|' 就替换为 '\0'，使"上一段"成为一个独立字符串。
 *
 * 示例：
 *   输入: "ls -l | grep test | wc -l"
 *                               ↓ 原地替换
 *   buff:  "ls -l \0grep test \0wc -l"
 *            ↑         ↑          ↑
 *   pipe_command[0]    [1]        [2]
 *
 *   pipe_command[3] = NULL
 *
 *   pipe_num = 2
 ******************************************************************************/
int do_command(char *buff)
{
    int pipe_num = 0;
    char *ptr = buff;

    pipe_command[pipe_num] = ptr;                // 第一段命令的起始位置

    while (*ptr != '\0') {
        if (*ptr == '|') {
            pipe_num++;                          // 发现一个管道符
            *ptr++ = '\0';                       // 原地截断为 '\0'，指针移到下一个字符
            pipe_command[pipe_num] = ptr;        // 记录下一段命令的起始位置
            continue;
        }
        ptr++;
    }

    pipe_command[pipe_num + 1] = NULL;           // NULL 哨兵（便于调试）
    return pipe_num;                             // 返回管道数量
}

/******************************************************************************
 * do_redirect — 处理输入/输出重定向（在子进程中调用）
 *
 * @param cmd  单个命令字符串（如 "cat < input.txt > output.txt"）
 *
 * 识别并处理以下重定向符号：
 *   - '<'  + 空格 + 文件名：将标准输入重定向到该文件
 *   - '>'  + 空格 + 文件名：将标准输出重定向到该文件（覆盖）
 *   - '>>' + 空格 + 文件名：将标准输出追加重定向到该文件
 *
 * 实现方式：
 *   遍历命令字符串找到 '<' / '>' / '>>'，将符号位置置为 '\0'（使 argv
 *   解析在此处自然停止），然后打开目标文件并用 dup2() 替换标准 FD。
 *
 * 注意：
 *   - 重定向符号之后 argv 就截断了，所以不会传给 execvp
 *   - do_redirect 必须在 do_parse 之前调用，因为 do_parse 也会修改字符串
 ******************************************************************************/
void do_redirect(char *cmd)
{
    char *ptr = cmd;

    while (*ptr != '\0') {
        // ── 输出追加重定向 '>>' ──────────────────────────────────────────
        if (*ptr == '>' && *(ptr + 1) == '>') {
            *ptr = '\0';                         // 截断命令部分
            ptr += 2;                            // 跳过 ">>"
            while (isspace((unsigned char)*ptr)) ptr++;  // 跳过空白

            int fd = open(ptr, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) {
                perror("open >>");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);             // stdout → 文件
            close(fd);
            return;                              // 重定向处理完毕
        }

        // ── 输出重定向 '>' ────────────────────────────────────────────────
        if (*ptr == '>' && *(ptr + 1) != '>') {
            *ptr = '\0';                         // 截断命令部分
            ptr += 1;                            // 跳过 ">"
            while (isspace((unsigned char)*ptr)) ptr++;

            int fd = open(ptr, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open >");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);             // stdout → 文件
            close(fd);
            return;
        }

        // ── 输入重定向 '<' ────────────────────────────────────────────────
        if (*ptr == '<') {
            *ptr = '\0';                         // 截断命令部分
            ptr += 1;                            // 跳过 "<"
            while (isspace((unsigned char)*ptr)) ptr++;

            int fd = open(ptr, O_RDONLY);
            if (fd < 0) {
                perror("open <");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);              // stdin ← 文件
            close(fd);
            return;
        }

        ptr++;
    }
}

/******************************************************************************
 * do_pipe — 执行管道命令序列
 *
 * @param pipe_num  管道数量（= 命令数 - 1）
 * @return          0（成功）
 *
 * ── 管道工作原理 ──────────────────────────────────────────────────────────
 *
 *   对于 "cmd1 | cmd2 | cmd3"（pipe_num = 2），fork 出 3 个子进程：
 *
 *   子进程 0 (cmd1):
 *     - stdout 重定向到 pipefd[1][1]（下一根管道的写端）
 *     - stdin  不变（从终端读取）
 *
 *   子进程 1 (cmd2):
 *     - stdin  重定向到 pipefd[1][0]（上一根管道的读端）
 *     - stdout 重定向到 pipefd[2][1]（下一根管道的写端）
 *
 *   子进程 2 (cmd3):
 *     - stdin  重定向到 pipefd[2][0]（上一根管道的读端）
 *     - stdout 不变（输出到终端）
 *
 *   父进程：
 *     - fork 每个子进程后立即关闭自己持有的 pipe fd（避免 FD 泄漏）
 *     - waitpid 等待每个子进程结束（顺序等待，不是并行）
 *
 *   为什么 fork 前不创建所有 pipe？
 *     实际上是在循环内部创建 pipe 的：第一轮创建 pipe[0]，第二轮创建 pipe[1]...
 *     但代码里是把 pipe() 放在一个独立的 for 循环中先全部创建好，然后
 *     第二个 for 循环再 fork。这确保了所有 pipe fd 在 fork 前都已就绪。
 ******************************************************************************/
int do_pipe(int pipe_num)
{
    int pid = 0;
    int i;
    int pipefd[MAX_PIPE][2] = {{0}};             // pipefd[i][0]=读端, [1]=写端
    char **argv = NULL;

    /**************************************************************************
     * 第一阶段：创建所有管道
     *
     * 因为需要在 fork 之前就创建好所有 pipe，子进程才能继承完整的 fd 表。
     * 这里 pipefd[0] 实际上不会被任何子进程使用（它是多余的），
     * 但为了索引简洁（pipefd[i] 对应第 i 个命令的输入），保留它。
     **************************************************************************/
    for (i = 0; i <= pipe_num; i++) {
        if (pipe(pipefd[i]) < 0) {
            perror("pipe");
            exit(1);
        }
    }

    /**************************************************************************
     * 第二阶段：逐个 fork 子进程并设置管道
     *
     * 对于第 i 个子进程：
     *   - 如果 i != 0:        stdin  ← pipefd[i][0]（从前一个管道读）
     *   - 如果 i != pipe_num: stdout → pipefd[i+1][1]（写入下一个管道）
     *   - 然后 execvp 执行命令
     *
     * 管道 fd 使用完后要立即 close，原因：
     *   1. 防止 FD 泄漏
     *   2. 确保管道对端能读到 EOF —— 只有所有写端都关闭时，read() 才返回 0
     *
     * ⚠️ 已知问题：
     *   父进程在每次 fork 后立即 waitpid，导致子进程是串行执行而非并行。
     *   正确的做法是先 fork 所有子进程，然后统一 wait。这里保留原始实现方式。
     **************************************************************************/
    for (i = 0; i <= pipe_num; i++) {
        pid = fork();
        if (pid == 0) {
            // ── 子进程 ─────────────────────────────────────────────────────

            // 1. 处理输入/输出重定向（< > >>）
            do_redirect(pipe_command[i]);

            // 2. 解析当前命令为 argv 数组
            argv = do_parse(pipe_command[i]);

            // 3. 设置管道：stdin 重定向
            //    第一个命令（i==0）不需要重定向 stdin（保持从终端读）
            if (i != 0) {
                close(pipefd[i][1]);             // 关闭当前管道写端（不需要）
                dup2(pipefd[i][0], STDIN_FILENO);// stdin ← 当前管道读端
                // dup2 成功后，stdin(FD 0) 和 pipefd[i][0] 指向同一文件
                // 后面 close(pipefd[i][0]) 不影响 stdin
            }

            // 4. 设置管道：stdout 重定向
            //    最后一个命令（i==pipe_num）不需要重定向 stdout（保持输出到终端）
            if (i != pipe_num) {
                close(pipefd[i + 1][0]);         // 关闭下一管道读端（不需要）
                dup2(pipefd[i + 1][1], STDOUT_FILENO); // stdout → 下一管道写端
            }

            // 5. 关闭本子进程中所有剩余的 pipe fd
            //    避免 fd 泄漏，同时也保证对端能正常读到 EOF
            for (int j = 0; j <= pipe_num; j++) {
                close(pipefd[j][0]);
                close(pipefd[j][1]);
            }

            // 6. 执行命令
            execvp(argv[0], argv);

            // 如果能执行到这里，说明 execvp 失败了
            perror("execvp");
            exit(1);

        } else if (pid > 0) {
            // ── 父进程 ─────────────────────────────────────────────────────

            // 父进程不再需要当前管道的两端，立即关闭
            // 注意：这不会影响子进程，因为 dup2 后子进程的 FD 0/1 已经独立了
            close(pipefd[i][0]);
            close(pipefd[i][1]);

            // 等待当前子进程结束（顺序等待）
            waitpid(pid, NULL, 0);

        } else {
            // fork 失败
            perror("fork");
            exit(1);
        }
    }

    return 0;
}

/******************************************************************************
 * main — Shell 主循环
 *
 * 循环：
 *   1. do_face()   — 打印提示符，读取一行命令
 *   2. do_command()— 按 | 拆分为多条命令
 *   3. do_pipe()   — 创建管道并执行
 ******************************************************************************/
int main()
{
    int pipe_num;

    while (1) {
        // 步骤 1：读取命令（空行则继续下一轮）
        if (do_face() < 0)
            continue;

        // 步骤 2：按管道符拆分
        pipe_num = do_command(command);

        // 步骤 3：执行管道命令
        do_pipe(pipe_num);
    }

    return 0;
}
