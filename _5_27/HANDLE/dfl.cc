#include <iostream>
#include <unistd.h>
#include <signal.h>
void handler(int signum){
    std::cout << "I am: " << getpid() << ", I received signal: " << signum << std::endl;
}
int main(){
    signal(SIGINT, SIG_DFL); // 使用默认信号处理方式 一般来说默认动作就是终止进程
    while(1){
        std::cout << "I am a process, I am waiting for a signal..." << std::endl;
        sleep(1);
    }
    return 0;
}