# === Compiler and flags ===
CC      := gcc
CFLAGS  := -Wall -Wextra -Iinclude
LDFLAGS := 

# === Directories ===
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

# === Target binary ===
TARGET  := $(BIN_DIR)/program

# === Source and object files ===
SRC := $(wildcard $(SRC_DIR)/**/*.c $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))

# === Default target ===
all: $(TARGET)

# === Linking ===
$(TARGET): $(OBJ) | $(BIN_DIR)
	@echo "Linking $@"
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

# === Compilation ===
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "Compiling $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# === Create directories if missing ===
$(BIN_DIR) $(OBJ_DIR):
	@mkdir -p $@

# === Cleanup ===
clean:
	@echo "Cleaning build files..."
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# === Phony targets ===
.PHONY: all clean
