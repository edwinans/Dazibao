CC=gcc
CFLAGS= -g -Wall -std=c11 -I -o 

.PHONY:all 
all: test

test: test.o tlv.o
	$(CC) -o test test.o tlv.o -I src 

.PHONY:clean
clean:
	@rm -rf *.o
