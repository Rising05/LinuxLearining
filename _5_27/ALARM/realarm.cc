#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <vector>
#include <functional>

using func_t = std::function<void()>;

int gcount = 0;

std::vector<func_t> gfuncs;

void handler(int signum){
    std::cout << "Received signal: " << signum << std::endl;
    for(auto& func : gfuncs){
        func();
    }
    std::cout << "gcount: " << gcount << std::endl;

    int n = alarm(1);
    std::cout << "剩余时间: " << n << std::endl;
}

int main(){
    // 可以把一些周期性任务注册进来
    // 例如刷新内核数据、检查进程时间片、内存管理等
    /*
    gfuncs.push_back([](){
        std::cout << "我是一个内核刷新操作" << std::endl;
    });

    gfuncs.push_back([](){
        std::cout << "我是一个检测进程时间片的操作，如果时间片到了，我会切换进程" << std::endl;
    });

    gfuncs.push_back([](){
        std::cout << "我是一个内存管理操作，定期清理操作系统内部的内存碎片" << std::endl;
    });
    */

    alarm(1);

    signal(SIGALRM, handler);

    while(1){
        pause(0);

        std::cout << "I woke up..." << std::endl;

        gcount++;
    }
    
    return 0;
}