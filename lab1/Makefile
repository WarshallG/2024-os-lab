CC = gcc
OBJS = nsh.o

shell:$(OBJS)
	$(CC) $^ -o nsh

%.o:%.c
	$(CC) -c $^

run:
	./nsh
