CC = gcc
CFLAGS = -Wall -Wextra -std=c11

all: my_stdio.o

my_stdio.o: my_stdio.c my_stdio.h
	$(CC) $(CFLAGS) -c -o $@ my_stdio.c

clean:
	rm -f my_stdio.o
