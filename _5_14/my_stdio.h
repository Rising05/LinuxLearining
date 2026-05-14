#pragma once

#define SIZE 1024
#define FLUSH_NONE 0
#define FLUSH_LINE 1
#define FLUSH_FULL 2

struct IO_FILE{
    int flag;  //刷新方式
    int fileno;  //文件描述符
    char outbuffer[SIZE];  //用户级缓冲
    int cap;  //缓冲区容量
    int size;  //当前缓冲区已有数据大小
};

typedef struct IO_FILE mFILE;
mFILE *my_fopen(const char *pathname, const char *mode);
int mfwrite(const void* ptr, int num, mFILE *stream);
int my_fflush(mFILE *stream);
int my_fclose(mFILE *stream);
