#include <iostream>
#include "Log.hpp"

using namespace LogModule;

void fun() {
    int a = 10;
    LOG(LogLevel::FATAL) << "hello world" << 1234 << ", 3.14" << 'c' << a;
}

int main() {
    // ENABLE_CONSOLE_LOG_STRATEGY();
    LOG(LogLevel::DEBUG) << "hello world";
    LOG(LogLevel::DEBUG) << "hello world";
    LOG(LogLevel::DEBUG) << "hello world";

    // ENABLE_FILE_LOG_STRATEGY();
    LOG(LogLevel::DEBUG) << "hello world";
    LOG(LogLevel::DEBUG) << "hello world";
    LOG(LogLevel::WARNING) << "hello world";

    fun();
    return 0;
}
