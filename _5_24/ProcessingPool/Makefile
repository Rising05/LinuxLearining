# ============================================================
# ProgressingPool 项目 Makefile
# 用途：编译进程池（Process Pool）演示程序
# ============================================================

# 编译器：使用 GNU C++ 编译器
CXX = g++

# 编译选项：
#   -std=c++11  使用 C++11 标准（支持 lambda、auto、emplace_back 等特性）
#   -Wall       开启所有常见警告（-W 是警告，all 表示全部）
#   -g          生成调试信息，方便用 gdb 调试
#   -O0         关闭优化，确保调试时变量不被优化掉
CXXFLAGS = -std=c++11 -Wall -g -O0

# 目标可执行文件名
TARGET = processpool

# 源文件列表：所有需要编译的 .cpp 文件
# 当新增 .cpp 文件时，在此处添加文件名
SRCS = main.cpp processpool.cpp

# 目标文件列表：将 .cpp 后缀替换为 .o 后缀
# $(SRCS:.cpp=.o) 是 Makefile 的替换引用语法
# 效果：main.cpp processpool.cpp → main.o processpool.o
OBJS = $(SRCS:.cpp=.o)

# 默认目标：make 不带参数时执行此目标
# 依赖 $(TARGET)，即编译生成可执行文件
all: $(TARGET)

# 链接规则：将目标文件链接成可执行文件
# $@ 是自动变量，代表当前目标名（即 $(TARGET) = processpool）
# $^ 是自动变量，代表所有依赖文件（即 $(OBJS) = main.o processpool.o）
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# 编译规则（隐式规则）：将 .cpp 文件编译为 .o 文件
# Makefile 内置隐式规则 $(CXX) $(CXXFLAGS) -c $< -o $@
# 以下显式写出以便修改，同时添加头文件依赖检测
# $< 是自动变量，代表第一个依赖文件（即对应的 .cpp 文件）
# $@ 是自动变量，代表当前目标名（即对应的 .o 文件）
%.o: %.cpp channel.hpp taskmanager.hpp processpool.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理目标：删除编译生成的文件
# .PHONY 声明 clean 为伪目标，即使存在名为 clean 的文件也强制执行
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)

# 运行目标：编译后运行程序
# 依赖 $(TARGET)，确保先编译再运行
.PHONY: run
run: $(TARGET)
	./$(TARGET)

# 重新编译：先清理再编译
# 等价于依次执行 make clean && make
.PHONY: rebuild
rebuild: clean all
