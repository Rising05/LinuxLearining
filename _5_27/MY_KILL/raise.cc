#include <iostream>
#include <unistd.h>
#include <signal.h>

void handler(int signum){
    std::cout << "Received signal: " << signum << std::endl;
}
int main(){
    signal(2, handler);

    while(1){
        sleep(1);
        raise(2);
    }
    return 0;
}