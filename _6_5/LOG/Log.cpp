#include "Log.hpp"

namespace LogModule {

// ==================== 自由函数实现 ====================

std::string LogLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

std::string GetCurrTime() {
    time_t tm = time(nullptr);
    struct tm curr;
    localtime_r(&tm, &curr);

    char timebuffer[64];
    snprintf(timebuffer, sizeof(timebuffer), "%4d-%02d-%02d %02d:%02d:%02d",
             curr.tm_year + 1900,
             curr.tm_mon + 1,
             curr.tm_mday,
             curr.tm_hour,
             curr.tm_min,
             curr.tm_sec);
    return timebuffer;
}

// ==================== ConsoleLogStrategy ====================

void ConsoleLogStrategy::SyncLog(const std::string &message) {
    LockGuard lockGuard(_mutex);
    std::cerr << message << std::endl;
}

ConsoleLogStrategy::~ConsoleLogStrategy() {
    // std::cout << "~ConsoleLogStrategy" << std::endl;
}

// ==================== FileLogStrategy ====================

FileLogStrategy::FileLogStrategy(const std::string logpath, std::string logfilename)
    : _logpath(logpath), _logfilename(logfilename) {
    LockGuard lockguard(_mutex);
    if (std::filesystem::exists(_logpath))
        return;
    try {
        std::filesystem::create_directories(_logpath);
    } catch (const std::filesystem::filesystem_error &e) {
        std::cerr << e.what() << '\n';
    }
}

void FileLogStrategy::SyncLog(const std::string &message) {
    LockGuard lockguard(_mutex);
    std::string log = _logpath + _logfilename;
    std::ofstream out(log.c_str(), std::ios::app);
    if (!out.is_open())
        return;
    out << message << "\n";
    out.close();
}

FileLogStrategy::~FileLogStrategy() {
    // std::cout << "~FileLogStrategy" << std::endl;
}

// ==================== Logger ====================

Logger::Logger() {
    UseConsoleStrategy();
}

Logger::~Logger() {}

void Logger::UseConsoleStrategy() {
    _strategy = std::make_unique<ConsoleLogStrategy>();
}

void Logger::UseFileStrategy() {
    _strategy = std::make_unique<FileLogStrategy>();
}

Logger::LogMessage Logger::operator()(LogLevel type, std::string filename, int line) {
    return LogMessage(type, filename, line, *this);
}

// ==================== Logger::LogMessage ====================

Logger::LogMessage::LogMessage(LogLevel type, std::string filename, int line, Logger &logger)
    : _type(type),
      _curr_time(GetCurrTime()),
      _pid(getpid()),
      _filename(filename),
      _line(line),
      _logger(logger) {
    std::stringstream ssbuffer;
    ssbuffer << "[" << _curr_time << "] "
             << "[" << LogLevelToString(type) << "] "
             << "[" << _pid << "] "
             << "[" << _filename << "] "
             << "[" << _line << "]"
             << " - ";
    _loginfo = ssbuffer.str();
}

Logger::LogMessage::~LogMessage() {
    if (_logger._strategy) {
        _logger._strategy->SyncLog(_loginfo);
    }
}

// ==================== 全局对象定义 ====================

Logger logger;

} // namespace LogModule
