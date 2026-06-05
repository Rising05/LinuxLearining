# ============================================================================
#  BlockQueue 项目 —— 最简 Makefile
# ============================================================================
#
#  用法:
#    make         编译生成可执行文件 test
#    make run     编译并运行
#    make clean   删除编译产物
#
# ============================================================================

# --- 编译器 & 编译选项 ---

CXX      = g++                           # C++ 编译器
CXXFLAGS = -std=c++11 -Wall -Wextra -g -O0   # C++11标准 | 所有警告 | 调试信息 | 无优化

# --- 链接选项 ---

LDFLAGS  = -lpthread                     # ★ 必须链接 pthread 库（BlockQueue 用到了 pthread_mutex / pthread_cond）

# --- 目标 & 源文件 ---

TARGET   = test                          # 最终生成的可执行文件名
SRCS     = test.cpp Task.cc              # 所有需要编译的 .cpp 文件（BlockQueue.hpp 是模板头文件，不需要单独编译）
OBJS     = $(SRCS:.cpp=.o)               # 自动推导 .o 文件名：test.o Task.o

# 处理 .cc 后缀：将 Task.cc 映射为 Task.o
OBJS     := $(OBJS:.cc=.o)

# --- 默认目标: 编译 ---
# 第一条规则是 make 的默认目标

all: $(TARGET)

# --- 链接: 将 .o 文件链接为可执行文件 ---

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
#         ↑          ↑  ↑  ↑
#         编译器      输出 所有依赖       链接 pthread
#                 文件名  (.o 文件)

# --- 编译: .cpp → .o ---
# 这是一个模式规则:
#   %.o : %.cpp  表示任何 .o 文件都依赖同名的 .cpp 文件
#   $<  表示第一个依赖（即 .cpp 文件）
#   $@  表示目标（即 .o 文件）

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- 编译: .cc → .o（与上一条同理）---

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- 运行 ---

run: $(TARGET)
	@echo "============================================"
	@echo "  开始运行 $(TARGET) ..."
	@echo "============================================"
	@./$(TARGET)

# --- 清理 ---

clean:
	rm -f $(OBJS) $(TARGET)

# --- 声明伪目标（防止与同名文件冲突） ---

.PHONY: all run clean