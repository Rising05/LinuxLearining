#pragma once
#include <iostream>

class nocopy {
    public:
    nocopy() {}

    nocopy(const nocopy &) = delete; // 禁止拷贝构造函数    

    const nocopy &operator=(const nocopy &) = delete; // 禁止拷贝赋值运算符

    ~nocopy() {}
};