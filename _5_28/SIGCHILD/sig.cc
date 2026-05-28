#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

void handler(int signum) {
    std::cout << "Received signal: " << signum << std::endl;


    while(waitpid(-1, nullptr, WNOHANG) > 0) {
        std::cout << "Child process terminated" << std::endl;
    }
}

int main() {
    signal(SIGCHLD, handler);

    pid_t pid = fork();

    if (pid == 0) {
        // Child process
        std::cout << "Child process started with PID: " << getpid() << std::endl;
        sleep(2); // Simulate some work
        std::cout << "Child process exiting" << std::endl;
        return 0;
    } else if (pid > 0) {
        // Parent process
        std::cout << "Parent process waiting for child to terminate..." << std::endl;
        pause(); // Wait for signals
    } else {
        std::cerr << "Fork failed" << std::endl;
        return 1;
    }
    return 0;
}