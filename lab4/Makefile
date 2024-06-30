# 定义变量
CC = gcc
CFLAGS = -Wall -g

# 定义目标文件名
TARGET = fat

# 定义源文件和头文件
SRCS = main.c fat32.c
HEADERS = fat32.h

# 定义生成的目标文件
OBJS = $(SRCS:.c=.o)

# 伪目标
.PHONY: all clean

# 默认目标
all: $(TARGET)

# 生成可执行文件
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# 生成目标文件
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@


# 清理目标
clean:
	rm -f $(OBJS) $(TARGET)
