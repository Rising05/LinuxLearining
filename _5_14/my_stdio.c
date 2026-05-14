#include "my_stdio.h"
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

mFILE *my_fopen(const char *filename, const char *mode){
    int fd = -1;

    if(strcmp(mode, "r") == 0){
        fd = open(filename, O_RDONLY);
    }
    else if(strcmp(mode, "w") == 0){
        fd = open(filename, O_WRONLY | O_CREAT, 0666);
    }
    else if(strcmp(mode, "a") == 0){
        fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
    }
    
    if(fd < 0) return NULL;

    mFILE *mf = malloc(sizeof(mFILE));
    if(!mf){
        close(fd);
        return NULL;
    }

    mf->fileno = fd;
    mf->flag = FLUSH_NONE;
    mf->cap = SIZE;
    mf->size = 0;

    return mf;
}

int my_fflush(mFILE *stream)
{
    if(stream->size > 0)
    {
        // 写到内核文件缓冲区中
        write(stream->fileno, stream->outbuffer, stream->size);

        // 尝试刷新到外设
        fsync(stream->fileno);

        stream->size = 0;
    }

    return 0;
}

int mfwrite(const void *ptr, int num, mFILE *stream)
{
    // 1. 拷贝到用户级缓冲区
    memcpy(stream->outbuffer + stream->size, ptr, num);
    stream->size += num;

    // 2. 行刷新：如果最后一个字符是 '\n'，就刷新
    if(stream->flag == FLUSH_LINE &&
       stream->size > 0 &&
       stream->outbuffer[stream->size - 1] == '\n')
    {
        my_fflush(stream);
    }

    return num;
}

int my_fclose(mFILE *stream)
{
    if(stream->size > 0)
    {
        my_fflush(stream);
    }

    close(stream->fileno);
    free(stream);
    return 0;
}