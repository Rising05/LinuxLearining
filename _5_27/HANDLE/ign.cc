#include <iostream>
#include <unistd.h>
#include <signal.h>
void handler(int signum){
    std::cout << "I am: " << getpid() << ", I received signal: " << signum << std::endl;
}
int main(){
    signal(SIGINT, SIG_IGN); // 忽略 SIGINT 信号
    while(1){
        std::cout << "I am a process, I am waiting for a signal..." << std::endl;
        sleep(1);
    }
    return 0;
}