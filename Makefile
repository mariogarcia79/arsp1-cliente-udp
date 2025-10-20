CC = gcc
CFLAGS = -Wall -Wextra -O2

SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=bin/%.o)

all: $(OBJ)

bin/%.o: src/%.c
	@mkdir -p bin
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf bin/*.o

.PHONY: all clean