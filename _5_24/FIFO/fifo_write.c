/**
 * fifo_write.c — FIFO 写入端（客户端）
 *
 * 功能：
 *   1. 打开已存在的命名管道 "mypipe"（需读端先启动创建）
 *   2. 从标准输入读取用户输入
 *   3. 将输入数据写入 FIFO 发送给读端
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
#include <string.h>

#define ERR_EXIT(m)              \
    do {                         \
        perror(m);               \
        exit(EXIT_FAILURE);      \
    } while (0)

int main()
{
    int wfd = open("mypipe", O_WRONLY);
    if (wfd < 0) {
        ERR_EXIT("open");
    }

    char buf[1024];
    while (1) {
        buf[0] = 0;
        printf("Please Enter# ");
        fflush(stdout);
        ssize_t s = read(0, buf, sizeof(buf) - 1);
        if (s > 0) {
            buf[s] = 0;
            write(wfd, buf, strlen(buf));
        }
        else if (s <= 0) {
            ERR_EXIT("read");
        }
    }

    close(wfd);
    return 0;
}
