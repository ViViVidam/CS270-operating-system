CC = gcc
makefuse:
	$(CC) -Wall main.c -o main `pkg-config fuse --cflags --libs`