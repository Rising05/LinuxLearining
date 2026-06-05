#include "Task.hpp"
#include <sstream>

// ==================== 构造函数 ====================

Task::Task()
    : _id(0), _a(0), _b(0), _type(Type::ADD)
{}

Task::Task(int id, int a, int b, Type type)
    : _id(id), _a(a), _b(b), _type(type)
{}

// ==================== 核心方法 ====================

int Task::Run() const {
    int result = 0;

    switch (_type) {
    case Type::ADD:
        result = _a + _b;
        std::cout << "[Task #" << _id << "] " << _a << " + " << _b
                  << " = " << result << std::endl;
        break;

    case Type::MUL:
        result = _a * _b;
        std::cout << "[Task #" << _id << "] " << _a << " * " << _b
                  << " = " << result << std::endl;
        break;
    }

    return result;
}

std::string Task::ToString() const {
    std::ostringstream oss;

    oss << "Task{ id=" << _id
        << ", a=" << _a
        << ", b=" << _b
        << ", type=";

    switch (_type) {
    case Type::ADD: oss << "ADD"; break;
    case Type::MUL: oss << "MUL"; break;
    }

    oss << " }";
    return oss.str();
}
