all: main

main: main.cc thread.hpp
	g++ -o main main.cc -lpthread

.PHONY: clean
clean:
	rm -f main
