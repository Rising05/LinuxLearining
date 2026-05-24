/**
 * fifo_read.c — FIFO 读取端（服务端）
 *
 * 功能：
 *   1. 使用 mkfifo 创建命名管道 "mypipe"
 *   2. 阻塞等待写端连接
 *   3. 循环读取并打印客户端消息
 *   4. 写端关闭后，read 返回 0，程序退出
 *
 * 运行方式（需要两个终端）：
 *   终端1: ./fifo_read
 *   终端2: ./fifo_write
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define ERR_EXIT(m)              \
    do {                         \
        perror(m);               \
        exit(EXIT_FAILURE);      \
    } while (0)

int main()
{
    umask(0);
    if (mkfifo("mypipe", 0644) < 0) {
        ERR_EXIT("mkfifo");
    }

    int rfd = open("mypipe", O_RDONLY);
    if (rfd < 0) {
        ERR_EXIT("open");
    }

    char buf[1024];
    while (1) {
        buf[0] = 0;
        printf("Please wait...\n");
        ssize_t s = read(rfd, buf, sizeof(buf) - 1);
        if (s > 0) {
            buf[s - 1] = 0;
            printf("client say# %s\n", buf);
        }
        else if (s == 0) {
            printf("client quit, exit now!\n");
            exit(EXIT_SUCCESS);
        }
        else {
            ERR_EXIT("read");
        }
    }

    close(rfd);
    return 0;
}
