CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -IIncludes

SRC_DIR = Source
INC_DIR = Includes
BUILD_DIR = Builds

TARGET = ghost_chat.out

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/chat.c \
       $(SRC_DIR)/session.c \
       $(SRC_DIR)/history.c \
       $(SRC_DIR)/signals.c

OBJS = $(BUILD_DIR)/main.o \
       $(BUILD_DIR)/chat.o \
       $(BUILD_DIR)/session.o \
       $(BUILD_DIR)/history.o \
       $(BUILD_DIR)/signals.o

HEADERS = $(INC_DIR)/common.h \
          $(INC_DIR)/chat.h \
          $(INC_DIR)/session.h \
          $(INC_DIR)/history.h \
          $(INC_DIR)/signals.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -f $(BUILD_DIR)/*.o $(TARGET)

re: clean all

.PHONY: all clean re
