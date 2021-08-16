CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic
LFLAGS=-lpthread
BIN=proj2
SOURCE=proj2.c

all:
	$(CC) $(CFLAGS) $(SOURCE) $(LFLAGS) -o $(BIN)
	
run: all
	./$(BIN)
	
clear:
	rm $(BIN)
