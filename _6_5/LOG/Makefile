CXX      = g++
CXXFLAGS = -std=c++17 -Wall -pthread
TARGET   = log

OBJS = main.o Log.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

main.o: main.cpp Log.hpp Lock.hpp
Log.o:  Log.cpp  Log.hpp Lock.hpp

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
