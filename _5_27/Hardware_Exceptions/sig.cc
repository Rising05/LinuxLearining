#include <iostream>
#include <unistd.h>
#include <signal.h>

void handler(int signum){
    printf("catch a sig : %d\n", signum);
}

int main(){
    sleep(1);

    int a = 10;
    
    a /= 0;

    while(1){

    }

    return 0;
}