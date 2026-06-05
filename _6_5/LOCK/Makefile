CXX      = g++
CXXFLAGS = -std=c++11 -Wall -pthread

ticket: ticket.cc Lock.hpp
	$(CXX) $(CXXFLAGS) -o $@ ticket.cc

.PHONY: clean
clean:
	rm -f ticket
