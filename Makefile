CC=gcc
CFLAGS=-Wall -Wextra -std=gnu17 -ggdb

ifeq ($(TARGET),wasm)
	CC=emcc
	CFLAGS=-o output.js
endif

SRC_DIR=src
OBJ_DIR=obj

TARGET=cws
ifeq ($(TARGET), wasm)
	TARGET=wasm
endif

SRCS=$(wildcard $(SRC_DIR)/*.c)
OBJS=$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

$(shell mkdir -p $(OBJ_DIR))

ifeq ($(TARGET), wasm)
$(TARGET): $(SRCS) 
	$(CC) $(CFLAGS) $(SRCS) -s "EXPORTED_RUNTIME_METHODS=['ccall']"
else 
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
endif


ifneq ($(TARGET), wasm)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
endif

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
