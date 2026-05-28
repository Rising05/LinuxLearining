#include <iostream>
#include <unistd.h>

int main(){
    while(1){
        std::cout << "I am a process, I am waiting for a signal..." << std::endl;
        sleep(1);
    }

    return 0;
}