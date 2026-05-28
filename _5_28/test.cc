#include <iostream>
#include <unistd.h>
#include <signal.h>

// 打印pending信号集
void print_pending(const sigset_t &pending){
    if(sigismember(&pending, SIGINT)){
        std::cout << "SIGINT is pending" << std::endl;
    } else {
        std::cout << "SIGINT is not pending" << std::endl;
    }
}
int main(){
    sigset_t block_set, old_set;

    sigemptyset(&block_set);
    sigaddset(&block_set, SIGINT);
    sigprocmask(SIG_BLOCK, &block_set, &old_set);

    int count = 0;

    while(1){
        sigset_t pending;
        sigpending(&pending);
        print_pending(pending);
        sleep(1);
        count++;
        if(count == 5){
            std::cout << "Unblocking SIGINT" << std::endl;
            sigprocmask(SIG_SETMASK, &old_set, nullptr);
        }
    }
    return 0;
}