#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <memory>
#include <ctime>
#include <sstream>
#include <filesystem>
#include <unistd.h>
#include "Lock.hpp"

namespace LogModule {

using namespace LockModule;

const std::string defaultpath = "./log/";
const std::string defaultname = "log.txt";

// 日志等级
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

// 日志等级转字符串
std::string LogLevelToString(LogLevel level);

// 获取当前可读时间
std::string GetCurrTime();

// 策略模式：日志刷新策略接口
class LogStrategy {
public:
    virtual ~LogStrategy() = default;
    virtual void SyncLog(const std::string &message) = 0;
};

// 控制台日志策略
class ConsoleLogStrategy : public LogStrategy {
public:
    void SyncLog(const std::string &message) override;
    ~ConsoleLogStrategy();

private:
    Mutex _mutex;
};

// 文件日志策略
class FileLogStrategy : public LogStrategy {
public:
    FileLogStrategy(const std::string logpath = defaultpath,
                    std::string logfilename = defaultname);
    void SyncLog(const std::string &message) override;
    ~FileLogStrategy();

public:
    std::string _logpath;
    std::string _logfilename;
    Mutex _mutex;
};

// 具体的日志类
class Logger {
public:
    Logger();
    ~Logger();

    void UseConsoleStrategy();
    void UseFileStrategy();

    // 内部类：一条完整的日志对象（RAII 风格）
    class LogMessage {
    public:
        LogMessage(LogLevel type, std::string filename, int line, Logger &logger);
        ~LogMessage();

        // 模板必须在头文件中定义
        template <typename T>
        LogMessage &operator<<(const T &info) {
            std::stringstream ssbuffer;
            ssbuffer << info;
            _loginfo += ssbuffer.str();
            return *this;
        }

    private:
        LogLevel    _type;
        std::string _curr_time;
        pid_t       _pid;
        std::string _filename;
        int         _line;
        Logger     &_logger;
        std::string _loginfo;
    };

    // 创建临时 LogMessage 对象
    LogMessage operator()(LogLevel type, std::string filename, int line);

private:
    std::unique_ptr<LogStrategy> _strategy;
};

// 全局 logger 对象（extern 声明）
extern Logger logger;

// 宏：方便获取文件名和行号
#define LOG(type) logger(type, __FILE__, __LINE__)

// 选择日志策略的宏
#define ENABLE_CONSOLE_LOG_STRATEGY() logger.UseConsoleStrategy()
#define ENABLE_FILE_LOG_STRATEGY()    logger.UseFileStrategy()

} // namespace LogModule
