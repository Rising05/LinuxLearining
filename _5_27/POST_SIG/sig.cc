#include <iostream>
#include <unistd.h>
#include <signal.h>

int main()
{
    while (true)
    {
        // 程序什么也不做，只是保持运行
        sleep(1);
    }

    return 0;
}