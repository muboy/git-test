CC=gcc
CFLAGS=-Wall -O2 -D_GNU_SOURCE -c

OUT_DIR=bin

echo_server_epoll: echo_server_epoll.o
	$(CC) echo_server_epoll.o -o $(OUT_DIR)/echo_server_epoll

echo_server_epoll.o: echo_server_epoll.c
	$(CC) $(CFLAGS) echo_server_epoll.c -o echo_server_epoll.o
clean:
	rm bin/*
