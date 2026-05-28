#include <iostream>
#include <unistd.h>
#include <signal.h>

void handler(int signum){
    std::cout << "I am: " << getpid() << std::endl;
    std::cout << "I received signal: " << signum << std::endl;
}
int main(){
    std::cout << "My PID is: " << getpid() << std::endl;
    signal(SIGQUIT, handler);
    while(1){
        std::cout << "I am a process, I am waiting for signal..." << std::endl;
        sleep(1);
    }
    return 0;
}