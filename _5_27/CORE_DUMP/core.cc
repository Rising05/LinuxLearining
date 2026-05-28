#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

int main(){
    // 子进程
    if(fork() == 0){
        sleep(1);

        int a = 10;

        a /= 0;

        exit(0);
    }

    int status = 0;

    waitpid(-1, &status, 0);

    printf("exitg signal: %d, core dump: %d\n", (status & 0x7f), (status & 0x80) >> 7);
    return 0;
}