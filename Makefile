CC = gcc
CFLAGS = -Wall -Wextra -O2

SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=bin/%)

all: $(OBJ)

bin/%: src/%.c
	@mkdir -p bin
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf bin/*

.PHONY: all clean
