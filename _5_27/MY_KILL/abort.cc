#include <iostream>
#include <unistd.h>
#include <signal.h>

void handler(int signum){
    std::cout << "Received signal: " << signum << std::endl;
}
int main(){
    signal(SIGABRT, handler);

    while(1){
        sleep(1);
        
        abort();
    }
    return 0;
}