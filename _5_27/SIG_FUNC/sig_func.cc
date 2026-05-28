#include <iostream>
#include <unistd.h>
#include <signal.h>

void handler(int signum){
    std::cout << "Received signal: " << signum << std::endl;
}

int main(){
    std::cout << "我是进程：" << getpid() << std::endl; 
    signal(SIGINT, handler); // 注册信号处理函数
    while(1){
        std::cout << "I am a process, I am waiting for a signal..." << std::endl;
        sleep(1);
    }
}