#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

// IO少
int count = 0;
void handler(int sig_num){
    std::cout << "count: " << count << std::endl;
    exit(0);
}
int main(){
    signal(SIGALRM, handler);
    alarm(1);

    while(1){
        count++;
    }
    return 0;
}

// // IO多
// int main(){
//     int count = 0;
//     alarm(1);

//     while(1){
//         std::cout << count++ << std::endl;
//     }

//     return 0;
// }