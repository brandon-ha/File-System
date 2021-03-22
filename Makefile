# 'make' will compile code

.PHONY = all debug clean

CC = g++
CFLAGS = -Wall -std=c++11 -Iinclude
TARGET = fs

SRC_DIR = src

SRC := $(wildcard $(SRC_DIR)/*.cpp)

all: $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

debug: $(SRC)
	$(CC) $(CFLAGS) -g -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)