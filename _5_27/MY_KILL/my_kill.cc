#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

int main(int argc, char* argv[]){
    if(argc != 3){
        std::cerr << "Usage: " << argv[0] << " -<signumber> <pid>" << std::endl;
        return 1;
    }

    // 传的是 -1
    int num = std::stoi(argv[1] + 1);
    pid_t pid = std::stoi(argv[2]);

    int n = kill(pid, num);
    return n;
}